#include "types.h"
#include "x86.h"
#include "defs.h"
#include "spinlock.h"
#include "traps.h"
#include "msg.h"

static struct spinlock mouselock;
static struct
{
  int x_sgn, y_sgn, x_mov, y_mov;
  int l_btn, r_btn, m_btn;
  int x_overflow, y_overflow;
  uint tick;
} packet;
static int count;
static int recovery;
static int lastbtn, isdown, lastdowntick, lastmsgtick;
static int lastid;

int mouse_x, mouse_y;

void mouse_wait(uchar type)
{
  uint time_out = 100000;
  if (type == 0)
  {
    while (--time_out)
    {
      if ((inb(0x64) & 1) == 1)
        return;
    }
  }
  else
  {
    while (--time_out)
    {
      if ((inb(0x64) & 2) == 0)
        return;
    }
  }
}

void mouse_write(uchar word)
{
  mouse_wait(1);
  outb(0x64, 0xd4);
  mouse_wait(1);
  outb(0x60, word);
}

uint mouse_read()
{
  mouse_wait(0);
  return inb(0x60);
}

void mouseinit(void)
{
  uchar statustemp;

  mouse_wait(1);
  outb(0x64, 0xa8);

  mouse_wait(1);
  outb(0x64, 0x20);
  mouse_wait(0);
  statustemp = (inb(0x60) | 2);
  mouse_wait(0);
  outb(0x64, 0x60);
  mouse_wait(1);
  outb(0x60, statustemp);

  mouse_write(0xf6);
  mouse_read();

  mouse_write(0xf3);
  mouse_read();
  mouse_write(10);
  mouse_read();

  mouse_write(0xf4);
  mouse_read();

  initlock(&mouselock, "mouse");
  ioapicenable(IRQ_MOUSE, 0);

  count = 0;
  lastdowntick = -1000;

  mouse_x = WIDTH / 2;
  mouse_y = HEIGHT / 2;
}

void mouse_msg()
{
  struct msg m;

  if (packet.x_overflow || packet.y_overflow)
    return;
  int x = packet.x_sgn ? (0xffffff00 | (packet.x_mov & 0xff)) : (packet.x_mov & 0xff);
  int y = packet.y_sgn ? (0xffffff00 | (packet.y_mov & 0xff)) : (packet.y_mov & 0xff);
  packet.x_mov = x;
  packet.y_mov = y;

  mouse_x += x;
  mouse_y -= y;

  int btns = packet.l_btn | (packet.r_btn << 1) | (packet.m_btn << 2);

  int type = M_NONE;

  if (packet.x_mov || packet.y_mov)
  {
    if (packet.tick - lastmsgtick < 5)
      return;

    lastmsgtick = packet.tick;

    type = M_MOUSE_MOVE;
    lastdowntick = -1000;
  }
  else if (btns && !isdown)
  {
    type = M_MOUSE_DOWN;

    lastdowntick = packet.tick;
    isdown = 1;
  }
  else if (packet.tick - lastdowntick < 30 && isdown)
  {
    if (lastbtn & 1)
    {
      type = M_MOUSE_LEFT_CLICK;
    }
    else if(lastbtn & 2)
    {
      type = M_MOUSE_RIGHT_CLICK;
    }

    isdown = 0;
    lastdowntick = -1000;

    m.type = M_MOUSE_UP;
    m.mouse.x = mouse_x;
    m.mouse.y = mouse_y;
    m.mouse.button = btns;
    send_msg(&m);
  }
  else if(isdown && !btns) {
    type = M_MOUSE_UP;

    isdown = 0;
    lastdowntick = -1000;
  }
  
  lastbtn = btns;

  m.type = type;
  m.mouse.x = mouse_x;
  m.mouse.y = mouse_y;
  m.mouse.button = btns;
  m.mouse.id = lastid;

  send_msg(&m);

  if (type == M_MOUSE_DOWN)
    lastid = m.mouse.id;
}

void mouseintr(uint tick)
{
  acquire(&mouselock);
  int state;
  while (((state = inb(0x64)) & 1) == 1)
  {
    int data = inb(0x60);
    count++;

    if (recovery == 0 && (data & 255) == 0)
      recovery = 1;
    else if (recovery == 1 && (data & 255) == 0)
      recovery = 2;
    else if ((data & 255) == 12)
      recovery = 0;
    else
      recovery = -1;

    switch (count)
    {
    case 1:
      if (data & 0x08)
      {
        packet.y_overflow = (data >> 7) & 0x1;
        packet.x_overflow = (data >> 6) & 0x1;
        packet.y_sgn = (data >> 5) & 0x1;
        packet.x_sgn = (data >> 4) & 0x1;
        packet.m_btn = (data >> 2) & 0x1;
        packet.r_btn = (data >> 1) & 0x1;
        packet.l_btn = (data >> 0) & 0x1;
        break;
      }
      else
      {
        count = 0;
        break;
      }

    case 2:
      packet.x_mov = data;
      break;
    case 3:
      packet.y_mov = data;
      packet.tick = tick;
      break;
    default:
      count = 0;
      break;
    }

    if (recovery == 2)
    {
      count = 0;
      recovery = -1;
    }
    else if (count == 3)
    {
      count = 0;
      mouse_msg();
    }
  }

  release(&mouselock);
}

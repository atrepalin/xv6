#include "types.h"
#include "x86.h"
#include "defs.h"
#include "kbd.h"
#include "msg.h"

static char input[1024];
static int idx = 0;

static char prev = 0;

int getchar() {
  return input[idx++];
}

int
kbdgetc(void)
{
  static uint shift;
  static uchar *charcode[4] = {
    normalmap, shiftmap, ctlmap, ctlmap
  };
  uint st, data, c;

  st = inb(KBSTATP);
  if((st & KBS_DIB) == 0)
    return -1;
  data = inb(KBDATAP);

  if(data == 0xE0){
    shift |= E0ESC;
    return 0;
  } else if(data & 0x80){
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    c = 0;
    goto end;
  } else if(shift & E0ESC){
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }

  shift |= shiftcode[data];
  shift ^= togglecode[data];
  c = charcode[shift & (CTL | SHIFT)][data];
  if(shift & CAPSLOCK){
    if('a' <= c && c <= 'z')
      c += 'A' - 'a';
    else if('A' <= c && c <= 'Z')
      c += 'a' - 'A';
  }

end:
  if(prev != c) {
    struct msg m;
    m.type = c ? M_KEY_DOWN : M_KEY_UP;
    m.key.charcode = c;
    m.key.pressed = c > 0;
    send_msg(&m);
  }

  prev = c;

  return c;
}

void
kbdintr(void)
{
  char *c = input;

  while((*(c++) = kbdgetc()) >= 0);

  consoleintr(getchar);

  idx = 0;
}

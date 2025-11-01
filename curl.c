#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"

#include "defs.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "mmu.h"
#include "fs.h"
#include "proc.h"

static int chan;

extern struct {
  struct spinlock lock;
} ptable;

void wait_for_msg(struct proc *p) {
    acquire(&ptable.lock);

    p->chan = &chan;
    p->state = SLEEPING;

    int intena = mycpu()->intena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;

    p->chan = 0;

    release(&ptable.lock);
}

void wakeup_on_msg(struct proc *p) {
    acquire(&ptable.lock);

    if (p->state == SLEEPING && p->chan == &chan) {
        p->state = RUNNABLE;
        p->chan = 0;
    }

    release(&ptable.lock);
}

#define HTTP_BUF_SIZE 2048

struct http_ctx {
    char buffer[HTTP_BUF_SIZE];
    int len;
    int error;
    struct proc *proc;
};

static err_t recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    struct http_ctx *ctx = (struct http_ctx*)arg;

    if (!p) {
        tcp_close(tpcb); 
        wakeup_on_msg(ctx->proc);
        return ERR_OK;
    }

    int to_copy = p->len;

    if (ctx->len + to_copy > sizeof(ctx->buffer) - 1) 
        to_copy = sizeof(ctx->buffer) - 1 - ctx->len;

    memmove(ctx->buffer + ctx->len, p->payload, to_copy); 
    ctx->len += to_copy; 
    ctx->buffer[ctx->len] = '\0';

    pbuf_free(p); 
    return ERR_OK;
}

static err_t connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    struct http_ctx *ctx = (struct http_ctx*)arg;

    if (err != ERR_OK) {
        ctx->error = err;
        wakeup_on_msg(ctx->proc);
        return err;
    }

    tcp_recv(tpcb, recv_cb);

    tcp_write(tpcb, ctx->buffer, strlen(ctx->buffer), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    return ERR_OK;
}

static void error_cb(void *arg, err_t err) {
    struct http_ctx *ctx = (struct http_ctx*)arg;

    if (err != ERR_OK) {
        ctx->error = err;
        wakeup_on_msg(ctx->proc);
        return;
    }
}

int snprintf(char *buf, int size, const char *fmt, ...) {
    char *p = buf;
    char *end = buf + size - 1;
    const char *f = fmt;

    uint *argp = (uint*)(void*)(&fmt + 1);

    while (*f && p < end) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char *s = (char*)*argp++;
                if (!s) s = "(null)";
                while (*s && p < end) *p++ = *s++;
            } else if (*f == 'd') {
                int val = (int)*argp++;
                char tmp[16];
                int i = 0;

                int neg = 0;
                if (val < 0) { neg = 1; val = -val; }

                if (val == 0) tmp[i++] = '0';
                else {
                    while (val > 0) {
                        tmp[i++] = '0' + (val % 10);
                        val /= 10;
                    }
                }
                if (neg) tmp[i++] = '-';

                for (int j = i - 1; j >= 0 && p < end; j--) *p++ = tmp[j];
            } else {
                if (p < end) *p++ = '%';
                if (p < end) *p++ = *f;
            }
            f++;
        } else {
            *p++ = *f++;
        }
    }

    *p = '\0';
    return p - buf;
}

void capitalize(char* method) {
    for (int i = 0; method[i] && i < strlen(method); i++) {
        char ch = method[i];
        if (ch >= 'a' && ch <= 'z')
            ch -= 32;
        method[i] = ch;
    }
}

int sys_curl() {
    int a, b, c, d;
    int port;
    char *method;
    char *path;
    char *output;

    if (argint(0, &a) < 0) return -1;
    if (argint(1, &b) < 0) return -1;
    if (argint(2, &c) < 0) return -1;
    if (argint(3, &d) < 0) return -1;
    if (argint(4, &port) < 0) return -1;
    if (argstr(5, &method) < 0) return -1;
    if (argstr(6, &path) < 0) return -1;
    if (argptr(7, &output, HTTP_BUF_SIZE) < 0) return -1;

    capitalize(method);

    struct tcp_pcb *pcb = tcp_new();
    ip4_addr_t server;
    IP4_ADDR(&server, a, b, c, d);

    struct http_ctx *ctx = (struct http_ctx*)kalloc();
    if (!ctx) return -1;

    struct proc *p = myproc();

    ctx->len = 0;
    ctx->proc = p;
    ctx->error = ERR_OK;
    snprintf(ctx->buffer, sizeof(ctx->buffer), 
        "%s %s HTTP/1.1\r\nHost: %d.%d.%d.%d\r\n\r\n", method, path, a, b, c, d);

    tcp_arg(pcb, (void*)ctx);
    tcp_err(pcb, error_cb);
    tcp_connect(pcb, &server, port, connected_cb);

    wait_for_msg(p);

    if (copyout(p->pgdir, (uint)output, ctx->buffer, HTTP_BUF_SIZE) < 0)
        return -1;

    int error = ctx->error;

    kfree((char *)ctx);

    return error;
}

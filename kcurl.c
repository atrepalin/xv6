#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"

#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "mmu.h"
#include "fs.h"
#include "proc.h"
#include "http.h"

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

    tcp_recved(tpcb, p->len);

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

void capitalize(char* method) {
    for (int i = 0; method[i] && i < strlen(method); i++) {
        char ch = method[i];
        if (ch >= 'a' && ch <= 'z')
            ch -= 32;
        method[i] = ch;
    }
}

static void send(struct http_ctx *ctx, const ip_addr_t *ipaddr, const u16_t port) {
    struct tcp_pcb *pcb = tcp_new();

    tcp_arg(pcb, (void*)ctx);
    tcp_err(pcb, error_cb);
    tcp_connect(pcb, ipaddr, port, connected_cb);
}

static void dns_cb(const char *name, const ip_addr_t *ipaddr, void *arg) {
    struct http_ctx *ctx = (struct http_ctx*)arg;

    if (ipaddr == NULL) {
        ctx->error = ERR_CONN;
        wakeup_on_msg(ctx->proc);
        return;
    }

    send(ctx, ipaddr, 80);
}

int sys_curl() {
    int type;
    struct request *req;
    char *method;
    char *path;
    char *output;
    char *body;
    int body_len;
    char *content_type;

    if (argint(0, &type) < 0)   return ERR_UNKNOWN;
    if (argptr(1, (char **)&req, sizeof(struct request)) < 0) return ERR_UNKNOWN;
    if (argstr(2, &method) < 0) return ERR_UNKNOWN;
    if (argstr(3, &path) < 0)   return ERR_UNKNOWN;
    if (argptr(4, &output, HTTP_BUF_SIZE) < 0) return ERR_UNKNOWN;
    if (argptr(5, &body, 0) < 0 || !body) body = 0;
    if (argint(6, &body_len) < 0) body_len = 0;
    if (argstr(7, &content_type) < 0) content_type = 0;

    capitalize(method);

    struct http_ctx *ctx = (struct http_ctx*)kalloc();
    if (!ctx) return ERR_UNKNOWN;

    struct proc *p = myproc();
    ctx->len = 0;
    ctx->proc = p;
    ctx->error = ERR_OK;

    ip_addr_t ipaddr;

    switch (type) {
        case URL:
            ctx->len = snprintf(ctx->buffer, sizeof(ctx->buffer),
                "%s %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n",
                method, path, req->url);
            break;

        case IP:
            ctx->len = snprintf(ctx->buffer, sizeof(ctx->buffer),
                "%s %s HTTP/1.1\r\n"
                "Host: %d.%d.%d.%d\r\n"
                "Connection: close\r\n",
                method, path, req->ip.a, req->ip.b, req->ip.c, req->ip.d);
            break;
    }

    if (body && body_len > 0) {
        if (content_type && content_type[0])
            ctx->len += snprintf(ctx->buffer + ctx->len,
                                 sizeof(ctx->buffer) - ctx->len,
                                 "Content-Type: %s\r\n", content_type);

        ctx->len += snprintf(ctx->buffer + ctx->len,
                             sizeof(ctx->buffer) - ctx->len,
                             "Content-Length: %d\r\n\r\n",
                             body_len);

        int to_copy = body_len;
        if (ctx->len + to_copy > sizeof(ctx->buffer) - 1)
            to_copy = sizeof(ctx->buffer) - 1 - ctx->len;

        memmove(ctx->buffer + ctx->len, body, to_copy);
        ctx->len += to_copy;
        ctx->buffer[ctx->len] = '\0';
    } else {
        ctx->len += snprintf(ctx->buffer + ctx->len,
                             sizeof(ctx->buffer) - ctx->len,
                             "\r\n");
    }

    ctx->len = 0;

    switch (type) {
        case URL:
            if (dns_gethostbyname(req->url, &ipaddr, dns_cb, ctx) == ERR_OK) {
                dns_cb(req->url, &ipaddr, ctx);
            }
            break;
        case IP:
            IP4_ADDR(&ipaddr, req->ip.a, req->ip.b, req->ip.c, req->ip.d);
            send(ctx, &ipaddr, req->ip.port);
            break;
    }

    wait_for_msg(p);

    if (copyout(p->pgdir, (uint)output, ctx->buffer, HTTP_BUF_SIZE) < 0)
        return ERR_UNKNOWN;

    int error = ctx->error;
    kfree((char *)ctx);
    return error;
}

#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"

#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "fs.h"
#include "proc.h"
#include "http.h"
#include "kserver.h"
#include "e1000.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

#define CHUNK_SIZE TCP_MSS

struct connection_state {
    char* data;
    char* start;
    size_t len;
    size_t to_send;
};

static err_t
send_data(struct tcp_pcb *tpcb, struct connection_state *state)
{
    err_t err;

    while (state->len > 0) {
        u16_t chunk = min(state->len, CHUNK_SIZE);

        if (chunk == 0) {
            return ERR_MEM;
        }

        err = tcp_write(tpcb, state->data, chunk, TCP_WRITE_FLAG_COPY);

        if (err != ERR_OK) {
            return err;
        }

        state->data += chunk;
        state->len -= chunk;
    }

    tcp_output(tpcb);
    return ERR_OK;
}

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct connection_state *s = arg;

    s->to_send -= len;

    if (s->to_send <= 0) {
        kfree_nsize(s->start, HTTP_BUF_SIZE);
        kmfree(s);

        tcp_close(tpcb);

        return ERR_OK;
    }

    if (s->len > 0) {
        send_data(tpcb, s);
    }

    return ERR_OK;
}

char* strstr(char *haystack, char *needle) {
    int nlen = strlen(needle);
    int hlen = strlen(haystack);
    for (int i = 0; i < hlen - nlen + 1; i++) {
        if (haystack[i] == needle[0] && memcmp(haystack + i, needle, nlen) == 0)
            return haystack + i;
    }
    return 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (tolower(s1[i]) != tolower(s2[i]))
            return 1;
    }
    return 0;
}

char* strcasestr(char *haystack, char *needle) {
    int nlen = strlen(needle);
    int hlen = strlen(haystack);
    for (int i = 0; i < hlen - nlen + 1; i++) {
        if (haystack[i] == needle[0] && strncasecmp(haystack + i, needle, nlen) == 0)
            return haystack + i;
    }
    return 0;
}

int atoi(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    struct proc *proc = (struct proc*)arg;

    if (!p) {
        return ERR_OK;
    }

    tcp_recved(tpcb, p->len);

    acquire(&proc->rq->lock);

    struct incoming_http_request *req = &proc->rq->requests[proc->rq->tail];

    if (!req->used) {
        req->pcb = (uint32_t)tpcb;
        req->expected_len = -1;
        req->used = 1;
    }

    int to_copy = p->len;

    if (req->total_len + to_copy > sizeof(req->request) - 1) 
        to_copy = sizeof(req->request) - 1 - req->total_len;

    memmove(req->request + req->total_len, p->payload, to_copy); 
    req->total_len += to_copy; 
    req->request[req->total_len] = '\0';

    if (!req->headers_done) {
        char *end_of_headers = strstr(req->request, "\r\n\r\n");
        if (end_of_headers) {
            req->headers_done = 1;

            char *cl = strcasestr(req->request, "Content-Length:");
            if (cl) {
                cl += strlen("Content-Length:");
                while (*cl == ' ') cl++;
                req->expected_len = atoi(cl);

                int headers_len = end_of_headers + 4 - req->request;
                req->received_body_len = req->total_len - headers_len;
            } else {
                req->expected_len = 0;
            }
        }
    } else if (req->expected_len >= 0) {
        char *end_of_headers = strstr(req->request, "\r\n\r\n");
        int headers_len = end_of_headers ? (end_of_headers + 4 - req->request) : 0;
        req->received_body_len = req->total_len - headers_len;
    }

    int ready = 0;

    if (req->headers_done) {
        if (req->expected_len == 0) {
            ready = 1;
        } else if (req->expected_len > 0 &&
                   req->received_body_len >= req->expected_len) {
            ready = 1;
        }
    }

    if (ready) {
        int next = (proc->rq->tail + 1) % MSG_QUEUE_SIZE;
        if (next != proc->rq->head)
            proc->rq->tail = next;

        wakeup_on_msg(proc);
    }

    release(&proc->rq->lock);
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    struct proc *proc = (struct proc*)arg;

    struct http_conn *conn = kmalloc(sizeof(struct http_conn));
    conn->pcb = newpcb;
    conn->next = proc->rq->conns;
    proc->rq->conns = conn;

    tcp_arg(newpcb, arg);
    tcp_recv(newpcb, http_recv);
    tcp_sent(newpcb, http_sent);
    return ERR_OK;
}

int 
sys_httpd_init(void) {
    int port;

    if (argint(0, &port) < 0) return ERR_UNKNOWN;

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) ERR_UNKNOWN;

    err_t err;

    struct ip4_addr ipaddr;
    uint8_t ip[4];
    e1000_get_ip(ip);

    IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);

    if ((err = tcp_bind(pcb, &ipaddr, port)) != ERR_OK) {
        return err;
    }

    struct proc *p = myproc();

    p->rq = kalloc_nsize(sizeof(struct request_queue));

    memset(p->rq, 0, sizeof(struct request_queue));

    p->rq->head = p->rq->tail = 0;

    initlock(&p->rq->lock, "httpd");

    tcp_arg(pcb, p);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, http_accept);

    p->rq->pcb = pcb;

    return ERR_OK;
}

void http_close_conn(struct http_conn *conn) {
    if (!conn || !conn->pcb)
        return;

    tcp_arg(conn->pcb, 0);
    tcp_recv(conn->pcb, 0);
    tcp_sent(conn->pcb, 0);

    tcp_close(conn->pcb);

    kmfree(conn);
}

void
httpd_stop(void) {
    struct proc *p = myproc();

    if (p->rq != 0) {
        struct http_conn *c = p->rq->conns;
        while (c) {
            struct http_conn *next = c->next;
            http_close_conn(c);
            c = next;
        }

        tcp_close(p->rq->pcb);

        if (holding(&p->rq->lock)) {
            release(&p->rq->lock);
        }
        
        kfree_nsize(p->rq, sizeof(struct request_queue));
        p->rq = 0;
    }
}

int
sys_httpd_send(void) {
    int pcb;
    int len;
    char *body;

    if (argint(0, &pcb) < 0)  return ERR_UNKNOWN;
    if (argint(1, &len) < 0)  return ERR_UNKNOWN;
    if (argstr(2, &body) < 0) return ERR_UNKNOWN;

    struct tcp_pcb *tpcb = (struct tcp_pcb*)pcb;

    struct connection_state *state = kmalloc(sizeof(struct connection_state));
        
    state->start = state->data = kalloc_nsize(HTTP_BUF_SIZE);
    strncpy(state->data, body, len);
    state->to_send = state->len = len;

    tcp_arg(tpcb, state);
    send_data(tpcb, state);

    return ERR_OK;
}

int 
sys_httpd_recv(void) {
    struct incoming_http_request *req;

    char* request;

    if (argptr(0, &request, sizeof(struct incoming_http_request)) < 0) return ERR_UNKNOWN;

    struct proc *p = myproc();

    acquire(&p->rq->lock);
    
    if (p->rq->head == p->rq->tail) {
        release(&p->rq->lock);
        wait_for_msg(p);
        acquire(&p->rq->lock);
    }

    req = &p->rq->requests[p->rq->head];
    p->rq->head = (p->rq->head + 1) % MSG_QUEUE_SIZE;

    release(&p->rq->lock);

    if (copyout(p->pgdir, (uint)request, req, sizeof(struct incoming_http_request)) < 0)
        return ERR_UNKNOWN;

    memset(req, 0, sizeof(struct incoming_http_request));

    return ERR_OK;
}
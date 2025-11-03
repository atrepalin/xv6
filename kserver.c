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

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
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

    acquire(&proc->rq.lock);

    struct incoming_http_request *req = &proc->rq.requests[proc->rq.tail];

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
        int next = (proc->rq.tail + 1) % MSG_QUEUE_SIZE;
        if (next != proc->rq.head)
            proc->rq.tail = next;

        wakeup_on_msg(proc);
    }

    release(&proc->rq.lock);
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
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

    int error;

    struct ip4_addr ipaddr;
    uint8_t ip[4];
    e1000_get_ip(ip);

    IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);

    if ((error = tcp_bind(pcb, &ipaddr, port)) != ERR_OK) {
        return error;
    }

    struct proc *p = myproc();
    p->rq.head = p->rq.tail = 0;

    initlock(&p->rq.lock, "httpd");

    tcp_arg(pcb, p);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, http_accept);

    return ERR_OK;
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

    tcp_write(tpcb, body, len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    return ERR_OK;
}

int 
sys_httpd_recv(void) {
    struct incoming_http_request *req;

    char* request;

    if (argptr(0, &request, sizeof(struct incoming_http_request)) < 0) return ERR_UNKNOWN;

    struct proc *p = myproc();

    acquire(&p->rq.lock);
    
    if (p->rq.head == p->rq.tail) {
        release(&p->rq.lock);
        wait_for_msg(p);
        acquire(&p->rq.lock);
    }

    req = &p->rq.requests[p->rq.head];
    p->rq.head = (p->rq.head + 1) % MSG_QUEUE_SIZE;

    release(&p->rq.lock);

    if (copyout(p->pgdir, (uint)request, req, sizeof(struct incoming_http_request)) < 0)
        return ERR_UNKNOWN;

    memset(req, 0, sizeof(struct incoming_http_request));

    return ERR_OK;
}
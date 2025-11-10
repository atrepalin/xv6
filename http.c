#include "types.h"
#include "http.h"
#include "user.h"

static const char *errors[] = {
  [ERR_OK]          = "Success",
  [-ERR_MEM]        = "Out of memory error",
  [-ERR_BUF]        = "Buffer error",
  [-ERR_TIMEOUT]    = "Timeout",
  [-ERR_RTE]        = "Routing problem",
  [-ERR_INPROGRESS] = "Operation in progress",
  [-ERR_VAL]        = "Illegal value",
  [-ERR_WOULDBLOCK] = "Operation would block",
  [-ERR_USE]        = "Address in use",
  [-ERR_ALREADY]    = "Already connecting",
  [-ERR_ISCONN]     = "Connection already established",
  [-ERR_CONN]       = "Not connected",
  [-ERR_IF]         = "Low-level network interface error",
  [-ERR_ABRT]       = "Connection aborted",
  [-ERR_RST]        = "Connection reset",
  [-ERR_CLSD]       = "Connection closed",
  [-ERR_ARG]        = "Illegal argument",
  [-ERR_UNKNOWN]    = "Unknown error",
};

const char* strerror(int errnum) {
  return errors[-errnum];
}

int httpd_poll(httpd_handler handler, void *arg) {
  static struct incoming_http_request req;
  static struct outgoing_http_response resp;

  int ret;
  
  if ((ret = httpd_recv(&req)) != ERR_OK) return ret;

  handler(&req, &resp, arg);

  if ((ret = httpd_send(req.pcb, resp.total_len, resp.response)) != ERR_OK) return ret;

  return ERR_OK;
}

void cgi_handler(struct incoming_http_request *req, struct outgoing_http_response *resp, void *arg) {
  const char *path = (const char*)arg;

  int p2c[2];
  int c2p[2];

  pipe(p2c);
  pipe(c2p);

  if (fork() == 0) {
    close(p2c[1]);
    close(c2p[0]);

    // stderr -> stdout
    close(stderr);
    dup(stdout);

    // stdin -> p2c
    close(stdin);
    dup(p2c[0]);
    close(p2c[0]);

    // stdout -> c2p
    close(stdout);
    dup(c2p[1]);
    close(c2p[1]);

    char *argv[] = { path, 0 };
    exec(path, argv);

    exit();
  }

  close(p2c[0]);
  close(c2p[1]);

  write(p2c[1], req->request, req->total_len);

  close(p2c[1]);

  static char response[HTTP_BUF_SIZE];

  int total = read_buffer(c2p[0], response);
  close(c2p[0]);

  wait();

  strcpy(resp->response, response);
  resp->total_len = total;
}

int read_buffer(int fd, char buf[HTTP_BUF_SIZE]) {
  int n;
  int total = 0;

  while((n = read(fd, buf + total, HTTP_BUF_SIZE - total)) > 0){
    total += n;
    if(total >= HTTP_BUF_SIZE - 1)
      break;
  }

  buf[total] = '\0';

  return total;
}
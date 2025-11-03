#include "types.h"
#include "http.h"

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

int httpd_poll(httpd_handler handler) {
  static struct incoming_http_request req;
  static struct outgoing_http_response resp;

  int ret;
  
  if ((ret = httpd_recv(&req)) != ERR_OK) return ret;

  handler(&req, &resp);

  if ((ret = httpd_send(req.pcb, resp.total_len, resp.response)) != ERR_OK) return ret;

  return ERR_OK;
}
struct request {
  union {
    const char* url;

    struct {
      uint8_t a;
      uint8_t b;
      uint8_t c;
      uint8_t d;

      uint16_t port;
    } ip;
  };
};

#define URL 0
#define IP  1

int curl(int, struct request*, const char*, const char*, char[2048],
  const char*, size_t, const char*);

#define ERR_OK          0    /* No error, everything OK. */
#define ERR_MEM        -1    /* Out of memory error. */
#define ERR_BUF        -2    /* Buffer error. */
#define ERR_TIMEOUT    -3    /* Timeout. */
#define ERR_RTE        -4    /* Routing problem. */
#define ERR_INPROGRESS -5    /* Operation in progress. */
#define ERR_VAL        -6    /* Illegal value. */
#define ERR_WOULDBLOCK -7    /* Operation would block. */
#define ERR_USE        -8    /* Address in use. */
#define ERR_ALREADY    -9    /* Already connecting. */
#define ERR_ISCONN     -10   /* Conn already established. */
#define ERR_CONN       -11   /* Not connected. */
#define ERR_IF         -12   /* Low-level netif error. */
#define ERR_ABRT       -13   /* Connection aborted. */
#define ERR_RST        -14   /* Connection reset. */
#define ERR_CLSD       -15   /* Connection closed. */
#define ERR_ARG        -16   /* Illegal argument. */
#define ERR_UNKNOWN    -17   /* Unknown error. */

static const char *errors[] = {
  [0]               = "Success",
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

const char* curl_strerror(int errnum) {
  return errors[-errnum];
}
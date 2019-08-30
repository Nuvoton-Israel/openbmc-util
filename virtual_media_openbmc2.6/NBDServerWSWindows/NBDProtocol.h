#ifndef NBDPROTOCOL_H
#define NBDPROTOCOL_H

/*
union MyUnion {
   int64_t i64;
   int32_t i32[2];
};

int64_t htonll(int64_t hostFormatInt64)
{
   MyUnion u;
   u.i64 = hostFormatInt64;
   int32_t temp = u.i32[0];
   u.i32[0] = htonl(u.i32[1]);
   u.i32[1] = htonl(temp);
   return u.i64;
}

int64_t ntohll(int64_t networkFormatInt64)
{
   MyUnion u;
   u.i64 = networkFormatInt64;
   int32_t temp = u.i32[0];
   u.i32[0] = ntohl(u.i32[1]);
   u.i32[1] = ntohl(temp);
   return u.i64;
}
*/

int64_t ntohll(int64_t networkFormatInt64);
int64_t htonll(int64_t hostFormatInt64);

//#define htonll(x) ((1==htonl(1)) ? (x) : (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
//#define ntohll(x) ((1==ntohl(1)) ? (x) : (((uint64_t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32_t)((x) >> 32)))
/*
#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))
static uint64_t ntohll(uint64_t a) {
    uint32_t lo = a & 0xffffffff;
    uint32_t hi = a >> 32U;
    lo = ntohl(lo);
    hi = ntohl(hi);
    return ((uint64_t) lo) << 32U | hi;
}
*/
struct nbd_request {
    uint32_t magic;
    uint32_t type;	/* == READ || == WRITE 	*/
    char handle[8];
    uint64_t from;
    uint32_t len;
};

/*
 * This is the reply packet that nbd-server sends back to the client after
 * it has completed an I/O request (or an error occurs).
 */
struct nbd_reply {
    uint32_t magic;
    uint32_t error;		/* 0 = ok, else error	*/
    char handle[8];		/* handle you got from request	*/
};

enum {
    NBD_CMD_READ = 0,
    NBD_CMD_WRITE = 1,
    NBD_CMD_DISC = 2,
    NBD_CMD_FLUSH = 3,
    NBD_CMD_TRIM = 4
};

/**
 * The highest value a variable of type off_t can reach. This is a signed
 * integer, so set all bits except for the leftmost one.
 **/
#define OFFT_MAX ~((off_t)1<<(sizeof(off_t)*8-1))
#define BUFSIZE ((1024*1024)+sizeof(struct nbd_reply)) /**< Size of buffer that can hold requests */
#define DIFFPAGESIZE 4096 /**< diff file uses those chunks */

#define opts_magic = 0x49484156454F5054;
#define NBD_DEFAULT_PORT	"10809"	/* Port on which named exports are served */

/* Options that the client can select to the server */
#define NBD_OPT_EXPORT_NAME	(1)	/** Client wants to select a named export (is followed by name of export) */
#define NBD_OPT_ABORT		(2)	/** Client wishes to abort negotiation */
#define NBD_OPT_LIST		(3)	/** Client request list of supported exports (not followed by data) */
#define NBD_OPT_STARTTLS    (5)     /** Client wishes to initiate TLS */

/* Replies the server can send during negotiation */
#define NBD_REP_ACK             (1)                         /* ACK a request. Data: option number to be acked */
#define NBD_REP_SERVER          (2)                         /* Reply to NBD_OPT_LIST (one of these per server; must be followed by NBD_REP_ACK to signal the end of the list */
#define NBD_REP_INFO            (3)                         /* NBD_OPT_INFO/GO. */
#define NBD_REP_FLAG_ERROR      (1 << 31)                   /* If the high bit is set, the reply is an error */
#define NBD_REP_ERR_UNSUP       (1 | NBD_REP_FLAG_ERROR)	/* Client requested an option not understood by this version of the server */
#define NBD_REP_ERR_POLICY      (2 | NBD_REP_FLAG_ERROR)	/* Client requested an option not allowed by server configuration. (e.g., the option was disabled) */
#define NBD_REP_ERR_INVALID     (3 | NBD_REP_FLAG_ERROR)	/* Client issued an invalid request */
#define NBD_REP_ERR_PLATFORM    (4 | NBD_REP_FLAG_ERROR)	/* Option not supported on this platform */
#define NBD_REP_ERR_TLS_REQD    (5 | NBD_REP_FLAG_ERROR)        /* TLS required */

/* Global flags */
#define NBD_FLAG_FIXED_NEWSTYLE (1 << 0)	/* new-style export that actually supports extending */
#define NBD_FLAG_NO_ZEROES	(1 << 1)	/* we won't send the 128 bits of zeroes if the client sends NBD_FLAG_C_NO_ZEROES */
/* Flags from client to server. Only one such option currently. */
#define NBD_FLAG_C_FIXED_NEWSTYLE NBD_FLAG_FIXED_NEWSTYLE
#define NBD_FLAG_C_NO_ZEROES	NBD_FLAG_NO_ZEROES

/* Global flags: */
#define F_OLDSTYLE 1	  /*< Allow oldstyle (port-based) exports */
#define F_LIST 2          /*< Allow clients to list the exports on a server */
#define F_NO_ZEROES 4	  /*< Do not send zeros to client */

/* values for flags field */
#define NBD_FLAG_HAS_FLAGS	(1 << 0)	/* Flags are there */
#define NBD_FLAG_READ_ONLY	(1 << 1)	/* Device is read-only */
#define NBD_FLAG_SEND_FLUSH	(1 << 2)	/* Send FLUSH */
#define NBD_FLAG_SEND_FUA	(1 << 3)	/* Send FUA (Force Unit Access) */
#define NBD_FLAG_ROTATIONAL	(1 << 4)	/* Use elevator algorithm - rotational media */
#define NBD_FLAG_SEND_TRIM	(1 << 5)	/* Send TRIM (discard) */

#define NBD_REQUEST_MAGIC 0x25609513
#define NBD_REPLY_MAGIC 0x67446698

#endif // NBDPROTOCOL_H

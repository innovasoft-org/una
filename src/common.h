#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>

/** Program name */
#define PROGRAM_NAME      "una"
/** Program's author */
#define PROGRAM_AUTHOR    "Jakub Piwowarczyk"
/** Program's version*/
#define PROGRAM_VERSION   "1.0.0.0"

#define RC_SUCCESS  (int) (0)
#define RC_FAILURE  (int) (65536)

#define MULTICAST_IP    "225.0.0.1"
#define MULTICAST_PORT  1025

/* Short option: port */
#define S_OPT_PORT          'p'
/* Long option: port */
#define L_OPT_PORT          "port"
/* Short option: group */
#define S_OPT_IP            'i'
/* Long option: group */
#define L_OPT_IP            "ip"
/** Short option buffer size*/
#define S_OPT_BUFFER_SIZE   'b'
/** Long option buffer size */
#define L_OPT_BUFFER_SIZE   "buffer-size"
/* Long option: reuse-addr */
#define L_OPT_REUSE_ADDR    "reuse-addr"
/** Long option sender*/
#define L_OPT_SENDER        "sender"
/** Long option receiver */
#define L_OPT_RECEIVER      "receiver"
/** Long option ttl */
#define L_OPT_TTL           "ttl"
/** Long option loop back */
#define L_OPT_LOOPBACK      "no-loopback"
/** Long option */
#define L_OPT_TIME_STAMPS   "time-stamps"

/** @brief Program context definition */
typedef struct program_ctx_t {
  /** IP to bind to */
  in_addr_t ip;
  /** Port number to use */
  short port;
  /** Determines if program is running */
  short is_running;
  /* Option reuse addr */
  int optval_reuse_addr;
  /* TTL */
  int ttl;
  /* Works as receiver */
  short receiver;
  /** Buffer size */
  int buffer_size;
  /** Loop back */
  int loopback;
  /** Time stamps */
  int time_stamps;
} context;

#define IS_MULTICAST(IPADDR) ( (IPADDR & 0x000000E0) == 0x000000E0 )

#endif /* __COMMON_H__ */
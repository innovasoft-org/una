#include "common.h"
#include "logger.h"
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h> /* sigaction() ... */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>	/* close */

/* Program context */
static context ctx;

/**
 * @brief Parse the command line arguments and set some global flags.
 * @param argc Number of arguments passed to program
 * @param argv Values of arguments
 */
int validate_args(int argc, char **argv) {
	int i, substr_len;
  char* substr = NULL;

	/* Set default values */
  ctx.receiver = 1;
  ctx.optval_reuse_addr = 0x00;
	ctx.ip = INADDR_ANY;
	ctx.port = MULTICAST_PORT;
  ctx.ttl = 0;
  ctx.buffer_size = 256;
  ctx.loopback = 1;
  ctx.time_stamps = 0;

  if(NULL == strstr(argv[0], PROGRAM_NAME)) {
    TOLOG(LOG_ERR,"Program name was changed to %s",argv[0]);
    return RC_FAILURE;
  }

	/* Parse arguments */
	for(i = 1; i < argc; ++i)
	{
    /* Single sash options */
		if(argv[i][0] == '-' && argv[i][1] != '-') {
			switch ( tolower(argv[i][1]) ) {
				case S_OPT_IP:
					if (strlen(argv[i]) > 3) {
						ctx.ip = inet_addr( &argv[i][3] );
          }
					break;
				case S_OPT_PORT:
					if (strlen(argv[i]) > 3) {
						ctx.port = atoi( &argv[i][3] );
          }
					break;
        case S_OPT_BUFFER_SIZE:
        	if (strlen(argv[i]) > 3) {
						ctx.buffer_size = inet_addr( &argv[i][3] );
          }
					break;
				default:
					return RC_FAILURE;
			}
		}
    /* Double dash options */
    else if(argv[i][0] == '-' && argv[i][1] == '-') {
      substr = &argv[i][2];
      substr_len = strlen( substr );
      if( 0 == substr_len ) {
        return RC_FAILURE;
      }
      else if( substr_len == strlen(L_OPT_REUSE_ADDR) && NULL!=strstr( substr, L_OPT_REUSE_ADDR) ) {
        ctx.optval_reuse_addr = 1;
      }
      else if( substr_len > strlen(L_OPT_PORT) && NULL!=strstr( substr, L_OPT_PORT) ) {
        ctx.port = atoi( &substr[strlen(L_OPT_PORT)+1] );
      }
      else if( substr_len > strlen(L_OPT_IP) && NULL!=strstr( substr, L_OPT_IP) ) {
        ctx.ip = inet_addr( &substr[strlen(L_OPT_IP)+1] );
      }
      else if( substr_len == strlen(L_OPT_RECEIVER) && NULL!=strstr( substr, L_OPT_RECEIVER) ) {
        ctx.receiver = 1;
      }
      else if( substr_len == strlen(L_OPT_SENDER) && NULL!=strstr( substr, L_OPT_SENDER) ) {
        ctx.receiver = 0;
      }
      else if( substr_len > strlen(L_OPT_TTL) && NULL!=strstr( substr, L_OPT_TTL) ) {
        ctx.ttl = atoi( &substr[strlen(L_OPT_TTL)+1] );
      }
      else if( substr_len > strlen(L_OPT_BUFFER_SIZE) && NULL!=strstr( substr, L_OPT_BUFFER_SIZE) ) {
        ctx.buffer_size = atoi( &substr[strlen(L_OPT_BUFFER_SIZE)+1] );
      }
      else if( substr_len == strlen(L_OPT_LOOPBACK) && NULL!=strstr( substr, L_OPT_LOOPBACK) ) {
        ctx.loopback = 0;
      }
      else if( substr_len == strlen(L_OPT_TIME_STAMPS) && NULL!=strstr( substr, L_OPT_TIME_STAMPS) ) {
        ctx.time_stamps = 1;
      }
      else {
        return RC_FAILURE;
      }
    }
    /* Incorrect options */
    else {
      return RC_FAILURE;
    }
	}
	return RC_SUCCESS;
}

/**
 * @brief Handler for the system signals
 * @param sig Signal value
 */
void signal_handler(int sig)
{
	switch(sig)
	{
		case SIGINT:
		case SIGTERM:
			ctx.is_running = 0x00;
			break;
		case SIGHUP:
			TOLOG(LOG_WARNING,"Received SIGHUP signal.");
			break;
		default:
			TOLOG(LOG_WARNING,"Unhandled signal %s", strsignal(sig));
			break;
	}
}

/**
 * @brief Prints available options for the program
 */
void usage(const char* program) {
	printf("usage: %s\r\n", program);
  printf(" -%c, --%s\r\n\t%s\r\n", S_OPT_PORT,        L_OPT_PORT,        "port to be used");
  printf(" -%c, --%s\r\n\t%s\r\n", S_OPT_IP,          L_OPT_IP,          "ip address to be used");
  printf("\tdefault: INADDR_ANY\r\n");
  printf(" -%c, --%s\r\n\t%s\r\n", S_OPT_BUFFER_SIZE, L_OPT_BUFFER_SIZE, "buffer size");
  printf(" --%s\r\n\t%s\r\n",                         L_OPT_REUSE_ADDR,  "turns on reuse address");
  printf(" --%s\r\n\t%s\r\n",                         L_OPT_LOOPBACK,    "disables multicast loop back");
  printf(" --%s\r\n\t%s\r\n",                         L_OPT_RECEIVER,    "operates as receiver");
  printf(" --%s\r\n\t%s\r\n",                         L_OPT_SENDER,      "operates as sender");
	printf("\r\n");
}

int main(int argc, char** argv) {
  int sock;
  uint8_t *buffer = NULL;
  size_t length, idx;
  char *str = NULL, c, time_buf[] = {"hh:mm:ss"};
  int rc, ttl, ttl_len, optval, from_len, ret, str_len;
  struct sigaction sa;
  struct ip_mreq mcast;
  struct sockaddr_in  local, from, to;

  ctx.is_running = 1;
  sock = 0;
  
  /* Initialize logger */
	logger_init( argv[0], LOG_DEBUG, stdout);

  /* Validate arguments */
  if(validate_args(argc, argv)) {
    usage(argv[0]);
    rc = RC_FAILURE;
    goto finish;
  }

  printf("NETWORK ANALYZER (c) 2023\r\n");

  if( NULL == (buffer = (unsigned char*) malloc (ctx.buffer_size ))) {
    TOLOG(LOG_CRIT, "Not enough memory");
    rc = RC_FAILURE;
    goto finish;
  }

  str_len = 2*ctx.buffer_size + ctx.buffer_size;
  if( NULL == (str = (unsigned char*) malloc ( str_len ))) {
    TOLOG(LOG_CRIT, "Not enough memory");
    rc = RC_FAILURE;
    goto finish;
  }

  printf("\r\n");
  printf("       Mode: %s\r\n", ctx.receiver ? "receiver" : "sender" );
  printf("         IP: %d.%d.%d.%d", (ctx.ip & 0xff), (ctx.ip>>8 & 0xff), (ctx.ip>>16 & 0xff), (ctx.ip>>24 & 0xff) );
  printf("%s", IS_MULTICAST(ctx.ip) ? " (multicast)\r\n" : "\r\n" );
  printf("       Port: %d\r\n", ctx.port);
  printf(" Reuse addr: %s\r\n", (ctx.optval_reuse_addr==1) ? "yes" : "no");
  printf("        TTL: %d\r\n", ctx.ttl);
  printf("Buffer size: %d\r\n", ctx.buffer_size);
  printf("  Loop back: %s\r\n", (ctx.loopback==1) ? "yes" : "no");
  printf("\r\n");

  /* Configure signal handler to stop the program on Ctrl+C pressed */
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &signal_handler;
  sigaction(SIGINT, &sa, NULL);

  /* Create the socket */
	if( -1 == (sock = socket(AF_INET, SOCK_DGRAM, 0))) {
		TOLOG(LOG_ERR, "socket(AF_INET, SOCK_DGRAM, 0)");
		rc = RC_FAILURE;
    goto finish;
	}

  if( 1 == ctx.optval_reuse_addr) {
  	/* Enable to reuse address */
    if( -1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ctx.optval_reuse_addr, sizeof(ctx.optval_reuse_addr))) {
      TOLOG(LOG_ERR,"setsockopt(...,SOL_SOCKET,SO_REUSEADDR,...)");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  if( 1 == ctx.receiver) {
    /* Bind socket to receive data */
    local.sin_family = AF_INET;
    local.sin_port = htons(ctx.port);
    if(ctx.ip == INADDR_ANY) {
      local.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if ( IS_MULTICAST(ctx.ip) ) {
      local.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
      local.sin_addr.s_addr = ctx.ip;
    }
    if( -1 == bind(sock, (struct sockaddr*) &local, sizeof(local)) ) {
      TOLOG(LOG_ERR, "bind() failed\r\n");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  if( IS_MULTICAST(ctx.ip) ) {
    printf("multicast settings...\r\n");
    /* Join the multicast group */
    mcast.imr_multiaddr.s_addr = ctx.ip;
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);
    if ( -1 == setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mcast, sizeof(mcast))) {
      TOLOG( LOG_ERR, "setsockopt(IP_ADD_MEMBERSHIP) failed");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  if( 0 != ctx.ttl) {
    /* Set the TTL */
    if (-1 == setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ctx.ttl, sizeof(ctx.ttl))) {
      TOLOG(LOG_ERR, "setsockopt(IP_MULTICAST_TTL) failed");
      rc = RC_FAILURE;
      goto finish;
    }
    
    /* Verify TTL */
    ttl_len = sizeof(ttl);
    if ( -1 == getsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, &ttl_len)) {
      TOLOG( LOG_ERR, "getsockopt(IP_MULTICAST_TTL) failed ");
      rc = RC_FAILURE;
      goto finish;
    }
    if(ttl != ctx.ttl) {
      printf("ttl = %d", ttl);
      TOLOG(LOG_ERR, "TTL verification failed");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  if(ctx.loopback) {
    /* Set loop back */
    optval = 0;
    if ( -1 == setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&optval, sizeof(optval))) {
      TOLOG( LOG_ERR, "getsockopt(IP_MULTICAST_LOOP) failed ");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  printf("Press Ctrl+c to stop\r\n");

  /* Perform the main loop */
  while(ctx.is_running) {
    if (1 == ctx.receiver) {
      /* receive the data */
      from_len = sizeof(from);
      if( -1 == (ret = recvfrom(sock, buffer, ctx.buffer_size, 0, (struct sockaddr *)&from, &from_len))) {
        TOLOG(LOG_ERR, "recvfrom() failed");
        rc = RC_FAILURE;
        break;
      }
      else if(0 == ret) {
        TOLOG(LOG_INFO, "Connection closed");
        rc = RC_SUCCESS;
        break;
      }
      htos(buffer, ret, str, str_len);
      if(ctx.time_stamps) {
        get_time(time_buf, sizeof(time_buf));
        printf("[%s] [%s] [%s]\r\n", time_buf, inet_ntoa(from.sin_addr), str);
      }
      else {
        printf("[%s] [%s]\r\n", inet_ntoa(from.sin_addr), str);
      }
    }
    else {
      to.sin_family = AF_INET;
      to.sin_port = htons(ctx.port);
      to.sin_addr.s_addr = ctx.ip;

      printf(">>");
      for( idx=0; '\n' != (c = getc(stdin)); str[idx++] = c ) { }
      str[idx] = 0;
      length = stoh(str, buffer, ctx.buffer_size);
      if( -1 == length) {
        printf("Incorrect input.\r\n");
        continue;
      }
      if( -1 == (ret = sendto(sock, buffer, length, 0, (struct sockaddr*)&to, sizeof(to)))) {
        TOLOG(LOG_ERR, "sendto() failed");
        rc = RC_FAILURE;
        break;
      }
    }
  };

  if( IS_MULTICAST(ctx.ip) ) {
    /* Leave the multicast group */
    mcast.imr_multiaddr.s_addr = ctx.ip;
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);
    if ( -1 == setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mcast, sizeof(mcast))) {
      TOLOG( LOG_ERR, "setsockopt(IP_DROP_MEMBERSHIP) failed");
      rc = RC_FAILURE;
      goto finish;
    }
  }

  /* Exit the program */
  printf("Stopped!\r\n");
  rc = RC_SUCCESS;

finish:
  /* Close the socket if necessary */
  if(sock) {
    close(sock);
  }
  if(NULL != str) {
    free(str);
  }
  if(NULL != buffer) {
    free(buffer);
  }
  return rc;
}
#include <time.h> /* time(), localtime() */
#include <stdio.h> /* sprintf() */
#include <strings.h>

#include "common.h"
#include "logger.h"

/**
 * Converts bytes to a string
 * @param in_buf Input buffer containing raw bytes
 * @param in_len Input buffer size
 * @param out_buf Buffer used to store the string
 * @param out_len Output buffer size
 * @return Returns number of converted bytes
 */
size_t htos(const unsigned char* in_buf, int in_len, char* out_buf, int out_len) {
  static const char HEX[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  size_t ret_len = out_len;

  for(;in_len > 0 && out_len-3 > 0;*out_buf++=HEX[(*in_buf>>4)&0xF],*out_buf++=HEX[(*in_buf++)&0xF],*out_buf++=' ',in_len-=1, out_len-=3) {}
  ret_len -= out_len + 1;
  *--out_buf = '\0';

  return ret_len;
}

size_t stoh(const char *in, uint8_t *out, size_t out_len) {  
  static const uint8_t map[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 0x00 - 0x07 */
    0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 0x08 - 0x0f */
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, /* 0x10 - 0x17 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  /* 0x18 - 0x1f */
  };
  uint8_t  lo, hi;
  char clo, chi;
  size_t ret_len = out_len, in_len;

  /* Fill out with zeros */
  bzero(out, out_len);

  for (; out_len > 0; ++in) {
    if( (0 == (clo = *in)) || (0 == (chi = *(in+1))) ) {
      break;
    }
    if(' ' == clo || ' ' == chi) {
      continue;
    }
    lo = (uint8_t) ((clo & 0x1f) ^ 0x10);
    hi = (uint8_t) ((chi & 0x1f) ^ 0x10);
    if( ((uint8_t) 0xff == (lo = map[lo])) || ((uint8_t) 0xff == (hi = map[hi])) ) {
      return -1;
    }
    *out++ = (uint8_t) (lo << 4) | hi;
    --out_len;
  };

  ret_len -= out_len;
  
  return ret_len;
}

/**
 * Returns the current time in a format: hh:mm:ss
 * @param buf Buffer to store formatted string
 * @param len Length of the buffer buf
 * @return Return -1 on error otherwise number of bytes in a string.
 */
int get_time(char* buf, int len)
{
  time_t  t;
  struct tm* st;

  if(buf == NULL || len < 9)
  {
    TOLOG(LOG_ERR,"Invalid argument");
    return RC_FAILURE;
  }

  /* Get the current time */
  t = time( NULL );
  st = localtime( &t );

  /* Write date to buffer in a format: hh:mm:ss */
  return sprintf(buf,"%02d:%02d:%02d", st->tm_hour, st->tm_min, st->tm_sec);
}

/**
 * Returns the current date in a format: YYYY-MM-DD
 * @param buf Buffer to store formatted string
 * @param len Size of the buffer buf
 * @return Returns -1 on error, otherwise the number of bytes in a string
 */
int get_date(char* buf, int len)
{
  time_t  t;
  struct tm* st;

  if(buf == NULL || len < 11)
  {
    return -1;
  }

  /* Get the current time */
  t = time( NULL );
  st = localtime( &t );

  /* Write date to buffer in a format: YYYY-MM-DD */
  return sprintf(buf,"%04d-%02d-%02d", 1900+st->tm_year, 1+st->tm_mon, st->tm_mday);
}

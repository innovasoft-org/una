#include <stdio.h> /* fprintf() */
#include <stdarg.h> /* va_start(), va_arg(), va_end() */
#include <syslog.h> /* syslog() */

#include "common.h"
#include "utils.h"

static const char* const level_name[] = {"EMERG", "ALERT","CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG"};
static FILE *fd;
static int SESSION_TRACKER; /* Keeps track of session */
static int max_level;
static char date[20];

int logger_init(char* progname, int _max_level, FILE* _fd)
{
  if(_max_level < LOG_EMERG || !_fd)
  {
    return RC_FAILURE;
  }

  max_level = _max_level;
  fd = _fd;

   if(max_level == LOG_INFO)
  {
    /* Initialize syslog */
    setlogmask( LOG_UPTO(LOG_INFO) );
    openlog(progname,LOG_PID,LOG_DAEMON);
  }

  return RC_SUCCESS;
}

int logger_clean()
{
  fclose( fd );
  return RC_SUCCESS;
}

int logger_write(int level, char* filename, int line, char *fmt,...)
{
  va_list         list;
  char            *p, *r;
  int             e;
  float           f;

  if(level > max_level)
  {
    return RC_SUCCESS;
  }

  get_date(date,11);
  date[10] = ' ';
  get_time(date+11, 9);
  fprintf(fd,"%s ",date);
  fprintf(fd,"%s ",level_name[level]);
  fprintf(fd,"%s @ %d: ",filename,line);
  va_start( list, fmt );

  for ( p = fmt ; *p ; ++p )
  {
    if ( *p != '%' )/* If simple string */
    {
      /* Simple string */
      fputc( *p,fd );
    }
    else
    {
      /* Formatted string */
      switch ( *++p )
      {
        /* string */
        case 's':
        {
          r = va_arg( list, char * );
          fprintf(fd,"%s", r);
          continue;
        }
        /* integer */
        case 'd':
        {
          e = va_arg( list, int );
          fprintf(fd,"%d", e);
          continue;
        }
        case 'f':
        {
          f = va_arg( list, double );
          fprintf(fd,"%f", f);
          continue;
        }
        default:
          fputc( *p, fd );
      }
    }
  }

  va_end( list );
  fputc( '\n', fd );
  ++SESSION_TRACKER;

  if(level <= LOG_INFO )
  {
    syslog(level,fmt);
  }


  return RC_SUCCESS;
}

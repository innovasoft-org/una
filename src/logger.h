#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <syslog.h> /* syslog(), LOG_* */
#include <stdio.h> /* FILE */

/* Using GNU levels dedicated for syslog 
LOG_EMERG = 0
LOG_ALERT = 1
LOG_CRIT = 2
LOG_ERR = 3
LOG_WARNING = 4
LOG_NOTICE = 5
LOG_INFO = 6,
LOG_DEBUG = 7
*/

int logger_init(char* progname, int max_level, FILE* dest);

int logger_clean();

int logger_write(int level, char* filename, int line, char *fmt,...);

#define TOLOG(level, ...) logger_write(level, __FILE__, __LINE__, __VA_ARGS__ )

#endif /* __LOGGER_H__ */

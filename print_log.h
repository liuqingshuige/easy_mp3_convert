#ifndef __PRINT_LOG_H__
#define __PRINT_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

// 服务器日志和格式化调试信息打印
extern int g_run_log_level;

#define RUN_LOG_FILE	"mp3_convert.log"
#define RUN_LOG_MAX_SIZE  (5*1024*1024) // 512kB 最大的日志文件大小
#define RUN_LOG_ALERT -3	/*!< Alert level */
#define RUN_LOG_CRIT  -2	/*!< Critical level */
#define RUN_LOG_ERR   -1	/*!< Error level */
#define RUN_LOG_WARN   1	/*!< Warning level */
#define RUN_LOG_NOTICE 2	/*!< Notice level */
#define RUN_LOG_INFO   3	/*!< Info level */
#define RUN_LOG_DBG    4	/*!< Debug level */

#define RUN_LOG_ALERT_PREFIX  "[%s %s:%d A] "
#define RUN_LOG_CRIT_PREFIX   "[%s %s:%d C] "
#define RUN_LOG_ERR_PREFIX    "[%s %s:%d E] "
#define RUN_LOG_WARN_PREFIX   "[%s %s:%d W] "
#define RUN_LOG_NOTICE_PREFIX "[%s %s:%d N] "
#define RUN_LOG_INFO_PREFIX   "[%s %s:%d I] "
#define RUN_LOG_DBG_PREFIX    "[%s %s:%d D] "

// 可否打印该信息
#define run_is_printable(_level)  ((g_run_log_level) >= ((int)(_level)))

// level: 打印级别，取值为上面的宏定义
// stderr_or_file: 输出到标准错误还是文件，0表示标准错误，其他表示文件
void run_init_log(int level, int stderr_or_file);
// 按照format打印log到标准错误或者指定的文件
void run_log_print(const char *format, ...);
// 允许在运行时更改日志打印级别
int run_set_log_level(int level);
// 退出日志记录
void run_log_exit(void);

static inline char *run_log_time(void)
{
    static char ctime_buf[128] = {0};
    struct tm* t;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    t = localtime(&tv.tv_sec);
    snprintf(ctime_buf, sizeof(ctime_buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
        t->tm_year+1900,
        t->tm_mon+1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec,
        (int)tv.tv_usec/1000);
    return ctime_buf;
}

// "[time function:line] alert: "
#define RUN_LOG(_prefix, _fmt, ...) \
	run_log_print(_prefix _fmt, run_log_time(), \
	__FUNCTION__, __LINE__, ##__VA_ARGS__) 

// very urgent error
#define RUN_ALERT(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_ALERT)){ \
	RUN_LOG(RUN_LOG_ALERT_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_CRIT(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_CRIT)){ \
	RUN_LOG(RUN_LOG_CRIT_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_ERR(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_ERR)){ \
	RUN_LOG(RUN_LOG_ERR_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_WARN(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_WARN)){ \
	RUN_LOG(RUN_LOG_WARN_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_NOTICE(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_NOTICE)){ \
	RUN_LOG(RUN_LOG_NOTICE_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_INFO(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_INFO)){ \
	RUN_LOG(RUN_LOG_INFO_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define RUN_DBG(fmt, ...) \
	do { \
	if (run_is_printable(RUN_LOG_DBG)){ \
	RUN_LOG(RUN_LOG_DBG_PREFIX, fmt, ##__VA_ARGS__);\
	} \
	}while(0)

#define LOG RUN_INFO
#define LOGD RUN_DBG
#define LOGI RUN_INFO
#define LOGN RUN_NOTICE
#define LOGW RUN_WARN
#define LOGE RUN_ERR
#define LOGC RUN_CRIT
#define LOGA RUN_ALERT

#endif


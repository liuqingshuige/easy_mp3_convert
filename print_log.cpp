#include "print_log.h"

int g_run_log_level = RUN_LOG_INFO; // 默认信息
static FILE *g_run_log_file = 0; // 默认输出到控制台

// level: 打印级别
// stderr_or_file: 输出到标准错误还是文件，0表示标准错误，其他表示文件
void run_init_log(int level, int stderr_or_file)
{
	if (level>=RUN_LOG_ALERT && level<=RUN_LOG_DBG) // 限制范围
		g_run_log_level = level;

	if (stderr_or_file) // 如果是输出到指定文件
	{
		g_run_log_file = fopen(RUN_LOG_FILE, "r+");
		if (!g_run_log_file) // 若文件不存在
		{
			g_run_log_file = fopen(RUN_LOG_FILE, "w"); // 创建文件
			if (g_run_log_file == NULL) // 创建失败
				g_run_log_file = stderr;
			else
			{
				fclose(g_run_log_file); // 关闭
				g_run_log_file = fopen(RUN_LOG_FILE, "r+"); // 重新打开
				if (g_run_log_file == NULL) // 打开失败
					g_run_log_file = stderr;
				else
					fseek(g_run_log_file, 0, SEEK_END); // 到尾巴
			}
		}
		else
			fseek(g_run_log_file, 0, SEEK_END); // 到尾巴
	}
	else
		g_run_log_file = stderr;
}

// 允许在运行时更改日志打印级别
int run_set_log_level(int level)
{
	int old = g_run_log_level;
	if (level>=RUN_LOG_ALERT && level<=RUN_LOG_DBG) // 限制范围
		g_run_log_level = level;
	return old;
}

// 退出日志记录
void run_log_exit(void)
{
	if (g_run_log_file && g_run_log_file!=stderr)
		fclose(g_run_log_file);
	g_run_log_file = 0;
}

// 日志输出
void run_log_print(const char *format, ...)
{
	if (!g_run_log_file)
        return;

	// 获取文件大小先
	if (g_run_log_file != stderr)
	{
		int pos = ftell(g_run_log_file); // 保存当前指针位置
		if (pos > RUN_LOG_MAX_SIZE) // 已经达到最大值了
			fseek(g_run_log_file, 0, SEEK_SET); // 回到文件头，覆盖之前的
		else
			fseek(g_run_log_file, pos, SEEK_SET); // 回到之前的位置
	}

	va_list ap;
	va_start(ap, format);
	vfprintf(g_run_log_file, format, ap);
	va_end(ap);
}




#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <memory.h>
#include <zconf.h>
#include <sys/time.h>

void log_format(const char *tag, const char *message, va_list args) {
	time_t now;
	struct tm* tm_info;
	char buffer[17]; // 170825 16:23:45
	struct timeval tv;

	gettimeofday(&tv,NULL);
	now = tv.tv_sec;

//	time(&now);
	tm_info = localtime(&now);
	strftime(buffer, 17, "%y%m%d %H:%M:%S", tm_info);

	dprintf(STDOUT_FILENO, "[%s %s.%d] ", tag, buffer, (int)tv.tv_usec);
	vdprintf(STDOUT_FILENO, message, args);
	dprintf(STDOUT_FILENO, "\n");
}

void log_error(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("E", message, args);
	va_end(args);
}

void log_warn(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("W", message, args);
	va_end(args);
}

void log_info(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("I", message, args);
	va_end(args);
}

void log_debug(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("D", message, args);
	va_end(args);
}

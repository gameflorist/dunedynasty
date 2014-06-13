#ifndef ERRORLOG_H
#define ERRORLOG_H

extern void ErrorLog_Init(const char *dir);
extern void Error(const char *format, ...);
extern void Warning(const char *format, ...);

#endif

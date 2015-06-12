#ifndef _LOGGING__H
#define _LOGGING__H
#define LOG4CPP_FIX_ERROR_COLLISION 1

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Category.hh>
#include <PropertyConfigurator.hh>
#include "vrtmCommon.h"

#define LOG4CPP_FIX_ERROR_COLLISION 1
#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

#define STRINGINIZE(X) #X
#define STRINGINIZE2(X) STRINGINIZE(X)

#define JUXTAPOSE(Y, Z)  Y " : %s : " Z " "
#define JUXTAPOSE2 JUXTAPOSE( __FILE__, STRINGINIZE2(__LINE__))

//#ifdef __unix__
#define LOG_ERROR(format_str, ...) (*rootLogger).error( JUXTAPOSE2 format_str, __func__, ##__VA_ARGS__ )
#define LOG_WARN(format_str, ...) (*rootLogger).warn( JUXTAPOSE2 format_str, __func__, ##__VA_ARGS__ )
#define LOG_INFO(format_str, ...) (*rootLogger).info( JUXTAPOSE2 format_str, __func__, ##__VA_ARGS__ )
#define LOG_DEBUG(format_str, ...) (*rootLogger).debug( JUXTAPOSE2 format_str, __func__, ##__VA_ARGS__ )
#define LOG_TRACE(format_str, ...) (*rootLogger).debug( JUXTAPOSE2 format_str, __func__, ##__VA_ARGS__ )



extern log4cpp::Category* rootLogger;

bool	initLog(const char* log_properties_file, const char* instance_name);
void 	set_logger(log4cpp::Category& root);

void	closeLog();
void	PrintBytes(const char* szMsg, byte* pbData, int iSize, int col=32);

#endif

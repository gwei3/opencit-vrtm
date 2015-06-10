//
//  File: logging.h
//      John Manferdelli
//
//  Description: Debugging
//
//
//  Copyright (c) 2011, Intel Corporation. Some contributions 
//    (c) John Manferdelli.  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//    Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the disclaimer below.
//    Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the disclaimer below in the 
//      documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
//  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//



// --------------------------------------------------------------------------
#ifndef _LOGGING__H
#define _LOGGING__H
#define LOG4CPP_FIX_ERROR_COLLISION 1

#include "jlmTypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Category.hh>
#include <PropertyConfigurator.hh>

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

// --------------------------------------------------------------------

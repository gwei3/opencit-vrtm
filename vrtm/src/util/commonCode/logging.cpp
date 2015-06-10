//
//  logging.cpp
//      John Manferdelli
//
//  Description: PrintBytes
//
//  Copyright (c) 2011, Intel Corporation. All rights reserved.
//  Some contributions (c) John Manferdelli.  All rights reserved.
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "jlmTypes.h"
#include "logging.h"

// ------------------------------------------------------------------------------

log4cpp::Category* rootLogger;

bool initLog(const char* log_properties_file, const char* instance_name) {
	std::string log_props_file = std::string(log_properties_file);
	bool init_log_failed = false;
	try {
		log4cpp::PropertyConfigurator::configure(log_props_file);
		log4cpp::Category& root = log4cpp::Category::getInstance(std::string(instance_name));
		rootLogger = &root;
	}
	catch( log4cpp::ConfigureFailure& config_failure) {
		fprintf(stderr,"%s",config_failure.what());
		fprintf(stderr,"Failed to load the configuration");
		fflush(stderr);
		init_log_failed = true;
	}
	return init_log_failed;
}
void closeLog() {
	log4cpp::Category::shutdown();
}

void PrintBytes(const char* szMsg, byte* pbData, int iSize, int col)
{
    int i;
    LOG_INFO( "%s", szMsg);
    for (i= 0; i<iSize; i++) {
        LOG_INFO( "%02x", pbData[i]);
        if((i%col)==(col-1))
            LOG_INFO( "\n");
    }
}

void set_logger(log4cpp::Category& root) {
	log4cpp::Category& rootLogger = root;
}
// ------------------------------------------------------------------------------------



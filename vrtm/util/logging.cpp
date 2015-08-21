
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <stdlib.h>

#include "logging.h"

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

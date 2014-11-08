%module rpclient

%{
#define SWIG_FILE_WITH_INIT
#include "rpclient.h"
%}

int get_rpcore_decision(char* kernel_file, char* ramdisk_file);



#ifndef _HYPERVWMICALLER__H
#define _HYPERVWMICALLER__H

#ifdef _WIN32

#define VM_STATUS_CANCELLED						0
#define VM_STATUS_STARTED						1
#define VM_STATUS_STOPPED						2
#define VM_STATUS_DELETED						3

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#include <string>
#include <map>

# pragma comment(lib, "wbemuuid.lib")

int get_hyperv_vms_status(std::map<std::string, int> &);

#endif

#endif
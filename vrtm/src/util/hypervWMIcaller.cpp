#ifdef _WIN32
#include "hypervWMIcaller.h"

#ifdef __STANDALONE__
#include<stdio.h>
#include<iostream>
#define LOG_ERROR(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#include "logging.h"
#endif

int init_com_wmi(IWbemLocator **pLoc, IWbemServices **pSvc, _bstr_t & name_space) {
	HRESULT hres;
	// Initialize COM.
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		LOG_ERROR("Failed to initialize COM library. Error code = %x", hres);
		return 1;              // API call has failed.
	}

	// Initialize 
	hres = CoInitializeSecurity(
		NULL,
		-1,      // COM negotiates service                  
		NULL,    // Authentication services
		NULL,    // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,    // authentication
		RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation
		NULL,             // Authentication info 
		EOAC_NONE,        // Additional capabilities
		NULL              // Reserved
		);


	if (FAILED(hres))
	{
		LOG_ERROR("Failed to initialize security. Error code = %x", hres);
		CoUninitialize();
		return 2;          // API call has failed.
	}

	// Obtain the initial locator to Windows Management
	// on a particular host computer.


	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)pLoc);

	if (FAILED(hres))
	{
		LOG_ERROR("Failed to create IWbemLocator object. Error code = %x", hres);
		CoUninitialize();
		return 3;          // API call has failed.
	}

	// Connect to the root\virtualization\v2 namespace(namespace for hyper-v) with the
	// current user and obtain pointer pSvc
	// to make IWbemServices calls.

	hres = (*pLoc)->ConnectServer(
		name_space, // WMI namespace
		NULL,                    // User name
		NULL,                    // User password
		0,                       // Locale
		NULL,                    // Security flags                 
		0,                       // Authority       
		0,                       // Context object
		pSvc                    // IWbemServices proxy
		);

	if (FAILED(hres))
	{
		LOG_ERROR("Could not connect. Error code = %x", hres);
		(*pLoc)->Release();
		CoUninitialize();
		return 4;          // API call has failed.
	}
	char * char_str_name_space = _com_util::ConvertBSTRToString(name_space);
	LOG_INFO("Connected to %s WMI namespace", char_str_name_space);
	delete[] char_str_name_space;

	// Set security levels on the proxy -------------------------
	// Set the IWbemServices proxy so that impersonation
	// of the user (client) occurs.
	hres = CoSetProxyBlanket(

		(*pSvc),                         // the proxy to set
		RPC_C_AUTHN_WINNT,            // authentication service
		RPC_C_AUTHZ_NONE,             // authorization service
		NULL,                         // Server principal name
		RPC_C_AUTHN_LEVEL_CALL,       // authentication level
		RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
		NULL,                         // client identity 
		EOAC_NONE                     // proxy capabilities     
		);

	if (FAILED(hres))
	{
		LOG_ERROR("Could not set proxy blanket. Error code = %x", hres);
		(*pSvc)->Release();
		(*pLoc)->Release();
		CoUninitialize();
		return 5;               // API call has failed.
	}
	return 0;
}

void uninit_com_wmi(IWbemLocator *pLoc, IWbemServices *pSvc) {
	if (*(LPVOID *)&pLoc) {
		pLoc->Release();
	}
	if (*(LPVOID *)&pSvc) {
		pSvc->Release();
	}
	CoUninitialize();
}

int get_hyperv_vms_status(std::map<std::string, int> & hyperv_vms) {
	HRESULT hres;
	IWbemLocator *pLoc = 0;
	IWbemServices *pSvc = 0;
	std::map<std::string, int> vm_list;

	// Connect to the root\virtualization\v2 namespace with the
	// current user and obtain pointer pSvc
	// to make IWbemServices calls.
	_bstr_t name_space = _bstr_t(L"ROOT\\VIRTUALIZATION\\V2");
	if (init_com_wmi(&pLoc, &pSvc, name_space) > 0) {
		return 1;
	}

	// Use the IWbemServices pointer to make requests of WMI. 
	// Make requests here:
	IEnumWbemClassObject *pEnumWbemClassObject;
	hres = pSvc->ExecQuery(
		bstr_t(L"WQL"),
		bstr_t(L"select * from Msvm_ComputerSystem where Caption=\"Virtual Machine\""),
		WBEM_FLAG_FORWARD_ONLY,
		NULL,
		&pEnumWbemClassObject
		);

	if (FAILED(hres))
	{
		LOG_ERROR("Query for processes failed. Error code = %x", hres);
		uninit_com_wmi(pLoc, pSvc);
		return 1;               // API call has failed.
	}
	else
	{
		IWbemClassObject *pclsObj;
		ULONG uReturn = 0;
		bstr_t element_name;
		int enabled_state;
		char * char_element_name;
		while (pEnumWbemClassObject)
		{
			hres = pEnumWbemClassObject->Next(WBEM_INFINITE, 1,
				&pclsObj, &uReturn);
			if (0 == uReturn)
			{
				break;
			}
			VARIANT vtProp;
			// Get the value of the Name property
			pclsObj->Get(L"ElementName", 0, &vtProp, 0, 0);
			bstr_t element_name = vtProp.bstrVal;
			char_element_name = _com_util::ConvertBSTRToString(element_name);
			std::string vrtm_vm_uuid = std::string(char_element_name);
			pclsObj->Get(L"EnabledState", 0, &vtProp, 0, 0);
			enabled_state = vtProp.intVal;
			vm_list.insert(std::pair<std::string, int>(vrtm_vm_uuid, enabled_state));
			delete[] char_element_name;
			VariantClear(&vtProp);
			pclsObj->Release();
		}
	}
	LOG_INFO("Successfully fetched status of VMs with query");
	for (std::map<std::string, int>::iterator iter = hyperv_vms.begin(); iter != hyperv_vms.end(); iter++) {
		std::map<std::string, int>::iterator sack_iter = vm_list.find(iter->first);
		if (sack_iter != vm_list.end()) {
			//vm exist on hyper-v
			if (sack_iter->second == 2 || sack_iter->second == 6 || sack_iter->second == 8 || sack_iter->second == 9) {
				//VM is in enabled state
				iter->second = VM_STATUS_STARTED;
			}
			else {
				//all other state are considered to be in stopped state
				iter->second = VM_STATUS_STOPPED;
			}
		}
		else {
			//VM does not exist on hyper-v 
			iter->second = VM_STATUS_DELETED;
		}
	}
	// Cleanup
	// ========
	vm_list.clear();
	uninit_com_wmi(pLoc, pSvc);
	return 0;   // API call successfully completed.
}

#ifdef __STANDALONE__

int main(int argc, char* argv[]) {
	std::map<std::string, int> needle;
	if (argc < 2) {
		std::cout << "no VM names supplied" << std::endl << "will make dummy WMI call" << std::endl;
		std::cout << "usage: " << std::endl << "./hypervVMIcaller.exe VM_NAME1 VM_NAME2 ..." << std::endl;
		get_hyperv_vms_status(needle);
	}
	else {
		int i;
		for (i = 1; i < argc; i++) {
			needle.insert(std::pair<std::string, int>(std::string(argv[i]), -1));
		}
		if (get_hyperv_vms_status(needle) > 0) {
			std::cout << "wmi call failed" << std::endl;
			return 1;
		}
		for (std::map<std::string, int>::iterator iter = needle.begin(); iter != needle.end(); iter++) {
			std::cout << "Element name (VM name) : " << iter->first << " state : " << iter->second << std::endl;
		}

	}
	return 0;
}
#endif

#endif
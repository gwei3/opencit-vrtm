/*****************************************************************
*
* This code has been developed at:
*************************************
* Pervasive Computing Laboratory
*************************************
* Telematic Engineering Dept.
* Carlos III university
* Contact:
*		Daniel Díaz Sanchez
*		Florina Almenarez
*		Andrés Marín
*************************************		
* Mail:	dds[@_@]it.uc3m.es
* Web: http://www.it.uc3m.es/dds
* Blog: http://rubinstein.gast.it.uc3m.es/research/dds
* Team: http://www.it.uc3m.es/pervasive
**********************************************************
* This code is released under the terms of OpenSSL license
* http://www.openssl.org
*****************************************************************/
#include "x509ac_utils.h"
#include "x509ac_log.h"

#ifdef WIN32
BOOL __stdcall DllMain( HANDLE hModule, 
					   DWORD  ul_reason_for_call, 
					   LPVOID lpReserved
					   )

{
	TRACEINIT("DllMain");

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		TRACE(fname,"DLL_PROCESS_ATTACH");
		break;
	case DLL_THREAD_ATTACH:
		TRACE(fname,"DLL_THREAD_ATTACH");
		break;
	case DLL_THREAD_DETACH:
		TRACE(fname,"DLL_THREAD_DETACH");
		break;
	case DLL_PROCESS_DETACH:
		TRACE(fname,"DLL_PROCESS_DETACH");
		break;
	}
	TRACEEND(TRUE);
	return TRUE;
}

#endif

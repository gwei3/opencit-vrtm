#ifdef _WIN32
#include <basetsd.h>
#include <Windows.h>
#include <Wincrypt.h>
#include <direct.h>
#include <Shellapi.h>
#include <Processthreadsapi.h>
typedef SSIZE_T ssize_t;
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#endif
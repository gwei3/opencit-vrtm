#ifdef _WIN32
#include <basetsd.h>
#include <windows.h>
#include <Wincrypt.h>
#include <direct.h>
typedef SSIZE_T ssize_t;
//#define strdup _strdup;
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#endif
typedef unsigned u32;
#ifndef byte
typedef unsigned char byte;
#endif

#ifndef NULL
#define NULL 0
#endif

#define SHA256DIGESTBYTESIZE	32
#ifdef RSIZE_MAX_STR
#define MAX_LEN RSIZE_MAX_STR
#else
#define	MAX_LEN	4096
#endif
#define SMALL_CHAR_ARR_SIZE		128
#define MEDIUM_CHAR_ARR_SIZE	256
#define LARGE_CHAR_ARR_SIZE		512
#define VLARGE_CHAR_ARR_SIZE	2048

#ifdef _WIN32
#define snprintf	sprintf_s
#define strdup		_strdup
#define unlink		_unlink
//#define popen		_popen
#ifdef VRTMCHANNEL_BUILD
#define VRTMCHANNEL_DLLPORT __declspec (dllexport)
#else
#define VRTMCHANNEL_DLLPORT __declspec (dllimport)
#endif
#elif __linux__
#define VRTMCHANNEL_DLLPORT
#endif

int make_dir(char *filename);
int remove_dir(char *filename);
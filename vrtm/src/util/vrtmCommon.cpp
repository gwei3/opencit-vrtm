#include "win_headers.h"
#include "vrtmCommon.h"
#include "logging.h"
#include <errno.h>

int make_dir(char *dir_name) {
	int result = 0;
#ifdef _WIN32
	if (CreateDirectory((LPCSTR)dir_name, NULL) == 0) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			LOG_WARN("directory: %s already exist, failed with error : %d", dir_name, GetLastError());
		}
		else if (GetLastError() == ERROR_PATH_NOT_FOUND) {
			LOG_ERROR("Error in creating directory: %s. \n PLEASE MAKE SURE INTERMIDEIATE DIRECTORY EXIST ON MACHINE", dir_name);
			result = -1;
		}
	}
#elif __linux__
	int ret_val = mkdir(dir_name, 0766);
	if (ret_val == 0 ) {
		LOG_DEBUG("directory: %s created successfully", dir_name);
	}
	else if (ret_val < 0 && errno == EEXIST) {
		LOG_DEBUG("directory: %s already exist", dir_name);
	}
	else {
		LOG_ERROR("Failed to create the directory: %s", dir_name);
		result = -1;
	}
#endif
	return result;
}

int remove_dir(char* dir_name) {
	int result;
#ifdef _WIN32
	SHFILEOPSTRUCT fileOp = { 0 };
	fileOp.hwnd = NULL; 
	fileOp.wFunc = FO_DELETE;
	fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

	TCHAR newFrom[MAX_PATH];
	strcpy_s(newFrom, dir_name);
	newFrom[strnlen_s(dir_name, MAX_LEN) + 1] = NULL;
	fileOp.pFrom = newFrom;

	result = SHFileOperation(&fileOp);
	LOG_DEBUG("SHFileOperation return code: 0x%x", result);
#elif __linux__
	char remove_file[2048] = {'\0'};
	snprintf(remove_file, sizeof(remove_file), "rm -rf %s", dir_name);
	result = system(remove_file);
#endif
	return result;
}

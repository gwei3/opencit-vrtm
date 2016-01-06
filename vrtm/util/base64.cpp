/*
 * base64.cpp
 *
 *  Created on: 14-May-2015
 *      Author: Vijay Prakash
 */

#include <stdio.h>
#ifdef __linux__
#include <openssl/bio.h>
#include <openssl/evp.h>
#endif
#include <string.h>
#include <math.h>
#include "logging.h"
#include "win_headers.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "safe_lib.h"
#ifdef __cplusplus
}
#endif

int calcDecodeLength(const char* b64input) {
	//calculates the length of decoded base64input
	int len = strnlen_s(b64input, MAX_LEN);
	LOG_TRACE("Message Length : %d", len);
	int padding = 0;
	//TODO check the length input first
	if(len < 4) {
		return 0;
	}
	if (b64input[len - 1] == '=' && b64input[len - 2] == '=') //last two chars are =
		padding = 2;
	else if (b64input[len - 1] == '=') //last char is =
		padding = 1;	
	return (int) len * 0.75 - padding;
}

int Base64Decode(char* b64message, char** buffer) {
	
#ifdef _WIN32	
	*buffer = NULL;
	DWORD possibleLen = -1;
	if (CryptBinaryToString((BYTE *)b64message, strlen(b64message), CRYPT_STRING_BASE64, *buffer, &possibleLen) == false) {
		return 1;
	}
	LOG_DEBUG("Possible decoded length : %d", possibleLen);
	*buffer = (char *) malloc(possibleLen);
	if (*buffer == NULL) {
		LOG_ERROR("Can't allocate memory for buffer of possible encoded size");
		return 1;
	}
	memcpy(*buffer,0, possibleLen);
	if (CryptBinaryToString((BYTE *)b64message, strlen(b64message), CRYPT_STRING_BASE64, *buffer, &possibleLen))
		return 0; //success
	else 
		return 1;//error
#elif __linux__
	BIO *bio, *b64;
	if ( buffer != NULL && b64message != NULL) {
		int decodeLen = calcDecodeLength(b64message), len = 0;
		LOG_DEBUG("Possible Message length after decoding : %d", decodeLen);
		*buffer = (char*) malloc(decodeLen + 1);
		if ( *buffer != NULL) {
			if(decodeLen == 0 ) {
				*buffer[0] = '\0';
				return 0;
			}
			FILE* stream = fmemopen(b64message, strnlen_s(b64message, MAX_LEN), "r");

			b64 = BIO_new(BIO_f_base64());
			bio = BIO_new_fp(stream, BIO_NOCLOSE);
			bio = BIO_push(b64, bio);
			BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
			len = BIO_read(bio, *buffer, strnlen_s(b64message, MAX_LEN));
			if(len != decodeLen) {
				free(*buffer);
				LOG_DEBUG("Anticiipated decode len and actual decode len doesn't match");
				return 1;//error
			}
			(*buffer)[len] = '\0';

			BIO_free_all(bio);
			fclose(stream);
		}
		else {
			LOG_ERROR("Can't allocate memory for buffer");
			return 1;
		}

		return (0); //success
	}
	else {
		return 1;
	}
#endif
}

int Base64Encode(char* message, char** buffer) {

#ifdef _WIN32
	DWORD encodelen = -1;
	*buffer = NULL;
	if (CryptStringToBinary((LPCSTR )message, strlen(message), CRYPT_STRING_BASE64, (BYTE* )*buffer, &encodelen, NULL, NULL) == false) {
		return 1; //error
	}
	LOG_DEBUG("Possible encoded length : %d", encodelen);
	*buffer = (char *) malloc(encodelen);
	if( *buffer == NULL ) {
		LOG_ERROR("Can't allocate memory for buffer of possible encoded size");
		return 1;
	}
	memcpy(*buffer, 0, encodelen);
	if (CryptStringToBinary((LPCSTR )message, strlen(message), CRYPT_STRING_BASE64, (BYTE* )*buffer, &encodelen, NULL, NULL) == false) {
		return 1; //error
	}
	else {
		return 0;
	}
#elif __linux__
	  BIO *bio, *b64;
	  FILE* stream;
	  int encodedSize = 4*ceil((double)strnlen_s(message, MAX_LEN)/3);
	  LOG_DEBUG("Possible encoded length : %d", encodedSize);
	  *buffer = (char *)malloc(encodedSize+1);

	  stream = fmemopen(*buffer, encodedSize+1, "w");
	  b64 = BIO_new(BIO_f_base64());
	  bio = BIO_new_fp(stream, BIO_NOCLOSE);
	  bio = BIO_push(b64, bio);
	  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
	  BIO_write(bio, message, strnlen_s(message, MAX_LEN));
	  BIO_flush(bio);
	  BIO_free_all(bio);
	  fclose(stream);
	  return (0); //success
#endif
}

#ifdef STANDALONE
int main() {
	//Encode To Base64
	char* base64EncodeOutput;
	Base64Encode("Hello World", &base64EncodeOutput);
	printf("Output (base64): %s\n", base64EncodeOutput);

	//Decode From Base64
	char* base64DecodeOutput;
	Base64Decode(base64EncodeOutput, &base64DecodeOutput);
	printf("Output: %s\n", base64DecodeOutput);

	return(0);
}
#endif

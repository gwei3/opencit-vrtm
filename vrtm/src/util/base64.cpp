/*
 * base64.cpp
 *
 *  Created on: 14-May-2015
 *      Author: Vijay Prakash
 */

#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string.h>
#include <math.h>
#include "logging.h"

int calcDecodeLength(const char* b64input) {
	//calculates the length of decoded base64input
	int len = strlen(b64input);
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
	BIO *bio, *b64;
	int decodeLen = calcDecodeLength(b64message), len = 0;
	LOG_DEBUG("Possible Message length after decoding : %d", decodeLen);
	*buffer = (char*) malloc(decodeLen + 1);
	if ( buffer != NULL ) {
		if(decodeLen == 0 ) {
			*buffer[0] = '\0';
			return 0;
		}
		FILE* stream = fmemopen(b64message, strlen(b64message), "r");

		b64 = BIO_new(BIO_f_base64());
		bio = BIO_new_fp(stream, BIO_NOCLOSE);
		bio = BIO_push(b64, bio);
		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
		len = BIO_read(bio, *buffer, strlen(b64message));
		if(len != decodeLen) {
			free(*buffer);
			LOG_DEBUG("Anticiipated decode len and actual decode len doesn't match");
			return 1;//error
		}
		(*buffer)[len] = '\0';

		BIO_free_all(bio);
		fclose(stream);

		return (0); //success
	}
	else {
		return 1;
	}
}

int Base64Encode(char* message, char** buffer) {
	  BIO *bio, *b64;
	  FILE* stream;
	  int encodedSize = 4*ceil((double)strlen(message)/3);
	  LOG_DEBUG("Possible encoded length : %d", encodedSize);
	  *buffer = (char *)malloc(encodedSize+1);

	  stream = fmemopen(*buffer, encodedSize+1, "w");
	  b64 = BIO_new(BIO_f_base64());
	  bio = BIO_new_fp(stream, BIO_NOCLOSE);
	  bio = BIO_push(b64, bio);
	  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
	  BIO_write(bio, message, strlen(message));
	  BIO_flush(bio);
	  BIO_free_all(bio);
	  fclose(stream);
	  return (0); //success
}

#ifdef STANDALONE
int main() {
	//Encode To Base64
	char* base64EncodeOutput;
	Base64Encode("Hello Worl", &base64EncodeOutput);
	printf("Output (base64): %s\n", base64EncodeOutput);

	//Decode From Base64
	char* base64DecodeOutput;
	Base64Decode(base64EncodeOutput, &base64DecodeOutput);
	printf("Output: %s\n", base64DecodeOutput);

	return(0);
}
#endif

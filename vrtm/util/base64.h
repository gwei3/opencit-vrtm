/*
 * base64.h
 *
 *  Created on: 14-May-2015
 *      Author: vijay prakash
 */

#ifndef BASE64_H_
#define BASE64_H_


int calcDecodeLength(const char* b64input);
int Base64Decode(char* b64message, char** buffer );
int Base64Encode(char* message, char** buffer);

#endif /* BASE64_H_ */

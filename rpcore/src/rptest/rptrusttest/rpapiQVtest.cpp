
//  File:rpapiQVtest.cpp
//    Quote/Verify
//    Sujan Negi
//
//  Description: Reliance Point Trust establishment services 
//
//  Copyright (c) 2011, Intel Corporation. Some contributions 
//    (c) John Manferdelli.  All rights reserved.
//
// Use, duplication and disclosure of this file and derived works of
// this file are subject to and licensed under the Apache License dated
// January, 2004, (the "License").  This License is contained in the
// top level directory originally provided with the CloudProxy Project.
// Your right to use or distribute this file, or derived works thereof,
// is subject to your being bound by those terms and your use indicates
// consent to those terms.
//
// If you distribute this file (or portions derived therefrom), you must
// include License in or with the file and, in the event you do not include
// the entire License in the file, the file must contain a reference
// to the location of the License.


// ------------------------------------------------------------------------
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "rpapi.h"

#define TCSERVICEGETPROGHASHFROMAPP             7
#define TCSERVICEATTESTFORFROMAPP              13
#define TCSERVICEGETOSCREDSFROMAPP              5
#define TCSERVICEGETOSCERTFROMAPP              19

//Test the following
//int GenerateQuote(Data_buffer * challengeIn, Data_buffer *quoteOut);
//int VerifyQuote( Data_buffer * quoteIn,  const char* szCertificates);

void    PrintBytes(const char* szMsg, byte* pbData, int iSize, int col=32);

int readfile(char* name, char* data ){
	int rc=8000;
        FILE* fp = fopen(name, "r");
        if (fp) {
                        rc= fread( data, 1, 8000, fp);
                        fclose(fp);
			data[rc]=0;
			
                        return true;
        }else {
                printf("Error reading file %s\n", name);
                return false;
        }
}


int main(int argc, char** argv){
	
        char buf[8000] = {0};

	Data_buffer dataIn;
	Data_buffer dataOut;
	Data_buffer nonce;
        byte ncbuf[100];

	char szCert[8000] = {0};
	char szPubKey[2000] = {0};
        //char qdata[1024*4] = {0};

	unsigned int rounds = 1;
        printf("argc %d\n", argc);
	if(argc < 3) {
	  printf("Usage: rpapiQVtest pub-keyfile certfile [rounds]\n");
	  return -1;
	}

        printf("argc %d\n", argc);
	if (argc == 4) {
		rounds = atoi(argv[3]);
                printf("Will execute %d time(s)\n", rounds);
	}

	if(readfile(argv[1], szPubKey) <=0) {
	  printf("Error: could not read szPubKey file\n");
	  return -1;
	}

	printf("szPubKey read = %s\n",szPubKey);
	if(readfile(argv[2], szCert) <=0) {
		
	  printf("Error: could not read cert file\n");
	  return -1;
	}
	printf("Cert read = %s\n",szCert);

        ncbuf[0]=7;
        ncbuf[1]=6;
        nonce.data=ncbuf;
        nonce.len=2;

	//const char * indata = "The quick brown fox jumped over the lazy dog";
      	int iRet; 
        //dataIn.data= (byte *)indata;
        //dataIn.len= strlen(indata);

        dataIn.data= (byte *)szPubKey;
        dataIn.len= strlen(szPubKey);
        dataOut.data= (byte *)buf;
        dataOut.len= 8000;
	
	printf("skPubKey len = %d\n", dataIn.len);

	while (rounds ) {
        	ncbuf[0]++;
		rounds--;
		fprintf(stdout, "Calling GenerateQuote... \n");
		iRet = GenerateQuote(&nonce, &dataIn, &dataOut);
		if (iRet <= 0 ) {
			fprintf(stdout, "GenerateQuote failed\n");
			return -1;
		} else {
            		printf("dataOut.len = %d\n",dataOut.len);
			int len = strlen((char*)dataOut.data);
            		printf("Quote len = %d\n",len);
            		//PrintBytes(" GenerateQuote: ", (byte*)buf, dataOut.len );
#ifdef TEST
            		printf("dataOut.data = %s\n",dataOut.data);
#endif
            		printf("GenerateQuote returns success\n");
		}

		dataOut.len = strlen((char *)dataOut.data);
           	//printf("Quoted data len = %d\n", dataOut.len);
           	printf("Now will verify the quote...\n");
		iRet = VerifyQuote(&dataOut, (const char*)&szCert, 0);
		if (iRet <= 0 ) {
			fprintf(stdout, "VerifyQuote failed\n");
			return -1;
		} else {
			fprintf(stdout, "VerifyQuote Success\n");
		}
	}

	return 0;
}


//  File:rpapiVtest.cpp
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

	Data_buffer dataQuote;
	Data_buffer dataCert;

	char szCert[8000] = {0};
	char szQuote[8000] = {0};
        //char qdata[1024*4] = {0};

	unsigned int rounds = 1;
        printf("argc %d\n", argc);
	if(argc < 3) {
	  printf("Usage: rpapiQVtest qfile certfile [rounds]\n");
	  return -1;
	}

        printf("argc %d\n", argc);
	if (argc == 4) {
		rounds = atoi(argv[3]);
                printf("Will execute %d time(s)\n", rounds);
	}
      	int iRet; 

	//Seems like both quote and Cert strings are modified in VerifyQuote..
       // Check to see...
	while (rounds ) {
		rounds--;
		if(readfile(argv[1], szQuote) <=0) {
	  		printf("Error: could not read Quote file\n");
	  		return -1;
		}
#ifdef TEST
		printf("szQuote read = %s\n",szQuote);
#endif
		if(readfile(argv[2], szCert) <=0) {
		
	  		printf("Error: could not read cert file\n");
	  		return -1;
		}
#ifdef TEST
		printf("Cert read = %s\n",szCert);
#endif


       		dataQuote.data= (byte *)szQuote;
       		dataQuote.len= strlen(szQuote);
       		dataCert.data= (byte *)szCert;
       		dataCert.len= strlen(szCert);
	
		printf("Quote Len = %d Cert len = %d \n",dataQuote.len,dataCert.len);
           	printf("Now will verify the quote...\n");
		//iRet = VerifyQuote(&dataQuote, (const char*)&(dataCert.data));
		iRet = VerifyQuote(&dataQuote, (const char*)szCert, 0);
		if (iRet <= 0 ) {
			fprintf(stdout, "VerifyQuote failed\n");
		} else {
			fprintf(stdout, "VerifyQuote Success\n");
		}
	}

	return 0;
}

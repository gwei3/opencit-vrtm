
//  File:rppiSUAtest.cpp
//    ATTEST SEAL UNSEAL TEST
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
#include <unistd.h>
#include "rpapi.h"

#define TCSERVICESEALFORFROMAPP                 9
#define TCSERVICEUNSEALFORFROMAPP              11
#define TCSERVICEATTESTFORFROMAPP              13

//int     Attest(Data_buffer * secretIn, Data_buffer *dataOut);
void    PrintBytes(const char* szMsg, byte* pbData, int iSize, int col=32);
int main(int argc, char** argv){
	
        char buf[1024*8] = {0};

	Data_buffer dataIn;
	Data_buffer dataOut;
	const char * indata = "Please tell the folks who am I?";
      	int iRet; 
        dataIn.data= (byte *)indata;
        dataIn.len= strlen(indata);
        dataOut.data= (byte *)buf;
        dataOut.len= 1024*8;

	unsigned int rounds = 1;

        printf("argc %d\n", argc);
	if(argc < 2)  {
		printf("Usage:rpapiSUAtest rounds\n");
                return -1;
	} else {
		rounds = atoi(argv[1]);
		printf("Will execute %d time(s)\n", rounds);
	}

	while (	 rounds ) {
		rounds--;
		iRet = Attest(&dataIn, &dataOut);
		if (iRet <= 0 ) {
			fprintf(stdout, "Attestation failed\n");
			return -1;
		} else {
			PrintBytes(" Attestation ", (byte*)buf, dataOut.len );
		}

		Data_buffer sealedData;
        	char sealed[1024] = {0};
		sealedData.data= (byte*)sealed;
		sealedData.len= 1024;
		iRet = Seal(&dataIn, &sealedData);
		if (iRet <= 0 ) {
			fprintf(stdout, "Seal failed\n");
			return -1;
		} else {
			PrintBytes(" Sealed data ", (byte*)sealed, sealedData.len);		}

        	dataOut.data= (byte *)buf;
        	dataOut.len= 1024*8;
		iRet = UnSeal(&sealedData, &dataOut);
		if (iRet <= 0 ) {
			fprintf(stdout, "Unseal failed\n");
			return -1;
		} else {
			PrintBytes(" Unsealed data ", (byte*)buf, dataOut.len );
			fprintf(stdout, "\tOriginal %s  \n\tUnsealed %s\n", indata, buf);	}

		usleep(500);
	}
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <unistd.h>
//#include "vTCIDirect.h"
#include "vTCI.h"

//Hash value in PCRs: 20 bytes
#define TPM_DIGEST_SIZE          20
typedef struct __packed {
    uint8_t     digest[TPM_DIGEST_SIZE];
} tpm_digest_t;
typedef tpm_digest_t tpm_pcr_value_t;

#define STDOUT stdout
#define BUFSIZE 4*1024

//#define BUFSIZE 12*1024
// RPMMIO
#define RPTPMDEVICE "/dev/rpmmio0"
#define g_logFile stdout
void PrintBytes(const char* szMsg, byte* pbData, int iSize, int col=32);

int main(int an, char** av)
{
    tpmStatus   oTpm;
    unsigned    u, v;
    unsigned    locality;
    u8          rgtoSeal[BUFSIZE];
    u8          rgSealed[BUFSIZE];
    u8          rgunSealed[BUFSIZE];
    u8          rgtoSign[BUFSIZE];
    u8          rgSigned[BUFSIZE];
    
    byte        rgpcr[BUFSIZE];
    byte        rgRandom[BUFSIZE];
    bool        fInitAIK= false;
    const char*       reqFile= "./HW/requestFile";
    const char*       aikBlobFile= "./HW/aikblob";
    const char*       pcaFile= "./HW/pcaBlobFile";
    const char*       aikKeyFile= "./HW/aikKeyFile.txt";
    const char*       aikCertFile= "./HW/aikCert.xml";
    char*       ownerSecret= NULL;
    
    char*       ekCertFile= NULL;
    int         i, n;
    int         size;

    fprintf(STDOUT, "TPM test\n\n");
    memset(rgtoSeal, 0, BUFSIZE);
    memset(rgSealed, 0, BUFSIZE);
    memset(rgunSealed, 0, BUFSIZE);
    memset(rgtoSign, 0, BUFSIZE);
     memset(rgSigned, 0, BUFSIZE);
    memset(rgRandom, 0, BUFSIZE);

    for(i=1;i<an;i++) {
        if(strcmp(av[i], "-initAIK")==0)
            fInitAIK= true;
        if(strcmp(av[i], "-ownerSecret")==0) {
            ownerSecret= av[++i];
        }
        if(strcmp(av[i], "-AIKBlob")==0) {
            aikBlobFile= av[++i];
        }
        if(strcmp(av[i], "-AIKCertFile")==0) {
            aikCertFile= av[++i];
        }
        if(strcmp(av[i], "-AIKeyFile")==0) {
            aikKeyFile= av[++i];
        }
        if(strcmp(av[i], "-ekCertFile")==0) {
            ekCertFile= av[++i];
        }
        if(strcmp(av[i], "-help")==0) {
                fprintf(g_logFile, "vTCIDirect.exe -initAIK -ownerSecret secret -AIKBlob blobfile -AIKCertFile file -ekCertFile file\n");
        return 0;

        }
    }
    
    if(!oTpm.initTPM()) {
        fprintf(STDOUT, "initTPM failed\n");
        return 1;
    }
    else {
        fprintf(STDOUT, "initTPM succeeded\n");
    }

	//Following function not in vTCI.cpp
	/*
    if(!oTpm.setSRKauth(NULL)) {
        fprintf(g_logFile, "can't set SRK auth\n");
        return false;
    }
    * */
	
    // tested
    
    if((n=oTpm.getRandom(16, rgRandom))>=0) {
        fprintf(STDOUT, "\ngetRandom succeeded got %d bytes\n", n);
        PrintBytes("Random bytes: ", rgRandom, 16);
    }
    else {
        fprintf(STDOUT, "\ngetRandom failed\n");
        return 1;
    }
    
    //test getCompositePCR
    byte pcrMask[3]= {0,0,0xE};
    //int numPCRs=3;
    u8 buf[60];
    unsigned bufSize=60;
    oTpm.getCompositePCR(0, pcrMask, &bufSize, buf);
    PrintBytes("getCompositePCR  ", buf, bufSize);
    
    int     numPCRs= 3;
    int     rgiPCR[24] ={17, 18, 19};
    unsigned pSize;
    
    //oTpm.getCompositePCRHash(&pSize, numPCRs, rgiPCR, buf);
    oTpm.getCompositePCRHash(&pSize, buf);
    
     // AIK
    if(aikBlobFile!=NULL) {
		fprintf(g_logFile, "getAIKKey next: aikBlobFile %s, aikCertFile %s\n\n", aikBlobFile, aikCertFile);
        if(!oTpm.getAIKKey(aikBlobFile, aikCertFile)) {
            fprintf(g_logFile, "getAIKKey failed\n\n");
        }
        else
            fprintf(g_logFile, "getAIKKey succeeded\n");

        u= BUFSIZE;
        byte    nonce[20];
        memset(nonce,0,sizeof(nonce));

        if(oTpm.quoteData(sizeof(nonce), nonce, &u, rgSigned, (byte*)&locality)) {
            fprintf(g_logFile, "\nquoteData succeeded\n");
            PrintBytes("\nData to quote\n", nonce, 20);
            PrintBytes("Quoted bytes\n", rgSigned, u);
        }
        else {
            fprintf(g_logFile, "\nquoteData failed\n");
        }
    }
    
    //Seal & unseal
    if(oTpm.sealData(BUFSIZE/32, rgtoSeal, &v, rgSealed)) {
        fprintf(g_logFile, "sealData succeeded %d bytes\n", v);
        PrintBytes("Bytes to seal\n", rgtoSeal, BUFSIZE/32);
        PrintBytes("Sealed bytes\n", rgSealed, v);
    }
    else {
        fprintf(g_logFile, "\nsealData failed\n");
        return 1;
    }
        u= BUFSIZE;
    if(oTpm.unsealData(v, rgSealed, &u, rgunSealed)) {
        fprintf(g_logFile, "unsealData succeeded\n");
        PrintBytes("Bytes to unseal\n", rgSealed, v);
        PrintBytes("Unsealed bytes\n", rgunSealed, u);
    }
    else {
        fprintf(g_logFile, "\nunsealData failed\n");
        return 1;
    }


#ifdef TPMDIRECT
    int pcrno= 17;
    size= BUFSIZE;
    if(oTpm.getPCRValue(pcrno, &size, rgpcr)) {
        fprintf(STDOUT, "\ngetPCRValue, pcr %d, succeeded\n", pcrno);
        PrintBytes("PCR contents: ", rgpcr, TPM_DIGEST_SIZE);
    }
    else {
        fprintf(STDOUT, "\ngetPCRValue succeeded got %d bytes\n", n);
    }
    memcpy(oTpm.m_rgpcrS,rgpcr,TPM_DIGEST_SIZE);
    
    // Locality
    if(oTpm.getLocality(&locality)) {
        fprintf(STDOUT, "\ngetLocality succeeded %08x\n\n", locality);
    }
    else {
        fprintf(STDOUT, "\ngetLocality failed\n");
    }
    
    int offset, ret;
    
	//fprintf(STDOUT, "set locality to 2 to extend PRC 21 next");
	//if(!oTpm.setLocality(2)){
	//	fprintf(STDOUT, "\ngetLocality failed\n");
	//}
	
	fprint(STDOUT, "use OTPM\n");
    pcrno= 23;
    size= BUFSIZE;
    if(oTpm.getPCRValue(pcrno, &size, rgpcr)) {
        fprintf(STDOUT, "\ngetPCRValue, pcr %d, succeeded\n", pcrno);
        PrintBytes("PCR contents: ", rgpcr, TPM_DIGEST_SIZE);
    }
    else { 
        fprintf(STDOUT, "\ngetRandom succeeded got %d bytes\n", n);
    }

    memcpy(&oTpm.m_rgpcrS[TPM_DIGEST_SIZE],rgpcr,TPM_DIGEST_SIZE);
    oTpm.m_rgpcrSValid= true;

#endif
  


    
    
    if(oTpm.closeTPM()) { 
        fprintf(STDOUT, "\ncloseTPM succeeded\n");
        return 1;
    }
    else {
        fprintf(STDOUT, "\ncloseTPM failed\n");
    }

    fprintf(STDOUT, "\n\nTPM test done\n");
    return 0;
}


// --------------------------------------------------------------------------



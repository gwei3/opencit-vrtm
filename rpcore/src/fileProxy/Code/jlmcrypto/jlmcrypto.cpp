//
//  jlmcrypto.cpp
//      John Manferdelli
//
//  Description: common crypto interface
//
//  Copyright (c) 2011, Intel Corporation. All rights reserved.
//  Some contributions (c) John Manferdelli.  All rights reserved.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "jlmTypes.h"
#include "jlmcrypto.h"
#include "logging.h"
#include "sha256.h"


// ------------------------------------------------------------------------------


int     iRandDev= -1;


bool initCryptoRand()
{
    iRandDev= open("/dev/urandom", O_RDONLY);
    if(iRandDev<0)
        return false;
    return true;
}


bool closeCryptoRand()
{
    if(iRandDev>=0) {
        close(iRandDev);
    }
    iRandDev= -1;
    return true;
}


bool getCryptoRandom(int iNumBits, byte* buf)
{
    int     iSize= (iNumBits+NBITSINBYTE-1)/NBITSINBYTE;

    if(iRandDev<0) {
        return false;
    }
    int iSize2= read(iRandDev, buf, iSize);
    if(iSize2==iSize) {
        return true;
    }
    fprintf(g_logFile, "getCryptoRandom returning false %d bytes instead of %d\n", 
            iSize2, iSize);
    return false;
}


static bool g_fAllCryptoInit= false;

bool initAllCrypto()
{
    extern void initBigNum();

    if(g_fAllCryptoInit)
        return true;
    // init RNG
    if(!initCryptoRand())
        return false;
    // init bignum
    initBigNum();
    g_fAllCryptoInit= true;
   return true; 
}


bool closeAllCrypto()
{
    closeCryptoRand();
    g_fAllCryptoInit= false;
    return true;
}


// -------------------------------------------------------------------------------------------


//
//  Base64 encode/decode: jmbase64.cpp
//  (c) 2007, John Manferdelli
//

//  pad character is '='

static const char* s_transChar= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char s_revTrans[80]= {
     62 /* + */,   0 /* , */,   0 /* - */,   0 /* . */,  63 /* / */,
     52 /* 0 */,  53 /* 1 */,  54 /* 2 */,  55 /* 3 */,  56 /* 4 */,
     57 /* 5 */,  58 /* 6 */,  59 /* 7 */,  60 /* 8 */,  61 /* 9 */,
      0 /* : */,   0 /* ; */,   0 /* < */,   0 /* = */,   0 /* > */,
      0 /* ? */,   0 /* @ */,   0 /* A */,   1 /* B */,   2 /* C */,
      3 /* D */,   4 /* E */,   5 /* F */,   6 /* G */,   7 /* H */,
      8 /* I */,   9 /* J */,  10 /* K */,  11 /* L */,  12 /* M */,
     13 /* N */,  14 /* O */,  15 /* P */,  16 /* Q */,  17 /* R */,
     18 /* S */,  19 /* T */,  20 /* U */,  21 /* V */,  22 /* W */,
     23 /* X */,  24 /* Y */,  25 /* Z */,   0 /* [ */,   0 /* \ */,
      0 /* ] */,   0 /* ^ */,   0 /* _ */,   0 /* ` */,  26 /* a */,
     27 /* b */,  28 /* c */,  29 /* d */,  30 /* e */,  31 /* f */,
     32 /* g */,  33 /* h */,  34 /* i */,  35 /* j */,  36 /* k */,
     37 /* l */,  38 /* m */,  39 /* n */,  40 /* o */,  41 /* p */,
     42 /* q */,  43 /* r */,  44 /* s */,  45 /* t */,  46 /* u */,
     47 /* v */,  48 /* w */,  49 /* x */,  50 /* y */,  51 /* z */
    };


inline bool whitespace(char b)
{
    return(b==' ' || b=='\t' || b=='\r' || b=='\n');
}


// ------------------------------------------------------------------------------------------

#if 1 //ONERSA

static char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
bool toBase64(int size, const byte* data, int* poutLen, char* str,  bool fDirFwd)
//int b64_encode(const unsigned char *data, int size, char **str)
{
    char *s, *p;
    int i;
    int c;
    const unsigned char *q;

    p = s = (char *) str;
   
    q = (const unsigned char *) data;
    i = 0;
    for (i = 0; i < size;) {
        c = q[i++];
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        c *= 256;
        if (i < size)
            c += q[i];
        i++;
        p[0] = base64_chars[(c & 0x00fc0000) >> 18];
        p[1] = base64_chars[(c & 0x0003f000) >> 12];
        p[2] = base64_chars[(c & 0x00000fc0) >> 6];
        p[3] = base64_chars[(c & 0x0000003f) >> 0];
        if (i > size)
            p[3] = '=';
        if (i > size + 1)
            p[2] = '=';
        p += 4;
    }
   
    *p = 0;
    *poutLen = strlen(s);
   
    return true;
}

static int
pos(char c)
{
    char *p;
    for (p = base64_chars; *p; p++)
        if (*p == c)
            return p - base64_chars;
    return -1;
}

#define DECODE_ERROR 0xffffffff

static unsigned int
token_decode(const char *token)
{
    int i;
    unsigned int val = 0;
    int marker = 0;
    if (strlen(token) < 4)
        return DECODE_ERROR;
    for (i = 0; i < 4; i++) {
        val *= 64;
        if (token[i] == '=')
            marker++;
        else if (marker > 0)
            return DECODE_ERROR;
        else
            val += pos(token[i]);
    }
    if (marker > 2)
        return DECODE_ERROR;
    return (marker << 24) | val;
}

bool fromBase64(int inLen, const char* str, int* poutLen, unsigned char* data, bool fDirFwd)
//int b64_decode(const char *str, unsigned char *data)
{
    const char *p;
    unsigned char *q;

    q = data;
   
    for (p = str; *p && (*p == '=' || strchr(base64_chars, *p)); p += 4)
    {
        unsigned int val = token_decode(p);
        unsigned int marker = (val >> 24) & 0xff;
       
        if (val == DECODE_ERROR)
            return false;
        *q++ = (val >> 16) & 0xff;
        if (marker < 2)
            *q++ = (val >> 8) & 0xff;
        if (marker < 1)
            *q++ = val & 0xff;
    }
   
    *poutLen = q - (unsigned char *) data;
    return true;
}


#else
bool toBase64(int inLen, const byte* pbIn, int* poutLen, char* szOut, bool fDirFwd)
//
//      Lengths are in characters
//
{
    int             numOut= ((inLen*4)+2)/3;
    int             i= 0;
    int             a, b, c, d;
    const byte*     pbC;

    // enough room?
    if(numOut>*poutLen)
        return false;

    if(fDirFwd) {
        pbC= pbIn+inLen-1;
        while(inLen>2) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC-1)>>4)&0xf);
            c= ((*(pbC-1)&0xf)<<2) | ((*(pbC-2)>>6)&0x3);
            d= (*(pbC-2)&0x3f);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= s_transChar[c];
            szOut[i++]= s_transChar[d];
            pbC-= 3;
            inLen-= 3;
        }
        // 8 bits left
        if(inLen==1) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC-1)>>4)&0xf);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= '=';
            szOut[i++]= '=';
        }
        // 16 bits left
        if(inLen==2) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC-1)>>4)&0xf);
            c= ((*(pbC-1)&0xf)<<2);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= s_transChar[c];
            szOut[i++]= '=';
        }
    }
    else {
        pbC= pbIn;
        while(inLen>2) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC+1)>>4)&0xf);
            c= ((*(pbC+1)&0xf)<<2) | ((*(pbC+2)>>6)&0x3);
            d= (*(pbC+2)&0x3f);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= s_transChar[c];
            szOut[i++]= s_transChar[d];
            pbC+= 3;
            inLen-= 3;
        }
        // 8 bits left
        if(inLen==1) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC+1)>>4)&0xf);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= '=';
            szOut[i++]= '=';
        }
        // 16 bits left
        if(inLen==2) {
            a= (*pbC>>2)&0x3f;
            b= ((*pbC&0x3)<<4) | ((*(pbC+1)>>4)&0xf);
            c= ((*(pbC+1)&0xf)<<2);
            szOut[i++]= s_transChar[a];
            szOut[i++]= s_transChar[b];
            szOut[i++]= s_transChar[c];
            szOut[i++]= '=';
        }
    }
    *poutLen= i;
    szOut[i++]= 0;
    return true;
}


bool fromBase64(int inLen, const char* szIn, int* poutLen, unsigned char* puOut, bool fDirFwd)
//
//      Lengths are in characters
//
{
    int             numOut= ((inLen*3)+3)/4;
    unsigned char*  puW;
    unsigned char   a,b,c,d;
    int             numLeft= inLen;

//v:
	fDirFwd = true;
	
    if(inLen>2 && *(szIn+inLen-1)=='=')
        numOut--;
    if(inLen>2 && *(szIn+inLen-2)=='=')
        numOut--;
    puW= puOut+numOut-1;

    // enough room?
    if(numOut>*poutLen) {
        return false;
    }

    while(numLeft>3) {
        while(whitespace(*szIn) && numLeft>0) {
            szIn++; numLeft--;
        }
        if(*szIn<43 || *szIn>122) {
            return false;
        }
        a= s_revTrans[*szIn-43];
        szIn++; numLeft--;
        while(whitespace(*szIn) && numLeft>0) {
            szIn++; numLeft--;
        }
        if(*szIn<43 || *szIn>122) {
            return false;
        }
        b= s_revTrans[*szIn-43];
        szIn++; numLeft--;
        while(whitespace(*szIn) && numLeft>0) {
            szIn++; numLeft--;
        }
        if(*szIn=='=') {
            if(!fDirFwd) {
                *(puOut)= (a<<2) | (b>>4);
                puOut+= 1;
            }
            else {
                *(puW)= (a<<2) | (b>>4);
                puW-= 1;
            }
            numLeft-= 2;
            continue;
        }
        if(*szIn<43 || *szIn>122) {
            return false;
        }
        c= s_revTrans[*szIn-43];
        szIn++; numLeft--;
        while(whitespace(*szIn) && numLeft>0) {
            szIn++; numLeft--;
        }
        if(*szIn=='=') {
            if(!fDirFwd) {
                *(puOut)= (a<<2) | (b>>4);
                *(puOut+1)= ((b&0xf)<<4) | (c>>2);
                puOut+= 2;
            }
            else {
                *(puW)= (a<<2) | (b>>4);
                *(puW-1)= ((b&0xf)<<4) | (c>>2);
                puW-= 2;
            }
            numLeft-= 1;
            continue;
        }
        if(*szIn<43 || *szIn>122) {
            return false;
        }
        d= s_revTrans[*szIn-43];
        szIn++; numLeft--;
        if(!fDirFwd) {
            *(puOut)= (a<<2) | (b>>4);
            *(puOut+1)= ((b&0xf)<<4) | (c>>2);
            *(puOut+2)= ((c&0x3)<<6) | d;
            puOut+= 3;
        }
        else {
            *(puW)= (a<<2) | (b>>4);
            *(puW-1)= ((b&0xf)<<4) | (c>>2);
            *(puW-2)= ((c&0x3)<<6) | d;
            puW-= 3;
        }
    }

    while(whitespace(*szIn) && numLeft>0) {
        szIn++; numLeft--;
        }
    if(numLeft>0) {
        return false;
    }

    *poutLen= numOut;
    return true;
}

#endif

bool  getBase64Rand(int iBytes, byte* puR, int* pOutSize, char* szOut)
//  Get random number and base 64 encode it
{
    if(!getCryptoRandom(iBytes*NBITSINBYTE, puR)) {
        return false;
    }
    if(!toBase64(iBytes, puR, pOutSize, szOut)) {
        fprintf(g_logFile, "Bytes: %d, base64 outputsize: %d\n", iBytes, *pOutSize);
        fprintf(g_logFile, "Can't base64 encode generated random number\n");
        return false;
    }
    return true;
}


// ------------------------------------------------------------------------------------


bool AES128CBCHMACSHA256SYMPADEncryptBlob(int insize, byte* in, int* poutsize, byte* out,
                        byte* enckey, byte* intkey)
{
    cbc     oCBC;
    int     inLeft= insize;
    byte    iv[AES128BYTEBLOCKSIZE];
    byte*   curIn= in;
    byte*   curOut= out;

#ifdef CRYPTOTEST2
    memset(out, 0, *poutsize);
    fprintf(g_logFile, "*****AES128CBCHMACSHA256SYMPADEncryptBlob. insize: %d\n", insize);
    PrintBytes( "encKey: ", enckey, AES128BYTEBLOCKSIZE);
    PrintBytes( "intKey: ", intkey, AES128BYTEBLOCKSIZE);
    PrintBytes( "input:\n", in, insize);
#endif
    // init iv
    if(!getCryptoRandom(AES128BYTEBLOCKSIZE*NBITSINBYTE, iv)) {
        fprintf(g_logFile, "Cant generate iv\n");
        return false;
    }

    // init 
    if(!oCBC.initEnc(AES128, SYMPAD, HMACSHA256, 
                     AES128BYTEKEYSIZE, enckey, AES128BYTEKEYSIZE, 
                     intkey, insize, AES128BYTEBLOCKSIZE, iv)) {
        fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncryptBlob false return 1\n");
        return false;
    }

    if(!oCBC.computeCipherLen()) {
        fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncryptBlob false return 2\n");
        return false;
    }

    // outputbuffer big enough?
    if(oCBC.m_iNumCipherBytes>*poutsize) {
        fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncryptBlob false return 3\n");
        return false;
    }
    *poutsize= oCBC.m_iNumCipherBytes;

    // first cipher block
    oCBC.firstCipherBlockOut(curOut);
    curOut+= AES128BYTEBLOCKSIZE;

    while(inLeft>AES128BYTEBLOCKSIZE) {
        oCBC.nextPlainBlockIn(curIn, curOut);
        curIn+= AES128BYTEBLOCKSIZE;
        curOut+= AES128BYTEBLOCKSIZE;
        inLeft-= AES128BYTEBLOCKSIZE;
    }

    // final block
    int n= oCBC.lastPlainBlockIn(inLeft, curIn, curOut);
    if(n<0) {
        fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADEncryptBlob false return 4\n");
        return false;
    }
#ifdef CRYPTOTEST2
    PrintBytes( "output:\n", out, *poutsize);
    fprintf(g_logFile, "\n%d, out\n", *poutsize);
#endif
    return true;
}


bool AES128CBCHMACSHA256SYMPADDecryptBlob(int insize, byte* in, int* poutsize, byte* out,
                         byte* enckey, byte* intkey)
{
    cbc     oCBC;
    int     inLeft= insize;
    byte*   curIn= in;
    byte*   curOut= out;

#ifdef CRYPTOTEST
    memset(out, 0, *poutsize);
    fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob, insize: %d\n", insize);
    PrintBytes("encKey: ", enckey, AES128BYTEBLOCKSIZE);
    PrintBytes("intKey: ", intkey, AES128BYTEBLOCKSIZE);
    PrintBytes("input:\n", in, insize);
#endif
    // init 
    if(!oCBC.initDec(AES128, SYMPAD, HMACSHA256, AES128BYTEKEYSIZE, enckey, 
                             AES128BYTEKEYSIZE, intkey, insize)) {
    	fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob: cant init decryption alg\n");
        return false;
    }

    if(!oCBC.computePlainLen()) {
    	fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob: cant compute plain text length\n");
        return false;
    }

    // outputbuffer big enough?
    if(oCBC.m_iNumPlainBytes>*poutsize) {
    	fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob: output buffer too small\n");
        return false;
    }
    *poutsize= oCBC.m_iNumPlainBytes;

    // first block
    oCBC.firstCipherBlockIn(curIn);
    inLeft-= AES128BYTEBLOCKSIZE;
    curIn+= AES128BYTEBLOCKSIZE;

    while(inLeft>(AES128BYTEBLOCKSIZE+SHA256DIGESTBYTESIZE)) {
        oCBC.nextCipherBlockIn(curIn, curOut);
        curIn+= AES128BYTEBLOCKSIZE;
        curOut+= AES128BYTEBLOCKSIZE;
        inLeft-= AES128BYTEBLOCKSIZE;
    }

    // final blocks
    int n= oCBC.lastCipherBlockIn(inLeft, curIn, curOut);
    if(n<0) {
    	fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob: bad lastCipherin %d\n", inLeft);
        return false;
    }
    *poutsize= oCBC.m_iNumPlainBytes;
#ifdef CRYPTOTEST
    PrintBytes("output:\n", out, *poutsize);
    fprintf(g_logFile, "\n%d, out\n", *poutsize);
    bool fValid= oCBC.validateMac();
    if(!fValid) 
    	fprintf(g_logFile, "AES128CBCHMACSHA256SYMPADDecryptBlob: validation failed\n");
    return fValid;
#else
    return oCBC.validateMac();
#endif
}


// -----------------------------------------------------------------------------



//  File: taoSupport.cpp
//      John Manferdelli
//  Description: Support prgrams for Tao.
//               This is the revised version after the paper
//
//  Copyright (c) 2012, John Manferdelli
//  Some contributions copyright (c) 2012, Intel Corporation
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


//#include "jlmTypes.h"
#include "logging.h"

#include "jlmcrypto.h"
#include "jlmUtility.h"
#include "modesandpadding.h"
#include "sha256.h"
#include "tao.h"
#include "bignum.h"
#include "mpFunctions.h"
#include "linuxHostsupport.h"


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>


// -------------------------------------------------------------------------


bool startMeAsMeasuredProgram(int an, char** av)
{
    int     n= 0;
asm ("int $0x3");
 /*
  *    if(!initLinuxService(DEFAULTDEVICE))
        return false;
    if(!startAppfromDeviceDriver(av[0], &n))
        return false;
    */
    return true;
}


// -------------------------------------------------------------------------


taoFiles::taoFiles ()
{
    m_storageType= 0;
    m_szdirectory= NULL;
    m_szsymFile= NULL;
    m_szprivateFile= NULL;
#ifdef CSR_REQ
    m_pem_szprivateFile= NULL;
#endif
    m_szcertFile= NULL;
    m_szAncestorEvidence= NULL;
}


taoFiles::~taoFiles ()
{
    m_storageType= 0;
    if(m_szdirectory!=NULL) {
        free(m_szdirectory);
        m_szdirectory= NULL;
    }
    if(m_szsymFile!=NULL) {
        free(m_szsymFile);
        m_szsymFile= NULL;
    }
    if(m_szprivateFile!=NULL) {
        free(m_szprivateFile);
        m_szprivateFile= NULL;
    }

#ifdef CSR_REQ
    if(m_pem_szprivateFile!=NULL) {
        free(m_pem_szprivateFile);
        m_pem_szprivateFile= NULL;
    }
#endif

    if(m_szcertFile!=NULL) {
        free(m_szcertFile);
        m_szcertFile= NULL;
    }
    if(m_szAncestorEvidence!=NULL) {
        free(m_szAncestorEvidence);
        m_szAncestorEvidence= NULL;
    }
}


bool  taoFiles::initNames(const char* directory, const char* subdirectory)
{
    char   szName[512];

    m_storageType= STORAGETYPENONE;
    if(directory==NULL)
        return false;
    if(subdirectory==NULL)
        return false;
    if(511<(strlen(directory)+strlen(subdirectory)+20))
        return false;
#ifdef TEST
    fprintf(g_logFile, "initNames(%s, %s)\n", directory, subdirectory);
#endif

    sprintf(szName,"%s/%s", directory, subdirectory);
    m_szdirectory= strdup(szName);

    sprintf(szName,"%s/%s", m_szdirectory, "symkey");
    m_szsymFile= strdup(szName);

    sprintf(szName,"%s/%s", m_szdirectory, "privatekey");
    m_szprivateFile= strdup(szName);

    sprintf(szName,"%s/%s", m_szdirectory, "cert");
    m_szcertFile= strdup(szName);

    sprintf(szName,"%s/%s", m_szdirectory, "evidence");
    m_szAncestorEvidence= strdup(szName);

#ifdef CSR_REQ
    sprintf(szName,"%s/%s", m_szdirectory, "privatekey.pem");
    m_pem_szprivateFile= strdup(szName);
#endif
 
   return true;
}


bool taoFiles::getBlobData(const char* file, bool* pValid, int* pSize, byte** ppData)
{
    struct stat statBlock;
    int         n;

    if(file==NULL ) {
        fprintf(g_logFile, "taoFiles::getBlobData failed NULL file\n");
        return false;
    }

    if(stat(file, &statBlock)<0) {
        fprintf(g_logFile, "taoFiles::getBlobData failed bad stat on %s\n", file);
        return false;
    }

    n= statBlock.st_size;
    *ppData= (byte*) malloc(n);
    if(*ppData==NULL) {
        fprintf(g_logFile, "taoFiles::getBlobData failed, can't allocate block\n");
        return false;
    }
    *pSize= n;

    if(!getBlobfromFile(file, *ppData, pSize)) {
        fprintf(g_logFile, "taoFiles::getBlobData failed, getBlobfromData\n");
        return false;
    }

    *pValid= true;
    return true;
}


bool taoFiles::putBlobData(const char* file, bool fValid, int size, byte* pData)
{
    if(!fValid)
        return false;
    if(!saveBlobtoFile(file, pData, size)) {
        fprintf(g_logFile, "taoFiles::putBlobData failed\n");
        return false;
    }
    return true;
}


#ifdef TEST
void taoFiles::printAll()
{
    fprintf(g_logFile, "taoFiles\n");
    fprintf(g_logFile, "\tType: %04x\n", m_storageType);
    fprintf(g_logFile, "\tDirectory: %s\n", m_szdirectory);
    fprintf(g_logFile, "\tSym file: %s\n", m_szsymFile);
    fprintf(g_logFile, "\tPrivate file: %s\n", m_szprivateFile);
    fprintf(g_logFile, "\tCert file: %s\n", m_szcertFile);
    fprintf(g_logFile, "\tEvidence: %s\n", m_szAncestorEvidence);
}
#endif


// --------------------------------------------------------------------------



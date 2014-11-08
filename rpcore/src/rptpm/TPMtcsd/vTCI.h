//
//      File: vTCI.h - virtual Trusted Computing Interface
//
//  Copyright (c) 2012 John Manferdelli.  All rights reserved.
//    Some contributions (c) 2012, Intel Corporation
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


// --------------------------------------------------------------------------

#ifndef _VTCI_H__
#define _VTCI_H__

#include <tss/platform.h>
#include <tss/tspi.h>
#include <tss/tss_error.h>
#include <stdint.h>

#define VTCISUCCESS             0
#define VTCIERRORBUFFTOOSMALL   1
#define VTCIUNAUTHORIZED        2
#define VTCIFUNCTIONFAILED      3

#define MAXPCRS       24

typedef unsigned char byte;
typedef unsigned char u8;
typedef uint32_t	u32;

class tpmStatus {
public:
    bool            m_fTPMInitialized;
    bool            m_fOwnerAuthSet;
    TSS_HCONTEXT    m_hContext;
    TSS_HTPM        m_hTPM;
    TSS_HPOLICY     m_hTPMPolicy;
    TSS_HKEY        m_hSRK;
    TSS_HPOLICY     m_hSRKPolicy;
    TSS_HENCDATA    m_hSealingData;
    TSS_HPCRS       m_hPCRs;
#ifdef QUOTE2_DEFINED
    TSS_HPCRS       m_hPCR2;
#endif
    TSS_HKEY        m_hSigningKey;
    TSS_HKEY        m_hEk;

    byte            m_locality;

    bool            m_fEKKeyValid;
    bool            m_faikKeyValid;

    bool            m_fPcr17Valid;
    int             m_iPcr17Len; 
    byte            m_rgPcr17Value[20];
    bool            m_fPcr18Valid;
    int             m_iPcr18Len; 
    byte            m_rgPcr18Value[20];
    bool            m_fPcr19Valid;
    int             m_iPcr19Len; 
    byte            m_rgPcr19Value[20];

    int             m_iaikmodulusLen; 
    byte            m_rgaikmodulus[256];

    int             m_iEkmodulusLen; 
    byte            m_rgekmodulus[256];

    char*           m_rgEKCert;
    char*           m_rgAIKCert;

    bool            m_fTPMAuthSet;
    char*           m_szTPMSecret;
    byte            m_rgTPMAuth[20];
    
    bool            m_fpcrSelectionValid;
    int             m_npcrs;
    byte            m_rgpcrMask[3];
    bool            m_rgpcrSValid;
    byte            m_rgpcrS[20*MAXPCRS];
    
    //HL edit: added for PCR17-19 extension
    int 			m_rgiPCR[24]; //array of PCRs that have been selected for quote,seal/unseal services
    
    tpmStatus();
    ~tpmStatus();

    bool initTPM();
    //bool setOwnerauthASCII(const char* ownerSecret);
    //bool setOwnerauth(BYTE* ownerSecret);
    bool setOwnerauth(const char* ownerSecret);
    bool setOwnerauthHexcode(const char* ownerSecret);
    bool setOwnerauthSHA1(BYTE* ownerSecret);
    
    bool setTPMauth(const char* ownerSecret);
    
    bool getRandom(unsigned size, byte* puData);
    bool getCompositePCR(u32 loc, byte* pM, unsigned* pSize, u8* buf);
    bool getCompositePCRHash( unsigned* pSize, u8* buf);
	bool selectPCRCompositeIndex(byte* pM);
	
    bool getLocality(unsigned* pOut);
    bool setLocality(u32 in);
    
    //HL: expose PCR selected
    bool getPcrSelection(int * rgiPCRs, int *pcrNum);

    bool sealData(unsigned toSealSize, byte* toSealData, 
                  unsigned* pSealedSize, byte* pSealed);
    bool unsealData(unsigned unsealedSize, byte* unsealedData, 
                    unsigned* punSealedSize, byte* punSealed);

    bool makeAIK(int numCerts, byte** rgpCerts, const char* pcaFile, 
                 const char* reqFile, const char* aikFile, const char* aikPKFile);
    bool confirmAIK(const char* ownerSecret, const char* aikBlobFile, 
                    const char* challengeFile, const char* responseFile);

    bool getAIKKey(const char* aikBlobFile, const char* aikCertFile);
    bool getEKInfo(const char* fileName, bool fgetKey);

    bool quoteData(unsigned sizequoteData, byte* toquoteData,
                   unsigned* psigSize, byte* signature, byte *locality);
    bool verifyQuote(int dataSize, byte* signedData, byte pcsMask[3],
                     byte* ppcrs, byte locality, 
                     int externalSize, byte* pbExternal,
                     bool addVer, u32 sizeversion, byte* versionInfo);
#ifdef TPMTEST

	void pcr_test(unsigned sizetoSeal, byte* tosealData);
#endif

    bool closeTPM();
};

extern tpmStatus   g_oTpm;
#endif


// --------------------------------------------------------------------------



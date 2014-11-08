// ------------------------------------------------------------------------
#ifndef _RPQUOTE_H_
#define _RPQUOTE_H_ 

#define PARAMSIZE 8192
#ifndef byte
typedef unsigned char byte;
#endif

int 	VerifyHostQuote(char* szQuoteData, char* szRoot);
int 	VerifyHostedComponentQuote(char* szQuoteData, char* szRoot);
int 	GenHostedComponentQuote(int sizenonce, byte* nonce, char* publicKeyXML, char** ppQuoteXML);


#endif


#include "tss/platform.h"
#include "tss/tspi.h"
#include "tss/tss_error.h"

#include "vTCI.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

int
main(void)
{
	    TSS_RESULT  ret;
	TSS_HCONTEXT hContext;
	TSS_HTPM hTPM;
	TSS_HPOLICY hPolicy;
	TSS_HKEY hSRK, hSealKey;
	TSS_HENCDATA hSealData;
	TSS_HPCRS hPCRs;
	BYTE wellKnownSecret[] = TSS_WELL_KNOWN_SECRET;
	BYTE rawData[128];
	UINT32 unsealedDataLength, pcrLength;
	BYTE *unsealedData, *pcrValue;
	int i;
	
	TSS_UUID        SRK_UUID= TSS_UUID_SRK;
	
	for (i = 0; i < 64; i++)
		rawData[i] = (BYTE) i+1;

	/* create context and connect to TPM */
	Tspi_Context_Create(&hContext);
	Tspi_Context_Connect(hContext, NULL);
	
	/* create empty keys and data object */
	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY, TSS_KEY_TSP_SRK, &hSRK);

	Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
										TSS_KEY_TYPE_STORAGE |
										TSS_KEY_SIZE_2048 |
										TSS_KEY_NO_AUTHORIZATION |
										TSS_KEY_NOT_MIGRATABLE,
										&hSealKey);
	
	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_ENCDATA, TSS_ENCDATA_SEAL, &hSealData);

	/* get TPM object */
	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);

	/* set up the default policy - this will apply to all objects */
	ret = Tspi_Context_GetDefaultPolicy(hContext, &hPolicy);
	ret = Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_SHA1, 20, wellKnownSecret);
	
	ret= Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSRK);
	
	ret= Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS, TSS_PCRS_STRUCT_INFO_LONG, &hPCRs);
	
	
	/* create and load the sealing key */
	ret = Tspi_Key_CreateKey(hSealKey, hSRK, 0);
	ret = Tspi_Key_LoadKey(hSealKey, hSRK);
	
	/* seal to PCR values */
	/* set the PCR values to the current values in the TPM */
	//ret = Tspi_TPM_PcrRead(hTPM, 5, &pcrLength, &pcrValue);
	//ret = Tspi_PcrComposite_SetPcrValue(hPCRs, 5, pcrLength, pcrValue);
	ret = Tspi_TPM_PcrRead(hTPM, 17, &pcrLength, &pcrValue);
	ret = Tspi_PcrComposite_SetPcrValue(hPCRs, 17, pcrLength, pcrValue);
	
	ret= Tspi_PcrComposite_SetPcrLocality(hPCRs,  TPM_LOC_ZERO);

	/* perform the seal operation */
	ret = Tspi_Data_Seal(hSealData, hSealKey, 64, rawData, hPCRs);
	/* unseal the blob */
	unsealedData = NULL;
	
	ret = Tspi_Data_Unseal(hSealData, hSealKey, &unsealedDataLength, &unsealedData);
	
	/* free memory */
	Tspi_Context_FreeMemory(hContext, unsealedData);
	/* clean up */
	Tspi_Key_UnloadKey(hSealKey);
	Tspi_Context_CloseObject(hContext, hPCRs);
	Tspi_Context_CloseObject(hContext, hSealKey);
	Tspi_Context_CloseObject(hContext, hSealData);
	/* close context */
	Tspi_Context_Close(hContext);
	return 0;
	}

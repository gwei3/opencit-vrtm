package com.intel.vrtm.client;

public enum RPCCall {
	START_APP,
	TERMINATE_APP,
	SET_UUID,
	GET_RPID,
	GET_VMMETA,
	IS_VM_VERIFIED,
	GET_VM_ATTESTATION_REPORT_PATH
}

class RPAPIIndex {
	
	public static final int VM2RP_STARTAPP = 15;
	public static final int RP2VM_STARTAPP= 16;

	public static final int VM2RP_TERMINATEAPP = 17;
	public static final int RP2VM_TERMINATEAPP = 18;


	public static final int VM2RP_SETUUID = 25;
	public static final int RP2VM_SETUUID = 26;

	
    public static final int RP2VM_GETRPID             =  35;  
    public static final int VM2RP_GETRPID             =  36;
    
	public static final int RP2VM_GETVMMETA           =  37;
    public static final int VM2RP_GETVMMETA           =  38;

    
    public static final int RP2VM_ISVMVERIFIED        =  39;
    public static final int VM2RP_ISVMVERIFIED        =  40;
    
    public static final int RP2VM_GET_VM_ATTESTATION_REPORT_PATH =  41;
    public static final int VM2RP_GET_VM_ATTESTATION_REPORT_PATH =  42;
    
}

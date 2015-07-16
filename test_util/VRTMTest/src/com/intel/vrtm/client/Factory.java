package com.intel.vrtm.client;

public class Factory {
	public static TCBuffer newTCBuffer(RPCCall rpcCall) {
		
		switch(rpcCall) {
			case START_APP:
				return new TCBuffer(RPAPIIndex.VM2RP_STARTAPP, 0);
			case TERMINATE_APP:
				return new TCBuffer(RPAPIIndex.VM2RP_TERMINATEAPP, 0);
			case SET_UUID:
				return new TCBuffer(RPAPIIndex.VM2RP_SETUUID, 0);
			case GET_RPID :
				return new TCBuffer( RPAPIIndex.VM2RP_GETRPID, 0);
			case GET_VMMETA:
				return new TCBuffer(RPAPIIndex.VM2RP_GETVMMETA, 0);
			case IS_VM_VERIFIED:
				return new TCBuffer(RPAPIIndex.VM2RP_ISVMVERIFIED, 0);
			case GET_VM_ATTESTATION_REPORT_PATH:
				return new TCBuffer(RPAPIIndex.VM2RP_GET_VM_ATTESTATION_REPORT_PATH, 0);
		}
		return null;
	}
}

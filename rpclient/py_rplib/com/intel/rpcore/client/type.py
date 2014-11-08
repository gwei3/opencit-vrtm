"""
It defines the rp_api code and status values of tcservice ( RPCore )

in current implementation we are only using RPAPICode.RP2VM_STARTAPP

"""
class ALGO:
    RSAKEYTYPE              =       2
    NOALG                   =       0
    KEYNAMEBUFSIZE          =       128
class Satus: 
    TCIOSUCCESS             =       0
    TCIOFAILED              =       1
    TCIONOSERVICE           =       2
    TCIONOMEM               =       3
    TCIONOSERVICERESOURCE   =       4
    TCIONOTPM               =       5
    
class QuoteConstant:
    QUOTEMETHODNONE                  =   "none"
    QUOTEMETHODTPM12RSA2048          =   "Quote-TPM1.2-RSA2048"    
    QUOTEMETHODTPM12RSA1024          =   "Quote-TPM1.2-RSA1024"
    QUOTEMETHODSHA256FILEHASHRSA1024 =   "Quote-Sha256FileHash-RSA1024"
    QUOTEMETHODSHA256FILEHASHRSA2048 =   "Quote-Sha256FileHash-RSA2048"


class RPAPICode:
    VM2RP_GETRPHOSTQUOTE    =        1
    RP2VM_GETRPHOSTQUOTE    =        2
      
    VM2RP_GETOSHASH         =        3
    RP2VM_GETOSHASH         =        4

    VM2RP_GETOSCREDS        =        5
    RP2VM_GETOSCREDS        =        6
    
    VM2RP_GETPROGHASH       =        7
    RP2VM_GETPROGHASH       =        8

    VM2RP_SEALFOR           =        9
    RP2VM_SEALFOR           =        10

    VM2RP_UNSEALFOR         =        11
    RP2VM_UNSEALFOR         =        12

    VM2RP_ATTESTFOR         =        13
    RP2VM_ATTESTFOR         =        14

    VM2RP_STARTAPP          =        15
    RP2VM_STARTAPP          =        16

    VM2RP_TERMINATEAPP      =        17
    RP2VM_TERMINATEAPP      =        18


    VM2RP_GETOSCERT         =        19
    RP2VM_GETOSCERT         =        20

    VM2RP_SETFILE           =        21
    RP2VM_SETFILE           =        22

    VM2RP_RPHOSTQUOTERESPONSE   =    23
    RP2VM_RPHOSTQUOTERESPONSE   =    24

    VM2RP_SETUUID               =    25
    RP2VM_SETUUID               =    26

    VM2RP_GETAIKCERT            =    31
    RP2VM_GETAIKCERT            =    32

    VM2RP_GETTPMQUOTE           =    33
    RP2VM_GETTPMQUOTE           =    34
	
    
    
    
method_name_to_rpapicode = {"foo" : RPAPICode.VM2RP_STARTAPP,
                            "get_prog_hash" : RPAPICode.VM2RP_GETPROGHASH,
                            "get_host_cert" : RPAPICode.VM2RP_GETOSCERT,
                            "attest_for" : RPAPICode.VM2RP_ATTESTFOR,
                            "set_vm_uuid" : RPAPICode.VM2RP_SETUUID,
                            "delete_vm" : RPAPICode.VM2RP_TERMINATEAPP,
			    "get_aik_cert" : RPAPICode.VM2RP_GETAIKCERT,
			    "get_tpm_quote" : RPAPICode.VM2RP_GETTPMQUOTE
                            }

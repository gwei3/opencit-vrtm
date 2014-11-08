"""
It defines the rp_api code and status values of tcservice ( RPCore )

in current implementation we are only using RPAPICode.RP2VM_STARTAPP

"""
class Satus: 
    TCIOSUCCESS             =       0
    TCIOFAILED              =       1
    TCIONOSERVICE           =       2
    TCIONOMEM               =       3
    TCIONOSERVICERESOURCE   =       4
    TCIONOTPM               =       5
    
    
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

    VM2RP_IS_MEASURED           =    27
    RP2VM_IS_MEASURED           =    28

    VM2RP_CHECK_VM_VDI          =    29
    RP2VM_CHECK_VM_VDI          =    30


#include "kvm_standalone.h"

#ifndef byte
typedef unsigned char byte;
#endif

//global variable for channel, used by the rptrust lib
int g_rpchan_fd = -1;

#define PARAMSIZE 8192

#define TCSERVICESTARTAPPFROMAPP   15

#define RPCORESERVICE_PORT 6005

int channel_open() {
    int fd = -1;

	fd =  ch_open(NULL, 0);


	if(fd < 0)
	{
        	fprintf(stdout, "Cannot connect: %s\n", strerror(errno));
        	return -1;
    }
    
  	ch_register(fd);  
    return fd;

}

void create_test_data (byte *data, int length) {
    int i;
    for (i = 0; i < length; i ++) data[i] = 'A'+(i%8);
}

static void bin2ascii(int iSize, const byte* pbData, char* szMsg)
{
    int i;
    for (i= 0; i<iSize; i++)	
			sprintf(&szMsg[2*i], "%02x", pbData[i]);
    
    szMsg[2*iSize] = '\0';
}


int main ( int argc, char** argv) 
{
	char s_rpid[64] = {0};
	
    int rpid = 0;
    
    static const int DATA_LENGTH = 8*1024;
    
    int secret_in_length = 0;
    
    int data_out_length = DATA_LENGTH*2;
        
    int secret_out_length = DATA_LENGTH;
    
    int attest_out_length = DATA_LENGTH;
    
	byte secretin[DATA_LENGTH]={0}, secretout[DATA_LENGTH]={0}, encdata[DATA_LENGTH*2]={0}, attestdata[DATA_LENGTH*2]={0};
	byte dataout[DATA_LENGTH]={0};

	char sz_buf[DATA_LENGTH] = {0};
	
	unsigned txx = 0;
	
	strcpy((char*)secretin, "the master secret");
	secret_in_length = strlen((char*)secretin);
	
    strcpy(s_rpid, getenv ("THIS_RPID"));
    rpid = atoi(s_rpid);
    printf("rpid is %s\n", s_rpid );
    if (rpid <= 999) {
        printf ("Something went wrong.\n");
        return -1;
    }
	
	g_rpchan_fd = channel_open();
	if ( g_rpchan_fd < 0)
		return -1;
		
    //create_test_data(secretin,DATA_LENGTH);
    Attest( secret_in_length, secretin, &attest_out_length, attestdata);
    
    attestdata[attest_out_length] = '\0';
    bin2ascii(attest_out_length, attestdata, sz_buf);
    printf ("Attest output : %s\n", sz_buf);
     
       
    Seal(secret_in_length, secretin, &data_out_length, encdata);
    bin2ascii(data_out_length, encdata, sz_buf);
    printf ("Seal output : %s\n", sz_buf);
    
    UnSeal(data_out_length, encdata, &secret_out_length, secretout);
    
    printf ("Unseal output compared to input secret: %d\n", memcmp(secretin,secretout,secret_out_length));
    
    getHostedComponentMeasurement(&txx, &data_out_length, dataout);
    
    bin2ascii(data_out_length, dataout, sz_buf);
    printf("Hosted Component measurement %s\n", sz_buf);
    
    getHostMeasurement(&txx, &data_out_length, dataout);
    bin2ascii(data_out_length, dataout, sz_buf);
    printf("Host measurement %s\n", sz_buf);
 	
	getHostCertChain(&data_out_length, dataout);
	printf("Certificate %s\n", dataout);
    return 0;
}


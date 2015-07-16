/*****************************************************************
*
* This code has been developed at:
*************************************
* Pervasive Computing Laboratory
*************************************
* Telematic Engineering Dept.
* Carlos III university
* Contact:
*		Daniel Díaz Sanchez
*		Florina Almenarez
*		Andrés Marín
*************************************		
* Mail:	dds[@_@]it.uc3m.es
* Web: http://www.it.uc3m.es/dds
* Blog: http://rubinstein.gast.it.uc3m.es/research/dds
* Team: http://www.it.uc3m.es/pervasive
**********************************************************
* This code is released under the terms of OpenSSL license
* http://www.openssl.org
*****************************************************************/






#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE
#	include <windows.h>
#	include <wincrypt.h>
#endif

#include <x509ac.h>
#include <x509attr.h>
#include <x509ac-supp.h>
#include <x509attr-supp.h>
#include <x509ac_utils.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <libconfig.h>



#define TEST_IETF_ATTR_SYNTAX
#define TEST_ROLE_SYNTAX


#define CONFIG_FILE "/tmp/ac_holder_config"
#define INPUT_FILE "/etc/x509ac_input"
#define ENTRIES 6
#define MAXLEN 80
#define MAXINPUTLEN 200
#define fatal(msg) fatal_error(__FILE__, __LINE__, msg)
struct sample_parameters
{
  char countryName[MAXLEN];
  char stateOrProvinceName[MAXLEN];
  char localityName[MAXLEN];
  char organizationName[MAXLEN];
  char organizationalUnitName[MAXLEN];
  char commonName[MAXLEN];
};

struct input_parameters
{
  char vm_hash[MAXINPUTLEN];
  char nonce[MAXINPUTLEN];
  char vm_uuid[MAXINPUTLEN];
  char vm_ip[MAXINPUTLEN];
  char vm_image_id[MAXINPUTLEN];
  char sign_from_rpcore[MAXINPUTLEN];
};

void init_parameters (struct sample_parameters * parms)
{
  strncpy (parms->countryName, "IN", MAXLEN);
  strncpy (parms->stateOrProvinceName, "MH", MAXLEN);
  strncpy (parms->localityName, "PUNE", MAXLEN);
  strncpy (parms->organizationName, "GSL", MAXLEN);
  strncpy (parms->organizationalUnitName, "IDM", MAXLEN);
  strncpy (parms->commonName, "GSLAB", MAXLEN);
}
char *trim (char * s)
{
  /* Initialize start, end pointers */
  char *s1 = s, *s2 = &s[strlen (s) - 1];

  /* Trim and delimit right side */
  while ( (isspace (*s2)) && (s2 >= s1) )
    s2--;
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace (*s1)) && (s1 < s2) )
    s1++;
  /* Copy finished string */
  strcpy (s, s1);
  return s;
}

void parse_config (struct sample_parameters * parms)
{
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL)
  {
    return;
  }

  /* Read next line */
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
    /* Skip blank lines and comments */
    if (buff[0] == '\n' || buff[0] == '#')
      continue;

    /* Parse name/value pair from line */
    char name[MAXLEN], value[MAXLEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, MAXLEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, MAXLEN);
    trim (value);
	
    /* Copy into correct entry in parameters struct */
    if (strcmp(name, "countryName")==0)
      strncpy (parms->countryName, value, MAXLEN);
    else if (strcmp(name, "stateOrProvinceName")==0)
      strncpy (parms->stateOrProvinceName, value, MAXLEN);
    else if (strcmp(name, "localityName")==0)
      strncpy (parms->localityName, value, MAXLEN);
    else if (strcmp(name, "organizationName")==0)
      strncpy (parms->organizationName, value, MAXLEN);
    else if (strcmp(name, "organizationalUnitName")==0)
      strncpy (parms->organizationalUnitName, value, MAXLEN);
    else if (strcmp(name, "commonName")==0)
      strncpy (parms->commonName, value, MAXLEN);
    else
      printf ("WARNING: %s/%s: Unknown name/value pair!\n",
        name, value);
  }

  /* Close file */
  fclose (fp);
}

void parse_input_file(struct input_parameters * parms)
{
    config_t cfg;               /*Returns all parameters in this structure */
    config_setting_t *setting;
    const char *str1, *str2;
    int tmp;

    /*Initialization */
    config_init(&cfg);

    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, INPUT_FILE))
    {
        //printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    /* Get the configuration file name. */
    if (config_lookup_string(&cfg, "VM_UUID", &str1)){
        printf("\nVM_UUID: %s", str1);
	strncpy (parms->vm_uuid, str1, MAXINPUTLEN);
    }
    else
        printf("\nNo 'VM_UUID' setting in configuration file- %s",INPUT_FILE);
	
    if (config_lookup_string(&cfg, "VM_HASH", &str1)){
        printf("\nVM_HASH: %s", str1);
	strncpy (parms->vm_hash, str1, MAXINPUTLEN);
    }
    else
        printf("\nNo 'VM_HASH' setting in configuration file- %s",INPUT_FILE);

    if (config_lookup_string(&cfg, "NONCE", &str1)){
        printf("\nNONCE: %s", str1);
	strncpy (parms->nonce, str1, MAXINPUTLEN);
    }	
    else
        printf("\nNo 'NONCE' setting in configuration file- %s",INPUT_FILE);

    if (config_lookup_string(&cfg, "VM_IMAGE_ID", &str1)){
        printf("\nVM_IMAGE_ID: %s", str1);
	strncpy (parms->vm_image_id, str1, MAXINPUTLEN);
    }
    else
        printf("\nNo 'VM_IMAGE_ID' setting in configuration file- %s",INPUT_FILE);
	
    if (config_lookup_string(&cfg, "VM_IP", &str1)){
        printf("\nVM_IP: %s", str1);
	strncpy (parms->vm_ip, str1, MAXINPUTLEN);
    }
    else
        printf("\nNo 'VM_IP' setting in configuration file- %s",INPUT_FILE);
	
    if (config_lookup_string(&cfg, "SIGNATURE_FROM_RPCORE", &str1)){
        printf("\nSIGNATURE_FROM_RPCORE: %s", str1);
        strncpy (parms->sign_from_rpcore, str1, MAXINPUTLEN);
    }
    else
        printf("\nNo 'SIGNATURE_FROM_RPCORE' setting in configuration file- %s",INPUT_FILE);

}

void fatal_error(const char *file, int line, const char *msg)
{
    fprintf(stderr, "**FATAL** %s:%i %s\n", file, line, msg);
    ERR_print_errors_fp(stderr);
    exit(-1);
}

void create_attribute_cert()
{
	/* declare variables */
	FILE *fp = NULL;
	X509AC *a, *role_specification;
	X509 *holder_cert = NULL,*x = NULL;
	X509 *issuer_cert = NULL;
	char* hfilename = "holder.pem";
	char* ifilename = "/tmp/cert.pem";
	X509_NAME *holderPKCissuer;
	ASN1_INTEGER *holderSerialNumber;
	X509AC_ISSUER_SERIAL *hbasecertid,*ibasecertid;
	GENERAL_NAMES *gens;
	GENERAL_NAME *gen;
	X509_NAME *service_name;
	X509_NAME *ident_name;
	X509_ATTRIBUTE *attr1,*attr2,*attr3,*attr4,*attr5;
	SvceAuthInfo *attr_val1,*av1;
	SvceAuthInfo *attr_val4,*av4;
	IetfAttrSyntax *attr_val2,*av2, *attr_val5, *av5;
	RoleSyntax *attr_val3,*av3;
	int i = 0;
	char line[ONELINELEN];
	RSA *key;
	EVP_PKEY* pkey;
	unsigned char * data_ ="vm_uuid";
	unsigned char *p,**pp;
	ASN1_TYPE *at;
	ASN1_OCTET_STRING *os;
	X509_ATTRIBUTE *atr;
	unsigned long error;
	char* error_str;
	X509_EXTENSION *ext1,*ext2,*ext3,*ext4;
	
	X509_STORE* store = NULL;
	X509_LOOKUP* lookup = NULL;
	X509_STORE_CTX* verify_ctx = NULL;
	
	/* CAs path*/
	char* valid_X509_cert_dirpath_CA;

	/* Authorization path */
	char* valid_X509_cert_dirpath_SOA;

	ERR_load_crypto_strings(); 
	OpenSSL_add_all_algorithms();
	/* open holder PKI cert */
	
	//if (!(fp = fopen (/*ifilename*/hfilename, "r")))
	//{
	//		printf("Error opening certificate\n");
	//	exit(0);
	//}
	//if (!(holder_cert = PEM_read_X509(fp, NULL, NULL, NULL)))
	//{
	//	printf("Error decoding PEM\n");
	//		exit(0);
	//}

	//fclose (fp);

	/* open issuer PKI cert */
	if (!(fp = fopen (ifilename, "r")))
	{
		printf("Error opening certificate\n");
		exit(0);
	}
	if (!(issuer_cert = PEM_read_X509(fp, NULL, NULL, NULL)))
	{
		printf("Error decoding PEM\n");
		exit(0);
	}

	fclose (fp);

	x = holder_cert;
	/* alloc structure */

	/*
	
	  A) ATTRIBUTE CERTIFICATE WITH SOME ATTRIBUTES:
	  The specification of privileges is generally an application-specific issue, 
	  specification of privileges, apart from ROLE model is out of the scope of 
	  X.509 recommendation, but here we are usign as an example some of the described
	  in RFC 3281:

		1.- SERVICE AUTHENTICATION INFORMATION (as defined in RFC 3281)
		2.- ACCESS IDENTITY (as defined in RFC 3281)
		3.- CHARGING IDENTITY (as defined in RFC 3281)
		4.- ROLE (as defined in RFC3281,  ITU X.509)
	
	*/



	/*Read input file*/
	struct input_parameters input_params;
        //printf ("Reading config file...\n");
        parse_input_file(&input_params);

	printf("Create attribute certificate\n");
	a = X509AC_new();
	printf("Setting version to v2\n");
	X509AC_set_version(a,2);

	printf("Setting holder and issuer\n");
//#	define USE_BASECERTID
#	ifdef USE_BASECERTID
	//hbasecertid = X509_get_basecertID(holder_cert);
	//X509AC_set_holder_baseCertID(a,hbasecertid);
#	else
	//X509AC_set_holder_entity_name(a,X509_get_subject_name(x));
	
	X509_NAME *holder_name;
	holder_name = X509_NAME_new();
	struct sample_parameters parms;

	  //printf ("Initializing parameters to default values...\n");
	init_parameters (&parms);

	//printf ("Reading config file...\n");
	parse_config (&parms);
	struct entry
	{
	    char *key;
	    char *value;
	};

	struct entry entries[ENTRIES] =
  	{
	    { "countryName", parms.countryName },
	    { "stateOrProvinceName", parms.stateOrProvinceName },
	    { "localityName", parms.localityName },
	    { "organizationName", parms.organizationName },
	    { "organizationalUnitName", parms.organizationalUnitName },
	    { "commonName", parms.commonName },
	};

	
	for (i = 0; i < ENTRIES; i++)
    	{
        int nid;                  // ASN numeric identifier
        X509_NAME_ENTRY *ent;

        if ((nid = OBJ_txt2nid(entries[i].key)) == NID_undef)
        {
            fprintf(stderr, "Error finding NID for %s\n", entries[i].key);
            fatal("Error on lookup");
        }
        if (!(ent = X509_NAME_ENTRY_create_by_NID(NULL, nid, MBSTRING_ASC,
            (unsigned char*)entries[i].value, - 1)))
            fatal("Error creating Name entry from NID");

        if (X509_NAME_add_entry(holder_name, ent, -1, 0) != 1)
            fatal("Error adding entry to Name");
         }

	X509AC_set_holder_entity_name(a,holder_name);
	
#	endif

#	ifdef USE_BASECERTID
	ibasecertid = X509_get_basecertID(issuer_cert);
	X509AC_set_issuer_baseCertID(a,ibasecertid);
#	else
	X509AC_set_issuer_name(a,X509_get_subject_name(issuer_cert));
#	endif	
	printf("Setting time validity\n");
	if(!X509_gmtime_adj(a->info->validity->notBefore,0))
		printf("Error setting the notBefore information");
	if(!X509_gmtime_adj(a->info->validity->notAfter,/*segundos*/60*60*24*7))
		printf("Error setting the notAfter information");
	
	
	
	printf("Setting the attributes\n");


	/* creating a SvceAuthInfoAttribute value */

	/* Create first the attribute values for SvceAuthInfo
		Create 2 values:
			Leganés Ingenieria Telematica
			Getafe Biblioteca
	*/
	
	/* First value */
	//attr_val1 = SvceAuthInfo_new();
	

	/*service_name = X509_NAME_new();
	X509_NAME_add_entry_by_txt(service_name,"C",
				MBSTRING_ASC, "ES", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"ST",
				MBSTRING_ASC, "Madrid", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"L",
				MBSTRING_ASC, "Leganes", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"O",
				MBSTRING_ASC, "Universidad Carlos III de Madrid", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"OU",
				MBSTRING_ASC, "Departamento Ingenieria Telematica", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"CN",
				MBSTRING_ASC, "Servicio de correo electronico", -1, -1, 0);
    
	attr_val1->service->type = GEN_DIRNAME;
	attr_val1->service->d.directoryName = X509_NAME_dup(service_name);
	X509_NAME_free(service_name);

	ident_name = X509_get_subject_name(holder_cert);
 	attr_val1->ident->type = GEN_DIRNAME;
	attr_val1->ident->d.directoryName = X509_NAME_dup(ident_name);*/

	/* Second Value */

//	attr_val4 = SvceAuthInfo_new();
	

/*	service_name = X509_NAME_new();
	X509_NAME_add_entry_by_txt(service_name,"C",
				MBSTRING_ASC, "ES", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"ST",
				MBSTRING_ASC, "Madrid", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"L",
				MBSTRING_ASC, "Getafe", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"O",
				MBSTRING_ASC, "Universidad Carlos III de Madrid", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"OU",
				MBSTRING_ASC, "Library", -1, -1, 0);
	X509_NAME_add_entry_by_txt(service_name,"CN",
				MBSTRING_ASC, "Catalog", -1, -1, 0);*/
    
/*	attr_val4->service->type = GEN_DIRNAME;
	attr_val4->service->d.directoryName = X509_NAME_dup(service_name);
	X509_NAME_free(service_name);*/

/*	ident_name = X509_get_subject_name(holder_cert);
 	attr_val4->ident->type = GEN_DIRNAME;
	attr_val4->ident->d.directoryName = X509_NAME_dup(ident_name);*/


#ifdef TEST_IETF_ATTR_SYNTAX
	/* creating a IetfAttrSyntax */

/*	attr_val2 = IetfAttrSyntax_new();
	attr_val2->policyAuthority = GENERAL_NAMES_new();
	gen = GENERAL_NAME_new();
	gen->type = GEN_DIRNAME;		
	gen->d.directoryName = X509_NAME_dup(ident_name);
	sk_GENERAL_NAME_push(attr_val2->policyAuthority,gen);
	attr_val2->type = 0;//V_ASN1_OCTET_STRING;
	if( attr_val2->values.octets == NULL )
		attr_val2->values.octets = ASN1_OCTET_STRING_new();
	ASN1_OCTET_STRING_set(attr_val2->values.octets,data_,24);

  */      
        attr_val5 = IetfAttrSyntax_new();
        attr_val5->policyAuthority = GENERAL_NAMES_new();
        //gen = GENERAL_NAME_new();
        //gen->type = GEN_DIRNAME;
        //gen->d.directoryName = X509_NAME_dup(ident_name);
        //sk_GENERAL_NAME_push(attr_val5->policyAuthority,gen);
        attr_val5->type = 0;//V_ASN1_OCTET_STRING;^M
        if( attr_val5->values.octets == NULL )
                attr_val5->values.octets = ASN1_OCTET_STRING_new();
        ASN1_OCTET_STRING_set(attr_val5->values.octets,input_params.vm_uuid,strlen(input_params.vm_uuid));

#endif

#ifdef TEST_ROLE_SYNTAX
	/* Creating a RoleSyntax */
	/*attr_val3 = RoleSyntax_new();
	attr_val3->roleAuthority = GENERAL_NAMES_new();
	attr_val3->roleName->type = GEN_URI;
	attr_val3->roleName->d.uniformResourceIdentifier = ASN1_IA5STRING_new();
	ASN1_STRING_set(attr_val3->roleName->d.uniformResourceIdentifier, 
		(unsigned char*)"it.uc3m.es:administrator",strlen("it.uc3m.es:administrator"));*/
#endif

	/* Creating attributes and adding values */

//	attr1 = X509_ATTRIBUTE_new(); /* SvceAuthInfo */
//	attr2 = X509_ATTRIBUTE_new(); /* Access Identity */
#ifdef TEST_IETF_ATTR_SYNTAX
//	attr3 = X509_ATTRIBUTE_new(); /* Charging Identity */
        attr5 = X509_ATTRIBUTE_new();
#endif
#ifdef TEST_ROLE_SYNTAX
//	attr4 = X509_ATTRIBUTE_new(); /* Role */

#endif

//	attr1->object = OBJ_nid2obj(NID_id_aca_authenticationInfo);
//	attr2->object = OBJ_nid2obj(NID_id_aca_accessIdentity);

#ifdef TEST_IETF_ATTR_SYNTAX
//	attr3->object = OBJ_nid2obj(NID_id_aca_chargingIdentity);
        int nid2 = OBJ_create("2.25", "vm_uuid", "vm_uuid");
        attr5->object = OBJ_nid2obj(nid2);
#endif
#ifdef TEST_ROLE_SYNTAX
//	attr4->object = OBJ_nid2obj(NID_role);
#endif

//	X509attr_SvceAuthInfo_add_value(attr1, attr_val1 ); /* add Leganes */
//	X509attr_SvceAuthInfo_add_value(attr1, attr_val4 ); /* add Getafe */
//	X509attr_SvceAuthInfo_add_value(attr2, attr_val1 );
#ifdef TEST_IETF_ATTR_SYNTAX
//	X509attr_IetfAttrSyntax_add_value(attr3, attr_val2);
        X509attr_IetfAttrSyntax_add_value(attr5, attr_val5);
#endif
#ifdef TEST_ROLE_SYNTAX
//	X509attr_RoleSyntax_add_value(attr4, attr_val3);
#endif

/*	if(!X509AC_add_attribute( a, attr1 ))
		printf("Error adding attribute to the certificate");
	
	if(!X509AC_add_attribute( a, attr2 ))
		printf("Error adding attribute to the certificate");

#ifdef TEST_IETF_ATTR_SYNTAX
//	if(!X509AC_add_attribute( a, attr3 ))
//		printf("Error adding attribute to the certificate");
  */      if(!X509AC_add_attribute( a, attr5 ))
                printf("Error adding attribute to the certificate");
/*#endif

#ifdef TEST_ROLE_SYNTAX
	if(!X509AC_add_attribute( a, attr4 ))
		printf("Error adding attribute to the certificate");
#endif*/

	/* Extensions */

	/*NID_ac_auditEntity  Critical TRUE RFC3281*/


	os = ASN1_OCTET_STRING_new();
	ASN1_STRING_set(os,(unsigned char*)input_params.vm_image_id,strlen(input_params.vm_image_id));
	int nid3 = OBJ_create("2.5.4.179.3", "ImageId", "Image unique id");
        //attr5->object = OBJ_nid2obj(nid3);

	ext1 = X509_EXTENSION_create_by_NID(NULL,nid3,1,os);
	X509AC_add_extension(a,ext1,-1);

	os = ASN1_OCTET_STRING_new();
        ASN1_STRING_set(os,(unsigned char*)input_params.nonce,strlen(input_params.nonce));

        nid3 = OBJ_create("2.5.4.179.4", "nonce", "nonce");
        //attr5->object = OBJ_nid2obj(nid3);

        ext1 = X509_EXTENSION_create_by_NID(NULL,nid3,1,os);
        X509AC_add_extension(a,ext1,-1);

	os = ASN1_OCTET_STRING_new();
        ASN1_STRING_set(os,(unsigned char*)input_params.vm_hash,strlen(input_params.vm_hash));

        nid3 = OBJ_create("2.16.840.1.101.3.4.2.1", "vm_measurement", "vm_measurement");
        //attr5->object = OBJ_nid2obj(nid3);

        ext1 = X509_EXTENSION_create_by_NID(NULL,nid3,1,os);
        X509AC_add_extension(a,ext1,-1);
	
	os = ASN1_OCTET_STRING_new();
        ASN1_STRING_set(os,(unsigned char*)input_params.vm_ip,strlen(input_params.vm_ip));

        nid3 = OBJ_create("2.5.4.179.5", "vm_ip", "vm_ip");
        //attr5->object = OBJ_nid2obj(nid3);

        ext1 = X509_EXTENSION_create_by_NID(NULL,nid3,1,os);
        X509AC_add_extension(a,ext1,-1);



	/* id-ce-timeSpecification: this extension not included since entity can
	   act as an Attribute Authority
	*/
	

	/* SIgnature*/
	//fp = fopen("cert.key","r");
	//pkey = PEM_read_PrivateKey(fp,NULL,NULL,NULL);
	//X509AC_sign_pkey(a, pkey, EVP_sha1());


	
	X509_ALGOR *signatureType = X509_ALGOR_new();
	ASN1_OBJECT *signatureTypeObject = OBJ_nid2obj("SHA256");
	signatureType->algorithm = signatureTypeObject;
	a->algor = signatureType;
	
	int signatureLength=strlen(input_params.sign_from_rpcore);
	a->signature->data = (unsigned char *)malloc(signatureLength);
	memcpy(a->signature->data, input_params.sign_from_rpcore, signatureLength);
	a->signature->length = signatureLength;
	//memcpy(certificate->signature->data, signature, signatureLenght);
	//certificate->signature->length = signatureLenght;

	printf("Printing the attribute certificate\n");
	printf("----------------------------------\n\n");

	//X509AC_print(a);	
	fp = fopen("/tmp/attr_cert.pem","w");
	PEM_write_X509AC(fp,a);
	fclose(fp);
	X509AC_free(a);
	fp = fopen("/tmp/attr_cert.pem","r");
	a = PEM_read_X509AC(fp,NULL,NULL,NULL);
	fclose(fp);
	//X509AC_print(a);


#ifdef TEST_VERIFY
	/* under development */
#endif
}

void main()
{
create_attribute_cert();
//printf("hi");
}

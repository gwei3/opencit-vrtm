
//author: Vinay Phegade

#include <Python.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "pyifc.h"

static 	 PyObject  *xmlrpc_enc = NULL, *xmlrpc_dec = NULL;

int init_pyifc(char *filename) {

	PyObject *name = NULL, *module=NULL, *dictionary=NULL, *func=NULL, *value=NULL;;


    Py_Initialize();

    name = PyString_FromString(filename);

    module = PyImport_Import(name);
	if (! module )
		goto cleanup;
	
	Py_XDECREF(name);
	  
    dictionary = PyModule_GetDict(module);

    Py_XDECREF(module);

    xmlrpc_enc = PyDict_GetItemString( dictionary, "rpc_encoder");
    xmlrpc_dec = PyDict_GetItemString( dictionary, "rpc_decoder");

    Py_XDECREF(dictionary);

    if (!PyCallable_Check(xmlrpc_enc)) 
			goto cleanup;
	
	Py_XINCREF(xmlrpc_enc);
 
 
    if (!PyCallable_Check(xmlrpc_dec)) 
			goto cleanup;
	
	Py_XINCREF(xmlrpc_dec);

	return 0;
	
cleanup:
	Py_XDECREF(xmlrpc_enc);
	Py_XDECREF(xmlrpc_dec);
	
	return -1;
  
}

int cbuf_to_xmlrpc(const char* func, const char* method, int size, const byte* data, int bufsize, byte* buf) {
	
	int status = -1;
	
	PyObject *inst = NULL, *args = NULL, *rval = NULL;
	Py_ssize_t 	pybufsize = 0;
	char* 		pybuf = NULL;
	
	inst = PyObject_CallObject( xmlrpc_enc, NULL ); 
		
	rval = PyObject_CallMethod( inst, (char*)func, "sz#", method, data, size );
	
	if (! rval )
		goto cleanup;
	
	status = PyString_AsStringAndSize(rval,(char**)&pybuf, &pybufsize);
	
	if (status < 0 )
		goto cleanup;
		
	if ( pybufsize < bufsize) {
		memcpy(buf, pybuf, pybufsize );
		status = pybufsize;
	}
	
cleanup:
	Py_XDECREF(rval);
	Py_XDECREF(inst);
	return status;
}

int args_to_xmlrpc(const char* method, int nargs, char** args, int bufsize, byte* buf) {
	
	int status = -1;
	
	PyObject *inst = NULL,  *rval = NULL;
	Py_ssize_t 	pybufsize = 0;
	char* 		pybuf = NULL;
	
	inst = PyObject_CallObject( xmlrpc_enc, NULL ); 
	
	rval = PyObject_CallMethod( inst, "Int", "i", nargs );
	if (! rval )
		goto cleanup;
		
	Py_XDECREF(rval);
	
	for (int i = 0; i < nargs; i++) {
		rval = PyObject_CallMethod( inst, "String", "s", args[i] );
		if (! rval )
			goto cleanup;
			
		Py_XDECREF(rval);
	}
	
	rval = PyObject_CallMethod( inst, "get_xml", "si", method, 0 );
	if (! rval )
		goto cleanup;
	
	status = PyString_AsStringAndSize(rval,(char**)&pybuf, &pybufsize);
	
	if (status < 0 )
		goto cleanup;
		
	if ( pybufsize < bufsize) {
		memcpy(buf, pybuf, pybufsize );
		status = pybufsize;
	}
	
cleanup:
	Py_XDECREF(rval);
	Py_XDECREF(inst);
	return status;
}

int args_to_xmlrpc_response(const char* method, int nargs, char** args, int bufsize, byte* buf) {
	
	int status = -1;
	
	PyObject *inst = NULL,  *rval = NULL;
	Py_ssize_t 	pybufsize = 0;
	char* 		pybuf = NULL;
	
	inst = PyObject_CallObject( xmlrpc_enc, NULL ); 
	
	rval = PyObject_CallMethod( inst, "Int", "i", nargs );
	if (! rval )
		goto cleanup;
		
	Py_XDECREF(rval);
	
	for (int i = 0; i < nargs; i++) {
		rval = PyObject_CallMethod( inst, "String", "s", args[i] );
		if (! rval )
			goto cleanup;
			
		Py_XDECREF(rval);
	}
	
	rval = PyObject_CallMethod( inst, "get_xml", "si", method, 1 );
	if (! rval )
		goto cleanup;
	
	status = PyString_AsStringAndSize(rval,(char**)&pybuf, &pybufsize);
	
	if (status < 0 )
		goto cleanup;
		
	if ( pybufsize < bufsize) {
		memcpy(buf, pybuf, pybufsize );
		status = pybufsize;
	}
	
cleanup:
	Py_XDECREF(rval);
	Py_XDECREF(inst);
	return status;
}

int xmlrpc_to_cbuf(const char* func, int* psize, byte* data, const byte* buf) {
	
	int status = -1;
	
	PyObject *inst = NULL, *args = NULL, *rval = NULL;
	Py_ssize_t 	pybufsize = 0;
	void* 		pybuf = NULL;
	
	inst = PyObject_CallObject( xmlrpc_dec, NULL ); 
		
	rval = PyObject_CallMethod( inst, (char*)func, "s", (char*)buf );
	
	if (! rval )
		goto cleanup;
	
	status = PyObject_AsReadBuffer(rval,(const void**)&pybuf, &pybufsize);
	
	if (status < 0 )
		goto cleanup;
	
	//ar: pass buffersize	
//	if ( pybufsize < *psize) {
		memcpy((void*)data, pybuf, pybufsize );
		*psize = pybufsize;
		status = pybufsize;
//	}
	
cleanup:
	Py_XDECREF(rval);
	Py_XDECREF(inst);
	return status;
}

int xmlrpc_to_args(char** psz, int* pnargs, char**pargs, const byte* buf) {

	xmlDocPtr doc;
	xmlNode *root = NULL, *cur_node = NULL, *param_node = NULL;
	char *method, *param;
	int i=0, arg_count = 0, status = -1;

	doc = xmlParseMemory((char*)buf, strlen((char*)buf));
	root = xmlDocGetRootElement(doc);

	for(cur_node = root->children; cur_node != NULL; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE  && 
				!xmlStrcmp(cur_node->name, (const xmlChar *) "methodName")) {
			method = (char*)xmlNodeGetContent(cur_node);
		}            
		else if (cur_node->type == XML_ELEMENT_NODE  && 
					!xmlStrcmp(cur_node->name, (const xmlChar *) "params")) {
			for (param_node = cur_node->children; param_node != NULL;  
					param_node = param_node->next) {
				if(xmlNodeIsText(param_node)) 
					continue;
				param = (char*) xmlNodeGetContent(param_node);
				if (param[strlen(param)-1] == '\n')
					param[strlen(param)-1] = '\0';
				if (param[0] == '\n')
					param ++;
				pargs[arg_count] = strdup(param);
				arg_count++;
			}                
		}
	}
	xmlFreeDoc (doc);

	*psz = method;
	*pnargs = arg_count;
	status = *pnargs;

	return status;

/*  GSL: Removed python binding code for xml to args conversion

	int status = -1;
	
	PyObject *inst = NULL, *method = NULL, *rval = NULL;
	PyObject *args = NULL;
	
	Py_ssize_t 	pybufsize = 0;
	char* 		pybuf = NULL;
	
	char* bufcopy = NULL;
	bufcopy = strdup((char*)buf);
	
	*psz = NULL;
	
	inst = PyObject_CallObject( xmlrpc_dec, NULL ); 
		
	rval = PyObject_CallMethod( inst, "xmlrpc_args", "s", bufcopy );

	free(bufcopy);
	
	if (! rval )
		goto cleanup;
	
	pybuf  = PyString_AsString(rval);
		
	if (pybuf)
		*psz = strdup((char*)pybuf);

	Py_DECREF(rval);
	
	rval = PyObject_CallMethod( inst, "size", NULL );
	
	if (! rval )
		goto cleanup;
	
	*pnargs = (int) PyInt_AsSsize_t(rval);
	
	
	Py_DECREF(rval);
	
	for (int i =1; i < *pnargs; i++ ) 
	{		
		rval = PyObject_CallMethod( inst, "items", "i", i );
		if (! rval )
			goto cleanup;
		
		pybuf  = PyString_AsString(rval);
		
		if (!pybuf )
			goto cleanup;
	
		pargs[i] = strdup((char*)pybuf);
		Py_DECREF(rval);
	}
	
	status = *pnargs;
	
cleanup:
	if (status < 0 ) {
		if (*psz )
			free (*psz);
	}
	
	Py_XDECREF(rval);
	Py_XDECREF(inst);
	return status;
*/

}

void deinit_pyifc() {

	Py_XDECREF(xmlrpc_enc);
	Py_XDECREF(xmlrpc_dec);
	Py_Finalize();
}


#ifdef STANDALONE
int main(int argcc, char** argv) {
	byte buf[4096];
	byte rbuf[4096];
	
	const byte* data = (byte*)"Hello world";
	int size;
	
	int an = 6;
	char* av[] = {"-kernel", "vmlinux", "-ramdisk", "initrd", "-config", "config"};
	
	char* args[32];
	char* name ;
	
	setenv("PYTHONPATH", "./", 1);
	
	
	init_pyifc(argv[1]);
	
	printf("Python initialization successful!!!\n");
	//cbuf_to_xmlrpc("encode_call", "foobaifoo", 11, data, 4096, buf);
	//size = 4096;
	//xmlrpc_to_cbuf ("decode_rpc", &size,  rbuf, buf);
	//cbuf_to_xmlrpc("encode_response", "", 11, data, 4096, buf);

	//size = 4096;
	//xmlrpc_to_cbuf ("decode_rpc", &size,  rbuf, buf);
	
	args_to_xmlrpc("start_app", an, av, 4096, buf);
	 
	xmlrpc_to_args(&name, &an, args, buf);
	
	deinit_pyifc();
	return 0;
}
#endif

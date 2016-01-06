
//author: Vinay Phegade

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include "parser.h"
#include "base64.h"
#include "logging.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "safe_lib.h"
#ifdef __cplusplus
}
#endif

int cbuf_to_xmlrpc(const char* func, const char* method, int size, const byte* data, int bufsize, byte* buf) {

	xmlDocPtr doc;
	xmlNodePtr root, params_node, param, param_value, value_type;
	LOG_TRACE("%s", data);
	//eprin("abd");
	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL,BAD_CAST "methodCall");
	xmlDocSetRootElement(doc, root);
	LOG_DEBUG("XML doc created, root element set");
	xmlNewChild(root, NULL, BAD_CAST "methodName", (xmlChar *)method);
	params_node = xmlNewChild(root, NULL, BAD_CAST "params", NULL);
	param = xmlNewChild(params_node, NULL, BAD_CAST "param", NULL);
	param_value = xmlNewChild(param, NULL, BAD_CAST "value", NULL);
	// TODO base64encode the input data make param_value child
	char* encoded_data;
	if (Base64Encode( (char *)data, &(encoded_data)) == 0 ) {
		LOG_DEBUG("Encoded Data : %s", encoded_data);
		value_type = xmlNewChild(param_value, NULL, BAD_CAST "string", BAD_CAST encoded_data);
		xmlChar * xmlbuf;
		xmlDocDumpFormatMemory(doc, &xmlbuf, &bufsize, 0);
		memcpy_s(buf, MAX_LEN, xmlbuf, bufsize + 1);
		LOG_DEBUG("XML Data : %s", buf);
		free(xmlbuf);
		free(encoded_data);
	}
	else {
		LOG_INFO("Error!, Encoding the data");
		bufsize = -1;
	}

	xmlFreeDoc(doc);
	//xmlCleanupParser();
	return bufsize;
}

int args_to_xmlrpc(const char* method, int nargs, char** args, int bufsize, byte* buf) {
	xmlDocPtr doc;
	xmlNodePtr root, params_node, param, param_value, value_type;
	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewNode(NULL,BAD_CAST "methodCall");
	xmlDocSetRootElement(doc, root);
	xmlNewChild(root, NULL, BAD_CAST "methodName",(xmlChar *) method);
	params_node = xmlNewChild(root, NULL, BAD_CAST "params", NULL);
	char* encoded_data;
	LOG_DEBUG("No of parameters to converted in XML %d", nargs);
	for( int i = 0 ; i < nargs ; i ++ ) {
		param = xmlNewChild(params_node, NULL, BAD_CAST "param", NULL);
		param_value = xmlNewChild(param, NULL, BAD_CAST "value", NULL);
		LOG_DEBUG("Parameter %d : %s ", i+1, args[i]);
		// TODO base64encode the input data make param_value child
		if (Base64Encode(args[i], &encoded_data) == 0) {
			LOG_DEBUG("Encoded Parameter : %s", encoded_data);
			value_type = xmlNewChild(param_value, NULL, BAD_CAST "string", BAD_CAST encoded_data);
			free(encoded_data);
		}
		else {
			LOG_DEBUG("Error! in encoding the parameter");
			xmlFreeDoc(doc);
			//xmlCleanupParser();
			return -1;
		}
		//xmlNewChild(param_value, NULL, BAD_CAST "string", BAD_CAST args[i]);
	}

	xmlChar * xmlbuf;
	xmlDocDumpFormatMemory(doc, &xmlbuf, &bufsize, 0);
	memcpy_s(buf, MAX_LEN, xmlbuf, bufsize + 1);
	free(xmlbuf);
	xmlFreeDoc(doc);
	//xmlCleanupParser();
	return bufsize;
}


int xmlrpc_to_cbuf(const char* func, int* psize, byte* data, const byte* buf) {
	xmlDocPtr doc;
	xmlNode * root = NULL, *cur_node = NULL, *param_node = NULL;
	char *method = NULL, *param, *decoded_data;
	int xmldata_size = -1;
	LOG_DEBUG("XML to be parsed : %s", buf);
	if (strnlen_s((char *)buf, MAX_LEN) == 0) {
		xmldata_size = *psize = 0;
		return xmldata_size;
	}
	doc = xmlParseMemory((char *)buf, strnlen_s((char *)buf, MAX_LEN));
	if(doc == NULL) {
		xmldata_size = *psize = 0;
		return xmldata_size;
	}

	root = xmlDocGetRootElement(doc);
	for(cur_node = root->children ; cur_node != NULL ; cur_node = cur_node->next) {
		if( cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (xmlChar *)"methodName")) {
			method = (char *)xmlNodeGetContent(cur_node); // not used , free'd at end
		}
		else if(cur_node->type == XML_ELEMENT_NODE && !xmlStrcmp(cur_node->name, (xmlChar *)"params")) {
			//param_node = xmlNodeGetContent(cur_node);
			for(param_node = cur_node->children; param_node != NULL ; param_node = param_node->next ) {
				if(xmlNodeIsText(param_node))
					continue;
				param = (char *)xmlNodeGetContent(cur_node);
				LOG_DEBUG("Encoded Node content : %s", param);
				if (Base64Decode(param, &decoded_data) ) {
					LOG_DEBUG("Error! in decoding the Node content");
					xmldata_size = *psize = -1;
					free(param);
					break;
				}
				LOG_DEBUG("Decoded Node Content : %s", decoded_data);
				if(decoded_data != NULL) {
					*psize = strnlen_s(decoded_data, MAX_LEN);
					memcpy_s(data, MAX_LEN, decoded_data, *psize+1);
					xmldata_size = *psize;
					free(decoded_data);
				}
				free(param);
			}
		}
	}
	if(method != NULL)	free(method);
	xmlFreeDoc(doc);
	//xmlCleanupParser();
	return xmldata_size;
}

int xmlrpc_to_args(char** psz, int* pnargs, char**pargs, const byte* buf) {

	xmlDocPtr doc;
	xmlNode *root = NULL, *cur_node = NULL, *param_node = NULL;
	char *method = NULL, *param, *decoded_data;
	int i=0, arg_count = 0, status = -1;
	LOG_DEBUG("XML to be parsed : %s", buf);
	if (strnlen_s((char *)buf, MAX_LEN) == 0) {
		method = (char *)calloc(1,sizeof(char));
		if(method != NULL) {
			method[0] = '\0';
			*psz = method;
		}
		*pnargs = arg_count;
		status = *pnargs;
		return status;
	}
	doc = xmlParseMemory((char*)buf, strnlen_s((char*)buf, MAX_LEN));
	if(doc == NULL) {
		method = (char *)calloc(1,sizeof(char));
		if(method != NULL) {
			method[0] = '\0';
			*psz = method;
		}
		*pnargs = arg_count;
		status = *pnargs;
		return status;
	}

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
				LOG_DEBUG("Encoded Node content : %s", param);
				if(Base64Decode(param, &decoded_data) == 0 ) {
					if(decoded_data != NULL) {
						LOG_DEBUG("Decoded Node content : %s", decoded_data);
						pargs[arg_count] = strdup( decoded_data);
						free(decoded_data);
					}
					xmlFree((xmlChar *)param);
					arg_count++;
				}
				else {
					LOG_DEBUG("Error in Decoding the node content");
					for( i = 0 ;i < arg_count ; i++ ) {
                        if(pargs[i]) {
						    free(pargs[i]);
                            pargs[i] = NULL;
                        }
					}
					xmlFree((xmlChar *)param);
					xmlFreeDoc (doc);
					//xmlCleanupParser();
					return status;
				}
			}                
		}
	}
	xmlFreeDoc (doc);
	//xmlCleanupParser();
	xmlMemoryDump();
	if(method != NULL)	*psz = method;
	*pnargs = arg_count;
	status = *pnargs;

	return status;

}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    xmlDocPtr doc = NULL;       /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */
    xmlDtdPtr dtd = NULL;       /* DTD pointer */
    char buff[256];
    int i, j;

    LIBXML_TEST_VERSION;

    /*
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);

    /*
     * Creates a DTD declaration. Isn't mandatory.
     */
    dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "tree2.dtd");

    /*
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node.
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node1",
                BAD_CAST "content of node 1");
    /*
     * The same as above, but the new child node doesn't have a content
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node2", NULL);

    /*
     * xmlNewProp() creates attributes, which is "attached" to an node.
     * It returns xmlAttrPtr, which isn't used here.
     */
    node =
        xmlNewChild(root_node, NULL, BAD_CAST "node3",
                    BAD_CAST "this node has attributes");
    xmlNewProp(node, BAD_CAST "attribute", BAD_CAST "yes");
    xmlNewProp(node, BAD_CAST "foo", BAD_CAST "bar");

    /*
     * Here goes another way to create nodes. xmlNewNode() and xmlNewText
     * creates a node and a text node separately. They are "attached"
     * by xmlAddChild()
     */
    node = xmlNewNode(NULL, BAD_CAST "node4");
    node1 = xmlNewText(BAD_CAST
                   "other way to create content (which is also a node)");
    xmlAddChild(node, node1);
    xmlAddChild(root_node, node);

    /*
     * A simple loop that "automates" nodes creation
     */
    for (i = 5; i < 7; i++) {
        snprintf(buff, sizeof(buff), "node%d", i);
        node = xmlNewChild(root_node, NULL, BAD_CAST buff, NULL);
        for (j = 1; j < 4; j++) {
            snprintf(buff, sizeof(buff), "node%d%d", i, j);
            node1 = xmlNewChild(node, NULL, BAD_CAST buff, NULL);
            xmlNewProp(node1, BAD_CAST "odd", BAD_CAST((j % 2) ? "no" : "yes"));
        }
    }
	xmlChar *temp_buffer = (xmlChar*)calloc(1,sizeof(xmlChar)*1024*10);
        int temp_buffer_size = 0;
        //xmlDocDumpFormatMemory(doc, &temp_buffer, &temp_buffer_size, 1);
        //printf("printing from buffer: length : %d  : \n %s\n",temp_buffer_size,temp_buffer);

    /*
     * Dumping document to stdio or file
     */
    xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
	char * data = "abra";
	
	cbuf_to_xmlrpc("foo", "foo", 5, data, temp_buffer_size, temp_buffer);
	printf("cbuf to xmlrpc: length : %d :\n %s\n:",temp_buffer_size,temp_buffer);
	char * buff_arr[2];
	buff_arr[0] = &"abra";
	buff_arr[1] = &"ka dabra";
	args_to_xmlrpc("foo", 2, buff_arr, temp_buffer_size, temp_buffer);

	printf("args_to_xmlrpc : length : %d : \n %s\n",temp_buffer_size,temp_buffer);
	return 0;
}
#endif

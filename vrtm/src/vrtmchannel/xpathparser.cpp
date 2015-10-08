/*
 * xpathparser.cpp
 *
 *synopsis: 	provide API to parse xpath with libxml2, setup_xpath_parser() and then pass the xpath expression
 *				whose values you want, then call teardown_xpath_parser() to free all the initialized data structure
 *
 *
 *  Created on: 28-Aug-2015
 *      Author: Vijay Prakash
 */

#include "xpathparser.h"

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

int list_elements_from_object(xmlNodeSetPtr nodes, char** elements_buf, int elements_buf_size);

/**
 * setup_xpath_parser:
 * @doc : 			pointer to doc pointer
 * @xpathCtx: 		pointer to xmlXPathContextPtr pointer
 * @filename: 		Full path of file which needs to be parsed
 *
 * Initialize the libxml2 library, load the document and create a new context for document
 *
 * return 0 on success and negative value on error
 */
int setup_xpath_parser(xmlDocPtr* doc,xmlXPathContextPtr * xpathCtx, const char* filename) {
	// Initialize libxml parser
	xmlInitParser();
	//Load XML document
	*doc = xmlParseFile(filename);
	if (*doc == NULL) {
		LOG_ERROR("Unable to parser file : %s", filename);
		return -1;
	}
	//Create xpath evaluation context
	*xpathCtx = xmlXPathNewContext(*doc);
	if (*xpathCtx == NULL ) {
		LOG_ERROR("Unable to create new XPath context");
		return -1;
	}
	return 0;
}

/**
 * parse_xpath:
 * @xpathCtx:			xmlXPathContextPtr of doc in which xpath is going to be searched
 * @xpathExpr:			xpath expression
 * @nslist:				char string of namespaces separated by space
 * @elements_buf:		buffer to store the elements value
 * @elements_buf_size:	number of entries buffer can store
 *
 * Parse the xpath and return the list of values of given xpath
 *
 *
 * if xpath contains multiple values it will return all values in one string,
 * So, provide exact xpath of element whose value you want to get
 * if multiple element with xpath are matched it return the array of character string,
 * if buffer to store elements value is NULL and its size is 0 then it return requred size of buffer
 *
 * on success return possible number of elements with given xpath if elements_buf is NULL and elements_buf_size is 0,
 * else return number of elements with given xpath
 * and negative value on error
 */

int parse_xpath(xmlXPathContextPtr xpathCtx, xmlChar* xpathExpr, xmlChar* nsList, char** elements_buf, int elements_buf_size) {
	xmlXPathObjectPtr xpathObj;

	// Register namespaces from list (if any)
	if ((nsList != NULL) && (register_namespaces(xpathCtx, nsList) < 0 )) {
		LOG_ERROR("failed to register namespaces list: %s", nsList);
		return -1;
	}

	//Evaluate xpath expression
	xpathObj = xmlXPathEvalExpression( xpathExpr, xpathCtx);
	if (xpathObj == NULL) {
		LOG_ERROR("unable to evaluate xpath expression: %s", xpathExpr);
		return -1;
	}
	// return the array of strings of elements values
	return list_elements_from_object(xpathObj->nodesetval, elements_buf, elements_buf_size);
}

/**
 * register_namespaces:
 * @xpathCtx:           the pointer to an XPath context.
 * @nsList:             the list of known namespaces in
 *                      "<prefix1>=<href1> <prefix2>=href2> ..." format.
 *
 * Registers namespaces from @nsList in @xpathCtx.
 *
 * Returns 0 on success and a negative value otherwise.
 */
int register_namespaces( xmlXPathContextPtr xpathCtx, xmlChar* nsList) {
	xmlChar* nsListDup;
	xmlChar* prefix;
	xmlChar* href;
	xmlChar* next;

	nsListDup = xmlStrdup(nsList);
	if(nsListDup == NULL) {
		LOG_ERROR( "unable to strdup namespaces list\n");
		return(-1);
	}

	next = nsListDup;
	while(next != NULL) {
		/* skip spaces */
		while((*next) == ' ') next++;
		if((*next) == '\0') break;

		/* find prefix */
		prefix = next;
		next = (xmlChar*)xmlStrchr(next, '=');
		if(next == NULL) {
			LOG_ERROR("invalid namespaces list format\n");
			xmlFree(nsListDup);
			return(-1);
		}
		*(next++) = '\0';

		/* find href */
		href = next;
		next = (xmlChar*)xmlStrchr(next, ' ');
		if(next != NULL) {
			*(next++) = '\0';
		}
        /* do register namespace */
        if(xmlXPathRegisterNs(xpathCtx, prefix, href) != 0) {
            LOG_ERROR("unable to register NS with prefix=\"%s\" and href=\"%s\"\n", prefix, href);
            xmlFree(nsListDup);
            return(-1);
        }
	}
	xmlFree(nsListDup);
	return 0;
}

/**
 * list_elements_from_object:
 *
 * @nodes : 			set of xmlNodes
 * @elements_buf: 		buffer to store the elements values
 * @elements_buf_size:	number of elements values buffer can store
 *
 * currently only return the elements values. It does not support attribute values can  be
 * get with xmlGetProp() API
 *
 * return number of elements in given xmlNodeSetPtr
 */

int list_elements_from_object(xmlNodeSetPtr nodes, char** elements_buf, int elements_buf_size) {
	xmlNodePtr cur;
	int size;
	int i;
	char* node_content;
	size = (nodes) ? nodes->nodeNr : 0;
	LOG_DEBUG("Total number of element with given xpath : %d", size);
	//return the possible number of elements with matching xpath
	if (elements_buf == NULL || elements_buf_size == 0) {
		return size;
	}
	for( i = 0 ; i < size && i < elements_buf_size ; i++ ) {
        if(nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            xmlNsPtr ns;

            ns = (xmlNsPtr)nodes->nodeTab[i];
            cur = (xmlNodePtr)ns->next;
            if(cur->ns) {
                LOG_DEBUG( "= namespace \"%s\"=\"%s\" for node %s:%s\n",
                    ns->prefix, ns->href, cur->ns->href, cur->name);
            } else {
            	LOG_DEBUG( "= namespace \"%s\"=\"%s\" for node %s\n",
            			ns->prefix, ns->href, cur->name);
            }
        } else if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
            cur = nodes->nodeTab[i];
            node_content = (char *)xmlNodeGetContent(cur);
            if(cur->ns) {
                LOG_DEBUG( "= element node \"%s:%s\" value : %s\n",
                    cur->ns->href, cur->name, node_content);
            } else {
                LOG_DEBUG( "= element node \"%s\" \t value : %s\n",
                    cur->name, node_content);
            }
            strcpy(elements_buf[i], node_content);
            free(node_content);
        } else {
            cur = nodes->nodeTab[i];
            LOG_DEBUG( "= node \"%s\": type %d\n", cur->name, cur->type);
        }
	}
	return i;
}

/**
 * teardown_xpath_parser :
 *
 * @doc:			xmlDocPtr, whose related data structure want to free
 * @xpathCtx:		xmlXPathContextPtr associated to the doc
 *
 * Tear down the instance of libxml and free the variables
 *
 * return 0
 */
int teardown_xpath_parser(xmlDocPtr doc, xmlXPathContextPtr xpathCtx) {
	/* Shutdown libxml */
	//xmlCleanupParser(); // causes memory leak
	if (doc ) xmlFreeDoc(doc);
	if (xpathCtx) xmlXPathFreeContext(xpathCtx);
	return 0;
}
#endif

#ifdef STANDALONE

#endif


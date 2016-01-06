/*
 * xpathparser.h
 *
 *  Created on: 28-Aug-2015
 *      Author: vijay
 */

#ifndef XPATH_PARSER_H_
#define XPATH_PARSER_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <logging.h>

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

int register_namespaces(xmlXPathContextPtr xpathCtx, xmlChar* nsList);
int list_elements_from_object(xmlNodeSetPtr nodes, char** elements_buf, int elements_buf_size);
int setup_xpath_parser(xmlDocPtr* doc,xmlXPathContextPtr * xpathCtx, const char* filename);
int parse_xpath(xmlXPathContextPtr xpathCtx, xmlChar* xpathExpr, xmlChar* nsList, char** elements_buf, int elements_buf_size);
int teardown_xpath_parser(xmlDocPtr doc, xmlXPathContextPtr xpathCtx);

#endif

#endif /* XPATH_PARSER_H_ */

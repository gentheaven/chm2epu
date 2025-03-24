/*
 Copyright (C) 2019 Albert Chen
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 
 Author: gentheaven@hotmail.com (Albert Chen)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mylib_parse.h"

/////////////////////////////////////////////////////////////////////////
//parse html content, return DOM tree
lxb_html_document_t* parser_init(char* buf, size_t len)
{
	lxb_status_t status;
	lxb_html_document_t* document;

	document = lxb_html_document_create();
	if (document == NULL) {
		return NULL;
	}

	status = lxb_html_document_parse(document, buf, len);
	if (status != LXB_STATUS_OK) {
		return NULL;
	}

	return document;
}

void parse_exit(lxb_html_document_t* document)
{
	if(document)
		lxb_html_document_destroy(document);
}


//<a target="_blank" href = "https://www.zhihu.com/question/5073499722/answer/40533676669"
//save url to result
int get_a_href(lxb_dom_node_t* node, char* result)
{
	lxb_dom_attr_t* attr;
	attr = lxb_dom_element_attr_by_id(lxb_dom_interface_element(node), LXB_DOM_ATTR_HREF);
	if (!attr)
		return 0;

	const char* str = lxb_dom_attr_value(attr, NULL);
	if (!str)
		return 0;
	strcpy(result, str);
	return 0;
}

int is_attr_value(lxb_dom_element_t* node, char* attr_name, char* value)
{
	lxb_dom_attr_t* attr;
	attr = lxb_dom_element_attr_by_name(node, attr_name, strlen(attr_name));
	if (!attr)
		return 0;
	const lxb_char_t* str = lxb_dom_attr_value(attr, NULL);
	if (!str)
		return 0;
	if (lexbor_str_data_casecmp(value, str))
		return 1;
	return 0;
}

/*
<div class="AuthorInfo">
    <meta itemprop="name" content="������˾��">

	cur = find_sub_attr(node, "meta", "itemprop", "name");
	match  <meta itemprop="name" >, return this <meta> node

 return node if found, 
 return NULL if not found
*/
lxb_dom_element_t* find_sub_attr(lxb_dom_element_t* node, lxb_char_t* name, char* attr, char* value)
{
	lxb_dom_element_t* element = NULL;
	lxb_status_t status;
	lxb_dom_collection_t* collection;

	lxb_html_document_t* doc = lxb_html_element_document(lxb_html_interface_element(node));
	collection = lxb_dom_collection_make(&doc->dom_document, 16);

	//e.g. find_sub_attr(node, "meta", "itemprop", "name");
	//find all <meta> nodes
	status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(node), collection,
		(const lxb_char_t*)name, strlen(name));
	if (status != LXB_STATUS_OK) {
		lxb_dom_collection_destroy(collection, true);
		return NULL;
	}

	size_t i;
	size_t len = lxb_dom_collection_length(collection);
	int found = 0;
	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(collection, i);
		//e.g. find_sub_attr(node, "meta", "itemprop", "name");
		//match itemprop = name
		if(is_attr_value(element, attr, value)) {
			found = 1;
			break;
		}
	}

	lxb_dom_collection_destroy(collection, true);
	if(found)
		return element;
	return NULL;
}

static lxb_status_t serializer_callback(const lxb_char_t* data, size_t len, void* ctx)
{
	FILE* fp = (FILE*)ctx;
	fwrite(data, 1, len, fp);
	return 0;
}

void serialize_node(lxb_dom_node_t* node, FILE* fp)
{
	lxb_html_serialize_tree_cb(node, serializer_callback, (void*)fp);
}


struct serial_buf_str {
	char* buf;
	int len;
	int cur_len;
};

static lxb_status_t serializer_buf_callback(const lxb_char_t* data, size_t len, void* ctx)
{
	struct serial_buf_str *sbuf = (struct serial_buf_str *)ctx;
	char* head = sbuf->buf + sbuf->cur_len;
	if ((sbuf->cur_len + len) >= sbuf->len) {
		printf("html content is too big, current buf size is %d\n", sbuf->len);
		return 1;
	}

	memcpy(head, data, len);
	head[len] = 0;
	sbuf->cur_len = sbuf->cur_len + (int)len;
	return 0;
}

void serialize_node_buf(lxb_dom_node_t* node, char* buf, int buf_len)
{
	struct serial_buf_str sbuf;
	sbuf.buf = buf;
	sbuf.len = buf_len;
	sbuf.cur_len = 0;
	lxb_html_serialize_tree_cb(node, serializer_buf_callback, (void*)&sbuf);
}

//call lxb_dom_collection_destroy(collection, true); after use
lxb_dom_collection_t* get_nodes_by_tag(lxb_dom_node_t* node, char* tag_name)
{
	lxb_status_t status;
	lxb_dom_collection_t* collection;

	lxb_html_document_t* doc;
	doc = lxb_html_interface_document(node->owner_document);
	collection = lxb_dom_collection_make(&doc->dom_document, 16);

	status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(node), collection,
		(const lxb_char_t*)tag_name, strlen(tag_name));
	if (status != LXB_STATUS_OK) {
		lxb_dom_collection_destroy(collection, true);
		return NULL;
	}
	return collection;
}

/*
<head>
	<meta charset="UTF-8">
	...
</head>
*/
void add_charset(lxb_html_document_t* doc)
{
	lxb_html_head_element_t* head = lxb_html_document_head_element(doc);
	if (!head)
		return;
	
	lxb_dom_element_t* el;
	el = lxb_dom_document_create_element(lxb_dom_interface_document(doc), "meta", 4, NULL);
	if (!el)
		return;

	lxb_dom_element_set_attribute(el, "charset", 7, "UTF-8", 5);
	lxb_dom_node_insert_child(lxb_dom_interface_node(head), lxb_dom_interface_node(el));
}

/*
<meta http-equiv="Content-Type" content="text/html; charset=GBK">
chang to 
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
*/
void chg_charset(lxb_html_document_t* doc)
{
	lxb_html_head_element_t* head = lxb_html_document_head_element(doc);
	if (!head)
		return;

	lxb_status_t status = 0;
	lxb_dom_collection_t* collection;

	collection = get_nodes_by_tag(lxb_dom_interface_node(head), "meta");
	if (!collection)
		return;

	size_t i, value_len;
	size_t len = lxb_dom_collection_length(collection);
	lxb_dom_element_t* element = NULL;
	lxb_char_t* value = NULL;
	int found = 0;
	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(collection, i);
		value = (lxb_char_t*)lxb_dom_element_get_attribute(element, "content", 7, &value_len);
		if (!value)
			continue;
		if (strstr(value, "charset")) {
			found = 1;
			break;
		}
	}
	lxb_dom_collection_destroy(collection, true);
	if (!found)
		return;

	lxb_dom_attr_t* attr;
	attr = lxb_dom_element_attr_by_id(element, LXB_DOM_ATTR_CONTENT);
	if (!attr)
		return;

	//content="text/html; charset=GBK" changed to 
	//content="text/html; charset=UTF-8"
	char new_value[128];
	char* temp;
	strcpy(new_value, value);
	temp = strstr(new_value, "charset");
	if (!temp)
		return;
	strcpy(temp, "charset=UTF-8");
	
	status = lxb_dom_attr_set_value(attr, (const lxb_char_t*)new_value, strlen(new_value));
	if (status != LXB_STATUS_OK) {
		printf("Failed to change attribute value\n");
	}
}

extern  lxb_status_t
lxb_dom_element_qualified_name_set(lxb_dom_element_t* element,
	const lxb_char_t* prefix, size_t prefix_len,
	const lxb_char_t* lname, size_t lname_len);


/*
�� HTML5 �У������Ապϱ�ǩ������б�� / �ǿ�ѡ�ġ�Ҳ����˵��<br> �� <br /> ������Ϊ����Ч�ġ�
������Ϊ�˴���Ŀɶ��Ժͼ����ԣ�����ͳһʹ��һ�ַ��

XHTML �淶���� XHTML ����б�ǩ��������ȷ�պϣ��Ապϱ�ǩ����ʹ�� / ��β������ᱻ��Ϊ�﷨����
epub ���� XHTML �淶��������Ҫ��ȷ�պ�

HTML �е��Ապϱ�ǩ��
<br />�����з����������ı���ǿ�ƻ��С�
<hr />��ˮƽ�ָ��ߣ���ҳ���ϴ���һ��ˮƽ�ķָ��ߡ�
<img />��������ҳ���в���ͼ�񣬱������ src ����ָ��ͼ���·����
<input />���ڱ��д��������ֶΣ����ı�������򡢵�ѡ��ȡ�
<link />�����������ⲿ��Դ���� CSS �ļ���
<meta />���ṩ���� HTML �ĵ���Ԫ���ݣ������ַ����롢ҳ�������ȡ�
<area />�����ڶ���ͼ��ӳ���е�����
<base />��ָ��ҳ����������� URL �Ļ��� URL��
<col />�����ڶ������е����ԡ�
<embed />������Ƕ���ⲿӦ�ó�����ý���ļ���
<param />��Ϊ <object> �� <applet> Ԫ�ش��ݲ�����
<source />��Ϊ <video>��<audio> �� <picture> Ԫ���ṩ���ý����Դѡ�
<track />��Ϊ <video> �� <audio> Ԫ���ṩ��Ļ�������ı������
<wbr />��ָ�����ı��к��ʵĻ��е㡣

XML �е��Ապϱ�ǩ��
�� XML ��κα�ǩ���������Ապϵģ�ֻҪԪ��û�����ݡ�
���� <person age="30" />��

�������ã��滻 DOM �еķǱպϱ�ǩ
lxb_dom_document_create_element();
*/
void self_closing_replace(lxb_html_document_t* doc, lxb_tag_id_t tag)
{
	char* label = NULL;
	switch (tag) {
	case LXB_TAG_BR:
	case LXB_TAG_IMG:
	case LXB_TAG_INPUT:
		label = (char*)lxb_tag_data_by_id(tag);
		break;
	default:
		return;
	}

	lxb_status_t status;
	lxb_dom_collection_t* collection;

	collection = lxb_dom_collection_make(&doc->dom_document, 16);
	status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(doc), collection,
		(const lxb_char_t*)label, strlen(label));
	if (status != LXB_STATUS_OK) {
		lxb_dom_collection_destroy(collection, true);
		return;
	}

	size_t i;
	lxb_dom_element_t* element;
	size_t len = lxb_dom_collection_length(collection);
	lxb_dom_attr_t* attr;

	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(collection, i);

		attr = lxb_dom_attr_interface_create(element->node.owner_document);
		attr->node.ns = element->node.ns;
		lxb_dom_attr_set_name(attr, "/", 1, true);
		lxb_dom_element_attr_append(element, attr);
	}

	lxb_dom_collection_destroy(collection, true);
}

void remove_dom_element(lxb_html_document_t* doc, lxb_tag_id_t tag)
{
	switch (tag) {
	case LXB_TAG_SCRIPT:
		break;
	default:
		return;
	}

	lxb_dom_collection_t* collection;
	collection = get_nodes_by_tag(lxb_dom_interface_node(doc), "script");
	if (!collection)
		return;

	size_t i;
	size_t len = lxb_dom_collection_length(collection);
	lxb_dom_element_t* element;
	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(collection, i);
		lxb_dom_node_remove(lxb_dom_interface_node(element));
	}
	lxb_dom_collection_destroy(collection, true);
}

//for debug use, debug.html
void save_dom_debug(lxb_html_document_t* doc)
{
	FILE* fp = fopen("debug.html", "w");
	serialize_node(lxb_dom_interface_node(doc), fp);
	fclose(fp);
}
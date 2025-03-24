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
    <meta itemprop="name" content="体制老司机">

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
在 HTML5 中，对于自闭合标签，最后的斜杠 / 是可选的。也就是说，<br> 和 <br /> 都被认为是有效的。
不过，为了代码的可读性和兼容性，建议统一使用一种风格。

XHTML 规范：在 XHTML 里，所有标签都必须正确闭合，自闭合标签必须使用 / 结尾，否则会被视为语法错误。
epub 采用 XHTML 规范。所以需要正确闭合

HTML 中的自闭合标签：
<br />：换行符，用于在文本中强制换行。
<hr />：水平分隔线，在页面上创建一条水平的分割线。
<img />：用于在页面中插入图像，必须包含 src 属性指定图像的路径。
<input />：在表单中创建输入字段，像文本框、密码框、单选框等。
<link />：用于引入外部资源，如 CSS 文件。
<meta />：提供关于 HTML 文档的元数据，例如字符编码、页面描述等。
<area />：用于定义图像映射中的区域。
<base />：指定页面中所有相对 URL 的基础 URL。
<col />：用于定义表格列的属性。
<embed />：用于嵌入外部应用程序或多媒体文件。
<param />：为 <object> 或 <applet> 元素传递参数。
<source />：为 <video>、<audio> 或 <picture> 元素提供多个媒体资源选项。
<track />：为 <video> 或 <audio> 元素提供字幕或其他文本轨道。
<wbr />：指定在文本中合适的换行点。

XML 中的自闭合标签：
在 XML 里，任何标签都可以是自闭合的，只要元素没有内容。
例如 <person age="30" />。

函数作用：替换 DOM 中的非闭合标签
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
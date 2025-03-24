#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "mylib_parse.h"
#include "libs.h"
#include "webui.h"

#include <windows.h>
#include <wincrypt.h>

char error_msg[] = "不能生成章节列表";
char OK_msg[] = "转换完成";

static struct file_link list_head;
struct image_link image_head;

#define MAX_LEN (1024 * 1024)
static char chapter_titles[MAX_LEN];
static char html_content[MAX_LEN]; //one html contnet size < 1MB

static int chapter_cnt = 0;
static int chapter_cur_len = 0;

void reset_chapter_list(void)
{
	init_list(&list_head, 0);
	chapter_cnt = 0;
	chapter_cur_len = 0;

	init_img_list(&image_head);
}

void init_date_time(void)
{
	time_t current_time;
	struct tm* local_time;
	char time_str[20];

	set_novel_date(2021);
	current_time = time(NULL);
	if (current_time == -1) {
		fprintf(stderr, "Failed to get current time.\n");
		return;
	}

	local_time = localtime(&current_time);
	if (local_time == NULL) {
		fprintf(stderr, "Failed to convert to local time.\n");
		return;
	}

	if (strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time) == 0) {
		fprintf(stderr, "Failed to format time.\n");
		return;
	}

	printf("current time %s\n", time_str);
	unsigned int year = local_time->tm_year + 1900;
	set_novel_date(year);
}

void close_app(webui_event_t* e)
{
	// Close all opened windows
	webui_exit();
}

void mk_dirs(void)
{
	system("powershell -Command \"del -r temp\""); //deltet temp\ directory
	system("mkdir temp");
	system("mkdir temp\\mht");
	system("mkdir temp\\html");

	system("xcopy /s /i epub-res\\META-INF temp\\html\\META-INF");
	system("xcopy /s /i epub-res\\OEBPS temp\\html\\OEBPS");
	system("copy epub-res\\mimetype temp\\html");
}

//hh -decompile .\mht\ .\com.chm
void chm2mht(char* buf, int size, char* stitle)
{
	//create chm file
	FILE* fp = fopen("temp\\com.chm", "wb");
	if (!fp)
		return;
	fwrite(buf, size, 1, fp);
	fclose(fp);
	system("hh -decompile .\\temp\\mht\\ .\\temp\\com.chm");
}

char* get_html_body(char* buf)
{
	char* head = strstr(buf, "Content-Type");
	if (!head)
		return NULL;

	head = strstr(head, "Content-Location");
	if (!head)
		return NULL;
	while (*head != '\n')
		head++;
	head++;

	char* tail = strstr(head, "------=_NextPart");
	if (!tail)
		return NULL;
	while (*tail != '\n')
		tail--;
	*tail = 0;
	return head;
}

//http://www.vckbase.com/document/journal/vckbase43/images/comtut3pic2.jpg
void get_file_name(char* url, char* filename, char* path)
{
	char* head = url;
	head = strstr(url, ".jpg");
	if (!head)
		return;
	while (*head != '/')
		head--;
	head++;
	//head = comtut3pic2.jpg
	strcpy(filename, head);
	sprintf(path, "temp\\html\\%s", head);
}

/*
* should free memory by caller
* return string pointer if build OK
* otherwise return NULL
*/
char* build_img_src_string(char* url, char* ori)
{
	char* head;
	char* tail;
	char c;

	head = strstr(ori, url);
	if (!head)
		return NULL;
	head = head + strlen(url) + 1;
	tail = strstr(head, "------=_NextPart");
	if (!tail)
		return NULL;
	tail--;
	c = *tail;
	*tail = 0;

	size_t len = strlen(head) + 1;
	char* buf = malloc(len + 128);
	if (!buf) {
		*tail = c;
		return NULL;
	}

	//data:image/jpg;base64, 'base64 data'
	sprintf(buf, "data:image/jpg;base64, %s ", head);

#if 0
	//save base64 figure to file
	len = strlen(head);
	char filename[256];
	char path[256];
	get_file_name(url, filename, path);

	unsigned long decode_len;
	unsigned char* decode = decode_base64(head, &decode_len);
	if (decode) {
		FILE* fp = fopen(path, "wb");
		if (!fp)
			return NULL;
		fwrite(decode, 1, decode_len, fp);
		free(decode);
	}
	strcpy(buf, filename);
#endif

	*tail = c;
	return buf;
}

/*
* 
1. <head> only save title
2. <img xxx > to <img xxx />
3. <br> to <br/>
4. remove <script> element

*/
void epub_handle(lxb_html_document_t* doc)
{
	self_closing_replace(doc, LXB_TAG_BR);
	self_closing_replace(doc, LXB_TAG_IMG);
	self_closing_replace(doc, LXB_TAG_INPUT);
	remove_dom_element(doc, LXB_TAG_SCRIPT);
}

/*
<img src="http://www.vckbase.com/document/journal/vckbase43/images/stmtutpic1.jpg" border=0>
changed to
<img src="data:image/jpg;base64, 'base64 data' " border=0>
return new html

html: decoded html content
ori: orignal mht content
size: in/out
return html DOM
*/
lxb_html_document_t* replace_pics(unsigned char* html, char* ori, unsigned long size)
{
	lxb_html_document_t* doc = NULL;
	doc = parser_init(html, size);
	if (!doc)
		return NULL;

	//epub special handle
	epub_handle(doc);

	lxb_dom_collection_t* imgs;
	imgs = get_nodes_by_tag(lxb_dom_interface_node(doc), "img");
	if (!imgs) { //no images
		return doc;
	}

	size_t len = lxb_dom_collection_length(imgs);
	size_t i, val_len;
	lxb_dom_element_t* element;
	lxb_dom_attr_t* attr;
	lxb_status_t status = 0;
	lxb_char_t* value;
	lxb_char_t* new_value;

	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(imgs, i);
		//<img src="http://www.vckbase.com/document/journal/vckbase43/images/stmtutpic1.jpg" border=0>
		attr = lxb_dom_element_attr_by_id(element, LXB_DOM_ATTR_SRC);
		if (!attr)
			continue;
		value = (lxb_char_t*)lxb_dom_attr_value(attr, &val_len);
		if (!value)
			continue;

		new_value = build_img_src_string(value, ori);
		if (!new_value)
			continue;

		status = lxb_dom_attr_set_value(attr, (const lxb_char_t*)new_value, strlen(new_value));
		free(new_value);
		if (status != LXB_STATUS_OK) {
			printf("Failed to change attribute value\n");
		}
	}
	lxb_dom_collection_destroy(imgs, true);

	return doc;
}

/*
Content-Transfer-Encoding: quoted-printable
Content-Transfer-Encoding: base64
*/ 
int is_base64_encode(char* buf)
{
	int ret = 1;
	char* head = strstr(buf, "Content-Transfer-Encoding");
	if (!head)
		return 1;
	head = head + strlen("Content-Transfer-Encoding") + 1;

	char encode_str[128];
	sscanf(head, "%s", encode_str);
	if (!strcmp(encode_str, "base64"))
		return 1;
	printf("Content-Transfer-Encoding: %s\n", encode_str);
	return 0;
}

void write_html_file(char* html_name, char* title, char* out, int out_len)
{
	FILE* fresult = fopen(html_name, "w");
	if (!fresult)
		return;

	fprintf(fresult, "%s\n", novel_content_head);

	//<head><title>COM 组件设计与应用（一）--雪尽马蹄轻</title>
	if (title) {
		int len;
		char* titleutf = to_utf(title, &len);
		fprintf(fresult, "<head>\n<title>\n%s\n</title>\n", titleutf);
		free(titleutf);
	} else {
		fprintf(fresult, "<head>\n<title>\ntitle\n</title>\n");
	}
	fprintf(fresult, "</head>\n\n");

	//body
	fwrite(out, 1, out_len - 1, fresult);
	//tail
	fprintf(fresult, "</html>");
	fclose(fresult);
}

//mht to html
void mht2html(char* mht_name, char* html_name)
{
	char* read_buf = NULL;
	unsigned char* decode = NULL;
	lxb_html_document_t* doc = NULL;

	//1. read mht content to read_buf[]
	int file_len;
	read_buf = read_file_content(mht_name, &file_len);
	if (!read_buf) {
		goto exit;
	}

	int base_encode = is_base64_encode(read_buf);

	//2. find html body and decode
	char* html = get_html_body(read_buf);
	if (!html)
		goto exit;

	unsigned long size;
	if(base_encode)
		decode = decode_base64(html, &size);
	else
		decode = decode_quoted(html, &size);
	if (!decode)
		goto exit;
	
	//3. replace pictures from url to base64 format
	char* left = html + strlen(html) + 1;
	doc = replace_pics(decode, left, size);
	if (!doc)
		goto exit;
 
	//4. DOM to html
	char* title;
	size_t title_len;
	title = (char*)lxb_html_document_title(doc, &title_len);
	lxb_html_body_element_t* body;
	body = lxb_html_document_body_element(doc);
	serialize_node_buf(lxb_dom_interface_node(body), html_content, sizeof(html_content));

	//5. html content to utf8
	int out_len;
	char* out;
	out = to_utf(html_content, &out_len);
	if (!out)
		goto exit;

	//6. write to file
	write_html_file(html_name, title, out, out_len);
	free(out);

exit:
	if (read_buf) //mht content
		free(read_buf);
	if(decode) //decoded html part of mht file
		free(decode);
	if(doc) //decoded html to DOM
		parse_exit(doc);
}


//temp\mht\img\21.1.gif to temp\html\OEBPS\img\21.1.gif
//<img border="0" src="img/21.1.gif">
void copy_pics(char* path)
{
	char cmd[1024];
	char full_path[256];
	char* head;

	//src="http://wxx/stmtutpic1.jpg"
	if (strstr(path, "http://"))
		return;
	if (strstr(path, "https://"))
		return;

	insert_img_list(&image_head, path);
	//local pics
	head = strchr(path, '/');
	if (head) {
		*head = 0;
		sprintf(full_path, "temp\\html\\OEBPS\\%s", path);
		if (!DirectoryExists(full_path)) {
			sprintf(cmd, "xcopy /s /i temp\\mht\\%s temp\\html\\OEBPS\\%s", path, path);
			printf("%s\n", cmd);
			system(cmd);
		}
		*head = '/';
	}
	
}

//<img border="0" src="img/21.1.gif">
void copy_html_pics(lxb_html_document_t* doc)
{
	lxb_dom_collection_t* imgs;
	imgs = get_nodes_by_tag(lxb_dom_interface_node(doc), "img");
	if (!imgs) { //no images
		return;
	}

	size_t len = lxb_dom_collection_length(imgs);
	size_t i, val_len;
	lxb_dom_element_t* element;
	lxb_char_t* value;

	for (i = 0; i < len; i++) {
		element = lxb_dom_collection_element(imgs, i);
		//<img border="0" src="img/21.1.gif">
		value = (lxb_char_t*)lxb_dom_element_get_attribute(element, "src", 3, &val_len);
		if (!value)
			continue;
		copy_pics(value);
	}
	lxb_dom_collection_destroy(imgs, true);
}

void html2good(char* path, char* html_name)
{
	char* read_buf = NULL;
	lxb_html_document_t* doc = NULL;

	//1. read html file to read_buf
	int file_len;
	read_buf = read_file_content(path, &file_len);
	if (!read_buf) {
		goto exit;
	}

	//2. html to DOM
	doc = parser_init(read_buf, file_len);
	if (!doc)
		goto exit;

	//3. epub special handle
	epub_handle(doc);
	copy_html_pics(doc);

	//4. DOM to html
	char* title;
	size_t title_len;
	title = (char*)lxb_html_document_title(doc, &title_len);
	lxb_html_body_element_t* body;
	body = lxb_html_document_body_element(doc);
	serialize_node_buf(lxb_dom_interface_node(body), html_content, sizeof(html_content));

	//5. to utf8
	int out_len;
	char* out;
	out = to_utf(html_content, &out_len);
	if (!out)
		return;

	//6. write to file
	write_html_file(html_name, title, out, out_len);
	free(out);

exit:
	if (read_buf) //html content
		free(read_buf);
	if (doc) //decoded html to DOM
		parse_exit(doc);
}

//fill all chapter name to out
void fill_chapter_string(char* chapter_name, char* out, int out_size)
{
	char* cur;

	int name_len = (int)strlen(chapter_name);
	if ((chapter_cur_len + name_len + 2) >= out_size) {
		printf("too many chapters\n");
		return;
	}

	cur = out + chapter_cur_len;
	memcpy(cur, chapter_name, name_len);
	chapter_cur_len = chapter_cur_len + name_len + 1; //include CR
	cur[name_len] = '\n';
	cur[name_len + 1] = 0;
}

//COM%20组件设计与应用（一）.mht to COM 组件设计与应用（一）.mht
//ori to out
void remove_special_char_path(char* ori, char* out)
{
	char* head;
	head = strstr(ori, "%20");
	if (head) {
		char* tail;
		tail = head + strlen("%20");
		*head = 0;
		sprintf(out, "%s %s", ori, tail);
		return;
	}
	//keep original
	strcpy(out, ori);
}

/*
* fill buf with chapter list
<HTML><HEAD>
</HEAD><BODY><UL>

<LI><OBJECT TYPE="text/sitemap">
<PARAM NAME="Name" VALUE="COM 组件设计与应用（一）">
<PARAM NAME="Local" VALUE="COM%20组件设计与应用（一）.mht">
</OBJECT>
<LI><OBJECT TYPE="text/sitemap">
<PARAM NAME="Name" VALUE="COM组件设计与应用（二）">
<PARAM NAME="Local" VALUE="COM组件设计与应用（二）.mht">
</OBJECT>

</UL></BODY></HTML>
*/
void get_chapter_list(char* chapters, int len, char* title)
{
	int file_size = 0;
	char* buf = NULL;
	char* utf_buf = NULL;
	lxb_html_document_t* doc = NULL;

	reset_chapter_list();

	//temp\\mht\\COM组件设计与应用.hhc
	char file_name[256];
	int ret = find_files("temp\\mht", ".hhc", file_name);
	if (ret)
		return;
	char path[512];
	sprintf(path, "temp\\mht\\%s", file_name);
	buf = read_file_content(path, &file_size);
	if (!buf)
		return;
	utf_buf = to_utf(buf, &file_size);
	if (!utf_buf)
		goto exit;

	doc = parser_init(utf_buf, file_size);
	if (!doc)
		goto exit;

	lxb_dom_collection_t* li;
	li = get_nodes_by_tag(lxb_dom_interface_node(doc), "LI");
	if (!li) {
		goto exit;
	}

	size_t number = lxb_dom_collection_length(li);
	size_t i, val_len;
	lxb_dom_element_t* element;
	lxb_dom_element_t* cur;
	lxb_char_t* value;
	lxb_char_t* real_path;
	char path_final[256];

	sprintf(path, "total %d chapters \n", (int)number);
	fill_chapter_string(path, chapters, len);

	for (i = 0; i < number; i++) {
		element = lxb_dom_collection_element(li, i);
		//<PARAM NAME="Name" VALUE="COM 组件设计与应用（一）">
		cur = find_sub_attr(element, "PARAM", "NAME", "name");
		if (!cur)
			continue;
		value = (lxb_char_t*)lxb_dom_element_get_attribute(cur, "VALUE", 5, &val_len);
		if (!value)
			continue;

		//<PARAM NAME="Local" VALUE="COM%20组件设计与应用（一）.mht">
		cur = find_sub_attr(element, "PARAM", "NAME", "Local");
		if (!cur)
			continue;
		real_path = (lxb_char_t*)lxb_dom_element_get_attribute(cur, "VALUE", 5, &val_len);
		if (!real_path)
			continue;
		/*
		<param name="Name" value="卷之一">
		<param name="Local" value="">
		*/
		if (0 == val_len)
			continue; //ignore it
		chapter_cnt++;
		fill_chapter_string(value, chapters, len);
		remove_special_char_path(real_path, path_final);
		insert_list(&list_head, value, path_final);
	}

	lxb_dom_collection_destroy(li, true);

exit:
	if(buf)
		free(buf);
	if (utf_buf)
		free(utf_buf);
	if(doc)
		parse_exit(doc);
}

int is_html_file(char* path)
{
	if (strstr(path, "htm"))
		return 1;
	if (strstr(path, "html"))
		return 1;
	return 0;
}

void convert_to_htmls(void)
{
	struct file_link* cur;
	struct file_link* head = &list_head;

	char src_name_utf[256];
	char dst_name_utf[256];
	char src_name[256];
	char dst_name[256];
	int out_len;

	/*
	* 在Windows系统中，文件名是以UTF-16编码存储的。
	虽然Windows API支持宽字符文件名，但标准的 fopen 函数并不直接支持UTF-8编码的文件名。
	为了在Windows上打开UTF-8编码的文件名，你需要将UTF-8编码的文件名转换为宽字符（UTF-16）格式，然后使用 _wfopen 函数
	这里直接转换为 ascii
	*/
	int html_flag = 0;
	list_for_each_entry(cur, &head->list, list, struct file_link) {
		sprintf(src_name_utf, "temp\\mht\\%s", cur->path);
		sprintf(dst_name_utf, "temp\\html\\OEBPS\\chapter%d.html", cur->chapter + 1);
		u8_to_gb2312(src_name_utf, src_name, &out_len);
		u8_to_gb2312(dst_name_utf, dst_name, &out_len);
		if (cur->repeated)
			continue;
		printf("start to convert %s\n", src_name);
		html_flag = is_html_file(cur->path);
		if(html_flag)
			html2good(src_name, dst_name);
		else
			mht2html(src_name, dst_name);
		printf("end to convert %s\n\n", dst_name);
	}
}

//list_only = 1, only get chapter list
int epub_convert(char* buf, int size, char* stitle, char* author, int list_only)
{
	int ret = 0;
	set_novel_name(stitle);

	mk_dirs();

	//1. chm to mht
	chm2mht(buf, size, stitle);

	get_chapter_list(chapter_titles, MAX_LEN, stitle);
	if (list_only) {
		if (chapter_cnt == 0)
			return -1;
		return 0;
	}

	//2. mht to html
	convert_to_htmls();

	//3. html to epub
	gen_book_id();
	output_book_toc(buf, stitle, &list_head);
	output_cover(buf, stitle);
	output_ncx(buf, stitle, &list_head);
	output_opf(buf, stitle, &list_head);

	destroy_list(&list_head);
	destroy_img_list(&image_head);

	output_epub(stitle);
		return 0;
}

//ascii msg to utf8
void webui_return_utf(webui_event_t* e, char* msg)
{
	char* temp;
	int out_len;
	temp = to_utf(msg, &out_len);
	if (temp) {
		webui_return_string(e, temp);
		free(temp);
	} else {
		webui_return_string(e, msg);
	}
}


//JavaScript: list_chapters_call_c(contents, file_size, title, author, flag)
void list_chapters_call_c(webui_event_t* e)
{
	char* content = (char*)webui_get_string_at(e, 0);
	int size = (int)webui_get_int_at(e, 1);
	char* title = (char*)webui_get_string_at(e, 2);
	char* author = (char*)webui_get_string_at(e, 3);
	int flag = (int)webui_get_int_at(e, 4);

	set_novel_author(author);

	int ret = epub_convert(content, size, title, author, flag);
	// Send back the response to JavaScript
	
	if (ret < 0) {
		webui_return_utf(e, error_msg);
	}
	else {
		if (flag == 1)
			webui_return_string(e, chapter_titles);
		else
			webui_return_utf(e, OK_msg);
	}
}

void webui_init(void)
{
	// Create a new window
	size_t MainWindow = webui_new_window();

	// Set the root folder for the UI
	//webui_set_root_folder(MainWindow, ".");

	// Bind HTML elements with the specified ID to C functions
	webui_bind(MainWindow, "close_app", close_app);
	webui_bind(MainWindow, "list_chapters_call_c", list_chapters_call_c);

	// Show the window, preferably in a chromium based browser
	if (!webui_show_browser(MainWindow, "chm2epub.html", AnyBrowser))
		webui_show(MainWindow, "chm2epub.html");

	// Wait until all windows get closed
	webui_wait();
	// Free all memory resources (Optional)
	webui_clean();
}

int main(int argc, char** argv)
{
	init_date_time();
	webui_init();
	return 0;
}

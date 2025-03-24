#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "libs.h"

static char novel_name[MAX_CHAPTER_NAME_LEN]; //novel name
static char author_name[MAX_CHAPTER_NAME_LEN]; //novel name 
static char book_id[MAX_CHAPTER_NAME_LEN];
static unsigned int novel_date = 2021; //current year
int utf_flag = 1; //default is utf8

static char tmp_path[] = "temp";
static char full_path[MAX_CHAPTER_NAME_LEN]; //save full path
static char full_prefix[MAX_CHAPTER_NAME_LEN]; //save full path

void set_charset(int flag)
{
	utf_flag = flag;
}

int get_charset(void)
{
	return utf_flag;
}

char* get_full_prefix(void)
{
#ifdef WINDOWS
	sprintf(full_prefix, ".\\temp");
#else
	sprintf(full_prefix, "./temp");
#endif
	return full_prefix;
}

char* get_full_path(char* file_name)
{
	sprintf(full_path, ".\\temp\\html\\OEBPS\\%s", file_name);
	return full_path;
}

void set_novel_name(char* name)
{
	strcpy(novel_name, (const char*)name);
}

char* get_novel_name(void)
{
	return novel_name;
}

void set_novel_author(char* author)
{
	strcpy(author_name, (const char*)author);
}

char* get_novel_author(void)
{
	return author_name;
}

void set_novel_date(unsigned int year)
{
	novel_date = year;
}

unsigned int get_novel_date(void)
{
	return novel_date;
}

//8 bytes random: e.g. fe1c73d3
void gen_book_id(void)
{
	const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	const int charsetLength = sizeof(charset) - 1;
	int length = 8;
	int i, index;

	srand((unsigned int)time(NULL));
	for (i = 0; i < length; i++) {
		index = rand() % charsetLength;
		book_id[i] = charset[index];
	}
	book_id[length] = '\0';
	printf("book id is %s\n", book_id);
}

char* get_book_id(void)
{
	return book_id;
}

//book-toc.html, toc.ncx
/*
book-toc.html
*/
int output_book_toc(char* buf, char* title, struct file_link* head)
{
	struct file_link* cur;
	FILE* fp_toc_html = NULL;
	char* path;

	path = get_full_path("book-toc.html");
	fp_toc_html = fopen(path, "w");
	if (!fp_toc_html)
		return -1;

	fprintf(fp_toc_html, "%s\n", toc_html_head);
	char* temp = NULL;
	int append = 0;
	list_for_each_entry(cur, &head->list, list, struct file_link) {
		//<dt class = "tocl2"><a href = "chapter1.html">xxx</a></dt>
		append = 0;
		if (cur->repeated){
			temp = strchr(cur->path, '#');
			if (temp)
				append = 1;
		}

		if(append){
			fprintf(fp_toc_html,
				"<dt class=\"tocl2\"><a href=\"chapter%d.html%s\">%s</a></dt>\n",
				cur->chapter + 1, temp, cur->chapter_name);
		} else {
			fprintf(fp_toc_html,
				"<dt class=\"tocl2\"><a href=\"chapter%d.html\">%s</a></dt>\n",
				cur->chapter + 1, cur->chapter_name);
		}
	}

	fprintf(fp_toc_html, "%s", toc_html_tail);
	fclose(fp_toc_html);

	return 0;
}

//use default cover.html
int output_cover(char* buf, char* title)
{
	return 0;
}

int output_ncx(char* buf, char* title, struct file_link* head)
{
	struct file_link* cur;
	FILE* fp_ncx_html = NULL;
	char* path;

	path = get_full_path("toc.ncx");
	fp_ncx_html = fopen(path, "w");
	if (!fp_ncx_html)
		return -1;

	//ncx content
	fprintf(fp_ncx_html, "%s\n", toc_ncx_head);
	//<meta name=\"dtb:uid\" content=\"easypub-fe1c73d3\" />
	fprintf(fp_ncx_html,
		"<meta name=\"dtb:uid\" content=\"easypub-%s\" />\n", get_book_id());
	//<meta name = "dtb:depth" content = "1" />
	fprintf(fp_ncx_html,
		"<meta name=\"dtb:depth\" content=\"1\"/>\n");

	fprintf(fp_ncx_html, "%s", toc_ncx_mid);
	//<text>novel name</text>
	fprintf(fp_ncx_html, "<text>%s</text>\n", get_novel_name());
	/*
	 </docTitle>  \n\
	<docAuthor>  \n\
	<text>author</text>  \n\
	</docAuthor>  \n\
	*/
	fprintf(fp_ncx_html, "</docTitle>\n<docAuthor>\n<text>%s</text>\n</docAuthor>\n\n", get_novel_author());

	//rest of part
	fprintf(fp_ncx_html, "%s\n", toc_ncx_content);

	char* temp = NULL;
	int append = 0;
	list_for_each_entry(cur, &head->list, list, struct file_link) {
		append = 0;
		if (cur->repeated) {
			temp = strchr(cur->path, '#');
			if (temp)
				append = 1;
		}
		/*
			<navPoint id="chapter0" playOrder="1">
			<navLabel><text>序</text></navLabel>
			<content src="chapter0.html"/>
			</navPoint>
		*/
		fprintf(fp_ncx_html, "<navPoint id=\"chapter%d\" playOrder=\"%d\">\n",
			cur->chapter + 1, cur->chapter + 3);
		fprintf(fp_ncx_html, "<navLabel><text>%s</text></navLabel>\n",
			cur->chapter_name);
		if (append) {
			fprintf(fp_ncx_html, "<content src=\"chapter%d.html%s\"/>\n",
				cur->chapter + 1, temp);
		} else {
			fprintf(fp_ncx_html, "<content src=\"chapter%d.html\"/>\n",
				cur->chapter + 1);
		}
		fprintf(fp_ncx_html, "</navPoint>\n\n");
	}

	fprintf(fp_ncx_html, "</navMap>\n");
	fprintf(fp_ncx_html, "</ncx>\n");

	fclose(fp_ncx_html);
	return 0;
}

extern struct image_link image_head;
//content.opf
int output_opf(char* buf, char* title, struct file_link* head)
{
	struct file_link* cur;
	FILE* fp_opf = NULL;
	int i, count = 0;
	char* path;
	char file_name[MAX_CHAPTER_NAME_LEN];

	sprintf(file_name, "content.opf");
	path = get_full_path(file_name);
	fp_opf = fopen(path, "w");
	if (!fp_opf)
		return -1;

	fprintf(fp_opf, "%s\n", opf_head);
	//<dc:identifier id=\"bookid\">easypub-fe1c73d3</dc:identifier>
	fprintf(fp_opf, "<dc:identifier id=\"bookid\">easypub-%s</dc:identifier>\n", get_book_id());

	//<dc:title>title</dc:title>
	//<dc:creator opf:role="aut">a</dc:creator>
	//<dc:date>2021 </dc:date>
	fprintf(fp_opf, "<dc:title>%s</dc:title>\n", get_novel_name());
	fprintf(fp_opf, "<dc:creator opf:role=\"aut\">%s</dc:creator>\n", get_novel_author());
	fprintf(fp_opf, "<dc:date>%d</dc:date>\n", get_novel_date());

	fprintf(fp_opf, "%s\n", opf_mid);
	/*
	* 在这里，插入所有需要用到的图片，如果不插入，则 iPad 的图书 app 不能正确显示图片
	<item id="gif-image" href="img/1.1.gif" media-type="image/gif"/>
	<item id="gif-image" href="img/1.2.gif" media-type="image/gif"/>
	*/
	struct image_link* image;
	int index = 0;
	list_for_each_entry(image, &image_head.list, list, struct image_link) {
		fprintf(fp_opf,
			"<item id=\"gif-image%d\" href=\"%s\" media-type=\"image/gif\"/>\n",
			index, image->path);
		index++;
	}

	char* temp = NULL;
	int append = 0;
	//<item id="chapter0" href="chapter0.html" media-type="application/xhtml+xml"/>
	list_for_each_entry(cur, &head->list, list, struct file_link) {

		append = 0;
		if (cur->repeated) {
			temp = strchr(cur->path, '#');
			if (temp)
				append = 1;
		}
		if (append) {
			fprintf(fp_opf, "<item id=\"chapter%d%s\" href=\"chapter%d.html%s\" media-type=\"application/xhtml+xml\"/>\n",
				cur->chapter + 1, temp + 1, cur->chapter + 1, temp);
		} else {
			fprintf(fp_opf, "<item id=\"chapter%d\" href=\"chapter%d.html\" media-type=\"application/xhtml+xml\"/>\n",
				cur->chapter + 1, cur->chapter + 1);
		}
		count++;
	}

	fprintf(fp_opf, "</manifest>\n");
	fprintf(fp_opf, "<spine toc=\"ncxtoc\">\n");
	//<itemref idref="cover" linear="no"/>
	fprintf(fp_opf, "<itemref idref=\"cover\" linear=\"no\"/>\n");
	//<itemref idref="htmltoc" linear="yes"/>
	fprintf(fp_opf, "<itemref idref=\"htmltoc\" linear=\"yes\"/>\n");
	//<itemref idref="chapter0" linear="yes"/>
	for (i = 0; i < count; i++) {
		fprintf(fp_opf, "<itemref idref=\"chapter%d\" linear=\"yes\"/>\n", i + 1);
	}
	fprintf(fp_opf, "%s\n", opf_tail);

	fclose(fp_opf);
	return 0;
}

void exec_utf8_cmd(char* cmd)
{
	char outbuf[512];
	int outlen;
	u8_to_gb2312(cmd, outbuf, &outlen);
	printf("%s\n", outbuf);
	system(outbuf);
}

//output epub file
void output_epub(char* title)
{
	char* path;
	char cmd[256];

	path = get_full_path("content");
	printf("path is %s\n", path);

	//del old file
	sprintf(cmd, "del \"%s\".epub", get_novel_name());
	exec_utf8_cmd(cmd);

	//creat epub
	//>.\7z.exe  a -tzip 1.epub temp\html
	sprintf(cmd, ".\\7z.exe a -tzip \"%s\".epub temp\\html", get_novel_name());
	exec_utf8_cmd(cmd);
}

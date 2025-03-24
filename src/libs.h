#ifndef _LIBS_H
#define _LIBS_H

#include "list.h"

#define MAX_LINK_LEN 255
#define MAX_CHAPTER_NAME_LEN 255
struct file_link{
	struct list_head list; //linux list

	int chapter; //chapter index, 0-based
	int repeated; //default is 0
	char chapter_name[MAX_CHAPTER_NAME_LEN];
	char path[MAX_CHAPTER_NAME_LEN];
};

struct image_link {
	struct list_head list; //linux list
	char path[MAX_CHAPTER_NAME_LEN];
};

extern char chapter0_name[];
extern char *default_regex_str;
//book-toc.html
extern const char toc_html_head[];
extern const char toc_html_tail[];
//toc.ncx
extern const char toc_ncx_head[];
extern const char toc_ncx_mid[];
extern const char toc_ncx_content[];

//content.opf
extern const char opf_head[];
extern const char opf_mid[];
extern const char opf_tail[];

//html content
extern const char novel_content_head[];
extern const char novel_content_mid[];

//return 1 if utf8; otherwise return 0
extern int text_charset(char* buf, int size);

extern void set_charset(int flag);
extern int get_charset(void);

extern void parse_title_name(char* path, char* title);
extern void set_novel_name(char* name);
extern char* get_novel_name(void);

extern void set_novel_author(char* author);
extern char* get_novel_author(void);

extern void set_novel_date(unsigned int year);
extern unsigned int get_novel_date(void);

extern void gen_book_id(void);
extern char* get_book_id(void);

extern void init_list(struct file_link* head, size_t total_size);

//name: chapter name
//path: file path
extern void insert_list(struct file_link* head, char* name, char* path);

extern void destroy_list(struct file_link* head);

extern void init_img_list(struct image_link* head);
extern void insert_img_list(struct image_link* head, char* path);
extern void destroy_img_list(struct image_link* head);

//win7 console: couldn't output the UTF-8 string
//so convert utf-8 to gb2312 for debugging
extern void FWRITE_IGNORE(char* buf);

//save result to out
extern int u8_to_gb2312(char* in, char* out, int* out_len);

//gb2312 to utf8, should free memory by caller
extern char* to_utf(char* in, int* out_len);

//base64 to ascii, malloc buffer
//should free memory by caller
extern unsigned char* decode_base64(char* str, unsigned long* size);

//quoted to ascii, malloc buffer
//should free memory by caller
//Content-Transfer-Encoding: quoted-printable
extern unsigned char* decode_quoted(char* str, unsigned long* size);

extern int file_size(char* path);

/* if local is UTF-8, return 0
	else return 1(GB2312)
*/
extern unsigned int get_local_charset(void);

extern int FileExists(const char* fileName);

extern int DirectoryExists(const char* dirPath);


//find first file name of match match_str
//return 0 if OK
extern int find_files(const char* dirPath, char* match_str, char* file_name);

//read file content to buf
//call free(buf) after use
extern char* read_file_content(char* path, int* fsize);

extern void convert_file_utf(char* filename);

extern int build_toc(char* buf, long size, struct file_link* head);
extern int output_book_toc(char* buf, char* title, struct file_link* head);
extern int output_cover(char* buf, char* title);
extern int output_ncx(char* buf, char* title, struct file_link* head);
extern int output_opf(char* buf, char* title, struct file_link* head);
extern int output_html(char* buf, char* title, struct file_link* head);
extern void output_epub(char* title);

#endif

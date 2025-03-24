#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <icu.h>

#include "libs.h"
#if defined(WINDOWS)
#include "dirent_win.h"
#else
#include <dirent.h>
#endif

void init_list(struct file_link* head, size_t total_size)
{
	memset(head, 0, sizeof(struct file_link));
	head->chapter = -1;
	INIT_LIST_HEAD(&head->list);
}

/*
<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="第一部分 程序员必读">
		<param name="Local" value="1.htm">
		</OBJECT>
	<UL>
		<LI> <OBJECT type="text/sitemap">
			<param name="Name" value="第1章 对程序错误的处理">
			<param name="Local" value="1.htm">
			</OBJECT>
		<UL>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="1.1 定义自己的错误代码">
				<param name="Local" value="1.htm#01">
				</OBJECT>
			<LI> <OBJECT type="text/sitemap">
				<param name="Name" value="1.2 ErrorShow示例应用程序">
				<param name="Local" value="1.htm#02">
				</OBJECT>
		</UL>
URL 中 “#” 后面的部分被称为锚点（fragment identifier），它主要用于定位网页内的特定位置，
当浏览器加载包含锚点的 URL 时，如果网页存在对应的锚点元素（比如带有特定 id 的 HTML 元素），
会自动滚动到该位置。例如在网页中有<p id="03">这是一段内容</p>，
访问 “5.htm#03” 就可能会让页面滚动到这个<p>标签对应的位置 。

1.htm#01
<a name="01"></a>1.1 定义自己的错误代码<p>

1.htm#02
<a name="02"></a>1.2 ErrorShow示例应用程序<p>
*/
static int list_check_repeated(struct file_link* head, char* path)
{
	struct file_link* cur;
	struct file_link* n;

	char* temp;
	//list_for_each_entry(cur, &head->list, list, struct file_link) {
	list_for_each_entry_safe(cur, n, &head->list, list, struct file_link) {
		if(!strcmp(cur->path, path))
			return 1;
		//1.html vs 1.htm#01
		if (strstr(path, cur->path)) {
			temp = path + strlen(cur->path);
			if (*temp == '#')
				return 1;
		}
	}
	return 0;
}

//attach to tail
void insert_list(struct file_link* head, char* name, char* path)
{
	struct file_link* node;
	struct file_link* pre;

	node = malloc(sizeof(struct file_link));
	if (!node)
		return;

	strcpy(node->chapter_name, name);
	strcpy(node->path, path);
	node->repeated = list_check_repeated(head, path);

	list_add_tail(&node->list, &head->list);
	pre = (struct file_link*)list_prev_entry(node, struct file_link, list);
	//calc chapter length
	if(!node->repeated)
		node->chapter = pre->chapter + 1;
	else
		node->chapter = pre->chapter;
}


void destroy_list(struct file_link* head)
{
	struct file_link* cur;
	struct file_link* n;

	//struct list_head* pos;
	//list_for_each(pos, &head->list) {

	//list_for_each_entry(cur, &head->list, list, struct file_link) {
	list_for_each_entry_safe(cur, n, &head->list, list, struct file_link) {
		list_del(&cur->list);
		free(cur);
	}
}


void init_img_list(struct image_link* head)
{
	memset(head, 0, sizeof(struct image_link));
	INIT_LIST_HEAD(&head->list);
}

//attach to tail
void insert_img_list(struct image_link* head, char* path)
{
	struct image_link* node;
	node = malloc(sizeof(struct image_link));
	if (!node)
		return;
	strcpy(node->path, path);
	list_add_tail(&node->list, &head->list);
}

void destroy_img_list(struct image_link* head)
{
	struct image_link* cur;
	struct image_link* n;

	list_for_each_entry_safe(cur, n, &head->list, list, struct image_link) {
		list_del(&cur->list);
		free(cur);
	}
}

/* single list with head node: end */

#ifdef WINDOWS
#include <Windows.h>
#include <lm.h>

int u8_to_gb2312(char* in, char* out, int* out_len)
{
	char* mid;
	int len;

	//convert an MBCS string to widechar, in to mid
	len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)in, -1, NULL, 0);
	len = len * sizeof(wchar_t);
	mid = malloc(len);
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)in, -1, (LPWSTR)mid, len);

	//mid to out
	len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)mid, len, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)mid, -1, (LPSTR)out, len, NULL, NULL);

	free(mid);
	*out_len = len;
	out[len] = 0;
	return 0;
}

//utf_content = to_utf(buf, size, &realsize);
//gb2312 to utf8, should free memory by caller
char* to_utf(char* in, int* out_len)
{
	char* mid;
	char* out;
	int len;

	//in to mid
	//Maps a character string to a UTF-16 (wide character) string
	len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)in, -1, NULL, 0);
	len = len * sizeof(wchar_t);
	mid = malloc(len);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)in, -1, (LPWSTR)mid, len);

	//mid to out
	//Maps a UTF-16 (wide character) string to a new character string. 
	len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)mid, -1, NULL, 0, NULL, NULL);
	out = malloc(len + 1);
	WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)mid, -1, (LPSTR)out, len, NULL, NULL);

	free(mid);
	*out_len = len;
	out[len] = 0;
	return out;
}

/* if local is UTF-8, return 0
	else return 1(GB2312)
*/
unsigned int get_local_charset(void)
{
	//CPINFOEX info;
	//GetCPInfoEx (CP_ACP, 0, &info);
	if (GetACP() == 936) { //CP936 is GB2312
		return 1;
	}
	return 0;
}

#endif

void FWRITE_IGNORE(char* buf)
{
#ifdef WINDOWS
	char outbuf[512];
	int outlen;
	u8_to_gb2312(buf, outbuf, &outlen);
	fprintf(stdout, "%s", outbuf);
#else
	fwrite(buf, 1, count, stdout);
#endif
	printf("\n");
}

void parse_title_name(char* path, char* title)
{
	char* head = path;
	char* last_head = path;

	strcpy(title, "novel");//default is novel
	while (head) {
		//find last \ or /
#ifdef WINDOWS
		head = strchr(head, '\\');
#else
		head = strchr(head, '/');
#endif
		if (head) {
			head++; //skip /
			last_head = head;
		}
	}

	head = strstr(last_head, ".txt");
	if (!head)
		return;
	*head = 0;
	strcpy(title, last_head);
	*head = '.';

	printf("path is %s\n", path);
	printf("novel name is %s\n", title);
}
	
//return 1 if utf8; otherwise return 0
int text_charset(char* buf, int size)
{
	UErrorCode status = U_ZERO_ERROR;
	UCharsetDetector* csd = ucsdet_open(&status);
	if (status)
		return 1;
	ucsdet_setText(csd, (const char*)buf, size, &status);
	if (status)
		return 1;
	int32_t matchCount;
	const UCharsetMatch** csm = ucsdet_detectAll(csd, &matchCount, &status);
	if (status)
		return 1;
	UErrorCode err = U_ZERO_ERROR;
	const char* name = ucsdet_getName(csm[0], &err);
	//const char* lang = ucsdet_getLanguage(csm[0], &err);
	//int32_t confidence = ucsdet_getConfidence(csm[0], &err);
	if(err != U_ZERO_ERROR)
		return 1;

	printf("txt file character set is %s\n", name);
	if (strcmp(name, "UTF-8"))
		return 0;
	
	return 1;
}

//malloc buf
unsigned char* decode_base64(char* str, unsigned long* size)
{
	// 计算解码后的数据大小
	DWORD binarySize = 0;
	if (!CryptStringToBinaryA(
		str,          // Base64 字符串
		(DWORD)strlen(str),  // 字符串长度
		CRYPT_STRING_BASE64, // 输入格式为 Base64
		NULL,               // 不需要输出缓冲区
		&binarySize,        // 获取解码后数据的大小
		NULL,               // 不需要跳过的字符
		NULL)) {            // 不需要格式
		printf("Failed to calculate binary size. Error: %lu\n", GetLastError());
		return NULL;
	}

	// 分配缓冲区以存储解码后的数据
	BYTE* binaryData = (BYTE*)malloc(binarySize);
	if (!binaryData) {
		printf("Failed to allocate memory.\n");
		return NULL;
	}

	// 解码 Base64 字符串
	if (!CryptStringToBinaryA(
		str,          // Base64 字符串
		(DWORD)strlen(str),  // 字符串长度
		CRYPT_STRING_BASE64, // 输入格式为 Base64
		binaryData,         // 输出缓冲区
		&binarySize,        // 解码后数据的大小
		NULL,               // 不需要跳过的字符
		NULL)) {            // 不需要格式
		printf("Failed to decode Base64 string. Error: %lu\n", GetLastError());
		free(binaryData);
		return NULL;
	}

	if (size)
		*size = binarySize;
	return binaryData;
}

int quoted_printable_decode(const char* pSrc, char* pDst, int nSrcLen)
{
	int nDstLen; // 输出的字符计数
	int i;
	i = 0;
	nDstLen = 0;
	while (i < nSrcLen)
	{
		if (strncmp(pSrc, "=\n", 2) == 0) // 软回车，跳过
		{
			pSrc += 2;
			i += 2;
		}
		else
		{
			if (*pSrc == '=') // 是编码字节
			{
				sscanf(pSrc, "=%02X", (unsigned int*)pDst);
				pDst++;
				pSrc += 3;
				i += 3;
			}
			else // 非编码字节
			{
				*pDst++ = (unsigned char)*pSrc++;
				i++;
			}

			nDstLen++;
		}
	}
	// 输出加个结束符
	*pDst = 0;
	return nDstLen;
}

//malloc buf
//Content-Transfer-Encoding: quoted-printable
unsigned char* decode_quoted(char* str, unsigned long *size)
{
	unsigned char* ret;

	size_t len = strlen(str);
	ret = malloc(len);
	if (!ret)
		return NULL;

	len = quoted_printable_decode(str, ret, (int)len);
	*size = (unsigned long)len;
	return ret;
}

int file_size(char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fclose(file);
	return (int)size;
}

//read file content to buf
//call free(buf) after use
char* read_file_content(char* path, int* fsize)
{
	FILE* fp = fopen(path, "r");
	if (!fp)
		return NULL;
	int file_len;

	//get file size
	fseek(fp, 0, SEEK_END);
	file_len = (int)ftell(fp);
	fseek(fp, 0, SEEK_SET); //to file head

	char* read_buf = malloc(file_len + 1);
	if (!read_buf) {
		fclose(fp);
		return NULL;
	}

	file_len = (int)fread(read_buf, 1, file_len, fp);
	fclose(fp);
	read_buf[file_len] = 0;
	if (fsize)
		*fsize = file_len;

	return read_buf;
}

void convert_file_utf(char* filename)
{
	int file_len;
	char* buf = read_file_content(filename, &file_len);
	if (!buf)
		return;

	int out_len;
	char* out;
	out = to_utf(buf, &out_len);
	if (!out)
		return;

	FILE* fp = fopen(filename, "w");
	fwrite(out, 1, out_len, fp);
	fclose(fp);
	free(out);
}

// Check if the file exists
int FileExists(const char* fileName)
{
	int result = 0;

#if defined(WINDOWS)
	if (_access(fileName, 0) != -1) result = 1;
#else
	if (access(fileName, F_OK) != -1) result = 1;
#endif

	// NOTE: Alternatively, stat() can be used instead of access()
	//#include <sys/stat.h>
	//struct stat statbuf;
	//if (stat(filename, &statbuf) == 0) result = 1;

	return result;
}

// Check if a directory path exists
int DirectoryExists(const char* dirPath)
{
	int result = 0;
	DIR* dir = opendir(dirPath);

	if (dir != NULL)
	{
		result = 1;
		closedir(dir);
	}

	return result;
}

//return dir pointer
void* start_find(const char* dirPath)
{
	return opendir(dirPath);
}

//return file pointer
void* find_next(void* handle, char* name)
{
	struct dirent* file = NULL;
	DIR* dir = (DIR*)handle;

	if (!dir || dir->handle == -1)
		return NULL;

	while (1) {
		if (!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1)
		{
			file = &dir->result;
			file->d_name = dir->info.name;
			if (strstr(file->d_name, name))
				break;
		}
		else {
			//reach the end
			file = NULL;
			break;
		}
	}

	return (void*)file;
}

char* get_find_file_name(void* handle)
{
	struct dirent* file = (struct dirent*)handle;
	return file->d_name;
}

void stop_find(void* handle)
{
	DIR* dir = (DIR*)handle;
	closedir(dir);
}


//find first file name of match match_str
//return 0 if OK
int find_files(const char* dirPath, char* match_str, char* file_name)
{
	void* dir = start_find(dirPath);
	if (!dir)
		return 1;

	void* file;
	file = find_next(dir, match_str);
	if (!file) {
		stop_find(dir);
		return 1;
	}

	char* name = get_find_file_name(file);
	strcpy(file_name, name);
	stop_find(dir);
	return 0;
}
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


char chapter0_name[] = "序";
//char default_regex_str[] = \
//"[.]*[第卷][0123456789一二三四五六七八九十零〇百千两]*[章回部节集卷].*[.]*";
char *default_regex_str = NULL;

////////////book-toc.html///////////////////////////////
const char toc_html_head[] = u8"\
<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"zh-CN\">\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=utf-8\" />\n\
<meta name=\"generator\" content=\"EasyPub v1.50\" />\n\
<title>\n\
Table Of Contents\n\
</title>\n\
<link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\"/>\n\
</head>\n\
<body>\n\
<h2 class=\"titletoc\">\n\
目录\n\
</h2>\n\
<div class=\"toc\">\n\
<dl>\
";

const char toc_html_tail[] = "\
</dl>\n\
</div>\n\
</body>\n\
</html>\n\
";

////////////toc.ncx///////////////////////////////
const char toc_ncx_head[] = "\
<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\n\
<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" \"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">\n\
<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n\
<head>\n\
<meta name=\"cover\" content=\"cover\"/>\
";

const char toc_ncx_mid[] = "\
<meta name=\"dtb:generator\" content=\"EasyPub v1.50\"/>\n\
<meta name=\"dtb:totalPageCount\" content=\"0\"/>\n\
<meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n\
</head>\n\n\
<docTitle>\n\
";

const char toc_ncx_content[] = u8"\
<navMap>\n\
<navPoint id=\"cover\" playOrder=\"1\">\n\
<navLabel><text>封面</text></navLabel>\n\
<content src=\"cover.html\"/>\n\
</navPoint>\n\n\
<navPoint id=\"htmltoc\" playOrder=\"2\">\n\
<navLabel><text>目录</text></navLabel>\n\
<content src=\"book-toc.html\"/>\n\
</navPoint>\n\
";

////////////content.opf///////////////////////////////
const char opf_head[] = "\
<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>\n\n\
<package version=\"2.0\" xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"bookid\">\n\
<metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\">\
";

//<dc:title>title</dc:title>
//<dc:creator opf:role="aut">a</dc:creator>

const char opf_mid[] = "\
<dc:rights>Created with EasyPub v1.50</dc:rights>\n\
<dc:language>zh-CN</dc:language>\n\
<meta name=\"cover\" content=\"cover-image\"/>\n\
</metadata>\n\
<manifest>\n\
<item id=\"ncxtoc\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\"/>\n\
<item id=\"htmltoc\"  href=\"book-toc.html\" media-type = \"application/xhtml+xml\"/>\n\
<item id=\"css\" href=\"style.css\" media-type=\"text/css\"/>\n\
<item id=\"cover-image\" href=\"default_cover.png\" media-type=\"image/png\"/>\n\
<item id=\"cover\" href=\"cover.html\" media-type=\"application/xhtml+xml\"/>\
";

const char opf_tail[] = "\
</spine>\n\
<guide>\n\
<reference href=\"cover.html\" type=\"cover\" title=\"Cover\"/>\n\
<reference href=\"book-toc.html\" type=\"toc\" title=\"Table Of Contents\"/>\n\
<reference href=\"chapter0.html\" type=\"text\" title=\"Beginning\"/>\n\
</guide>\n\
</package>\
";

/////////////html////////////////
const char novel_content_head[] = "\
<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"zh-CN\">\n\
";

/*
chapter 0 - 0
chapter 1 - 0

<title>chapter 1 - 0 </title>
*/

const char novel_content_mid[] = "\
<link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\"/>\n\
</head>\n\
<body>\
";

//<h2 id = "title" class = "titlel2std">xxx</h2>
//<p class="a">　xxx </p>
//<p class="a">　xxx </p>
//</body>
//</html>

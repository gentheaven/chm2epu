# chm 转为 epub



2025/3/19

## 缘起

以前有一些很好的电子书，是 chm 格式的，比如：COM组件设计与应用.chm

如果想阅读这个文件，很不方便。

chm 阅读器不好用，字体太小，渲染不好。

如果能转换为当前常用的 epub，更方便阅读。

现在找不到可用的转换器。尝试过 calibre ，转换失败。

所以写这个工具。



## 功能

chm 文件转换为 epub 格式

Windows10 测试通过

有图形界面

生成的epub文件，可以用 PC 上的 Calibre e-book viewer 查看，

也可以用 Calibre 转换为 PDF 后，放在任何地方阅读。 



## 开发环境

纯 c 语言实现

VS2019 社区版



## 开源库

界面开发：webui 开源库。webui-2.dll

网页解析：lexbor 开源库。lexbor.dll



## 思路

windows 自带工具 hh.exe

1. **解压 chm** ，chm 文件转换为 mht 文件

 		hh -decompile .\out\ .\COM.chm

2. 解码 mht 文件，转换为 html 文件
3. 生成 epub



## 目录结构

**chm2epub**  目录：VS2019 项目工程



**release**  目录：chm2epub.exe, 最终使用的转换程序

​	chm2epub.html ：转换程序的界面

​	webui-2.dll， lexbor.dll 转换程序依赖的运行库

​	epub-res 目录：生成 epub 文件时需要的一些资源

​	7z.exe 7z.dll：生成 epub 时需要 7zip 压缩文件夹



**test**  目录：

​	COM组件设计与应用.chm： 测试用例

​	Windows_core.chm： 测试用例

​	Windows_core.hhc：目录文件，这个目录包含  param name="Local" value="**1.htm#02**"

​	1.hhk：COM组件设计与应用.chm 解压后生成的目录文件，改名了

​	1.mht：COM组件设计与应用.chm 解压后生成的 mht 文件，改名了

​	1_decode.html： 1.mht 文件中的html 部分，解码后生成的



**res** 目录：程序编译时需要的lib 文件和头文件



**src** 目录：c 代码

htmllib.c： 生成 epub 的相关工具

str_resource.c： 生成 epub 需要的字符串资源

libs.c：OS 相关， 双向链表

mylib_parse.c：对 html 解析器 Lexbor 的二次封装



main.c：主程序

核心入口：

```c
//list_only = 1, only get chapter list
int epub_convert(char* buf, int size, char* stitle, char* author, int list_only);
```

输入参数： 

buf： chm 文件的内容

size：chm 文件的大小

stitle：要生成的 epub 文件的名称

author： 要生成的 epub 文件的作者

list_only：

​	1: 只得到 chm 的章节标题，而不生成 epub 文件

​	0：生成 epub 文件



release\chm2epub.html：这个文件描述了 GUI

本程序用到 webui 开源库，c 程序和 html 文件可以相互通讯。

 html 文件：负责描述图形界面和处理用户交互（Javascript）

C 程序：后端干活的

------



## 开发日志

2025/3/23， 大约花费了 5天时间，完成了第一版。

基本可用。

可以把 chm 转换为 epub，用 PC 上的 Calibre e-book viewer 查看结果。

Calibre  版本：7.21

用 Sigil 软件（V2.4.2）检查 epub 的兼容性。






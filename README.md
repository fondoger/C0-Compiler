# 基于C0文法的编译器

北航编译原理课程，扩充C0文法编译器，使用 C++ 编写。

## 0. 前言

实现这个编译器的过程中，主要是参照编译技术课本，并借鉴了大量PL/0编译器源代码及Pascal-S编译器源代码的内容。这是没有实现目标代码优化的版本。

本项目一些特点：

1. 缩进层次少，代码精简，代码行数(不包括注释)仅2100行
2. 生成的汇编代码(优化前)相对精简
3. 广泛使用 STL 容器，代码可读性、扩展性强
4. 尽可能使用引用代替指针，不会出现内存泄漏

## 1. 编译及运行

### 1.1 命令行编译运行(Makefile)

**前置条件**

* `g++`命令可用
* `make`命令可用

**克隆**

``bash
git clone https://github.com/fondoger/C0-Compiler
cd C0-Compiler
```

**编译**

```bash
make                        # 生成可执行文件, test 就是我们的编译器
```

**运行**

```bash
make run                    # 方法1: 使用默认源文件(hello_word.txt)
./test hello_world.txt      # 方法2: 使用指定源文件
```



### 1.2 Codeblocks项目导入及运行

0. 新建空白c++控制台项目

1. 导入所有`.h`和`.cpp`文件
    > In CodeBlocks, click `Project -> Add files...` to open a file browser, select all `.cpp` and `.h` files. (Alternatively, the option `Project->Add files recursively...` will search through all the subdirectories in the given folder, selecting the relevant files for inclusion.) 

2. 在CodebBlocks开启gcc `-std=c++11`编译选项
    > 1. Go to  `Toolbar -> Settings -> Compiler`
    > 2. In the `Selected compiler` drop-down menu, make sure `GNU GCC Compiler` is selected
    > 3. Below that, select the `compiler settings` tab and then the `compiler flags` tab underneath
    > 4. In the list below, make sure the box for "`Have g++ follow the C++11 ISO C++  language standard [-std=c++11]`" is checked
    > 5. Click `OK` to save

3. 在CodeBlocks中运行项目

## 2. 代码样例

> **提示**：<br>
> 1.在 `examples` 文件夹里有汉诺塔、斐波那契数列和快速排序的C0文法代码<br>
> 2.在 `tests` 文件夹里有本项目用到的所有测试代码

给出一段使用C0文法的汉诺塔求解代码：

```c
// 非标准C语言文法
void hanio(int n, char from, char buffer, char to)
{
    if (n == 0) return;
    else;
    hanio(n - 1, from, to, buffer);
    printf("Move disk from ", from);
    printf(" to ", to);
    printf("\n");
    hanio(n - 1, buffer, from, to);
}
void main()
{
    int n;
    printf("Please input: ");
    scanf(n);
    hanio(n, 'A', 'B', 'C');
}
```

将以上代码命名为`hanio.c`并编译代码：
```
$ ./test hanio.c
compile success!
mid code at: mid_code.txt
mips code at: mips_code.txt
```

将编译后的mips代码`mips_code.txt`使用mars模拟器执行：

```
$ java -jar mars.jar nc mips_code.txt
Please input: 3
Move disk from A to C
Move disk from A to B
Move disk from C to B
Move disk from A to C
Move disk from B to A
Move disk from B to C
Move disk from A to C
```

## 3. 项目介绍

**代码行数统计**

```bash
$ cloc ./<project_root_path>/
--------------------------------------------------------------------------
Language                files          blank        comment           code
--------------------------------------------------------------------------
C++                         7            204            335           1925
C/C++ Header                7             71            218            307
Markdown                    1             44              0             94
make                        1             19              9             21
--------------------------------------------------------------------------
SUM:                       16            338            562           2347
--------------------------------------------------------------------------
```

**项目结构**


* common.h: 全局变量
* error.h/error.cpp: 错误处理
* symbol.h/symbol.cpp: 词法分析
* table.h/table.cpp: 符号表管理
* grammar.h/translator.cpp: 语法分析、语义分析、中间代码生成
* mips.h/mips.cpp: 目标代码生成

**关于错误处理**

关于错误处理，绝大多数同学的错误处理都是用 if...else 来判断的，这样会导致代码嵌套层级特别深，很容易出现 bug。

为了实现准确的错误判定和跳读错误继续编译的功能，同时保证代码的可读性，我参照了Pascal-S编译器的源代码，在`error.h`中定义了三个宏函数`test1`, `test2`, `test3`，同时在`symbol.h`中定义了一个C++类`Symset`，这样的设计将代码从繁琐的 `if...else` 判断解放出来，只需要使用一行代码就能够完成错误处理和跳读，从而便于我们专注于语法分析逻辑，有效减少Bug的产生。此外，语法分析模块的代码行数和缩进嵌套层级也因此显著减少。

**关于`switch`语句**

对于大部分语法成分，采用递归下降分析时，每当阅读到一个语法成分就能够生成相应的顺序正确的的中间代码。但是对于`switch...case`语句，必须阅读完所有的`case`条件，才能开始生成顺序正确中间代码，并且`switch...case`语句也可能多次嵌套使用。

为了保持代码结构的统一性和简洁性，我设计了一个栈式结构来输出中间代码，在遇到`switch`语句时，调用`pushMidCodeCacheStack()`函数可以在栈顶生成一个缓存代码的`vector`容器，调用`flushCachedMidCode()`函数即可将缓存的中间代码输出，此外`startCachingMidCode()`和`pauseCachingMidCode()`两个函数可以控制缓存的开始或暂停缓存中间代码。如果你的编译器没有switch语句，可以注释掉代码中的这几个函数，不会对其他功能产生任何影响。

**给学弟学妹们的几个建议**

1.在编写编译器的初始阶段，最好不要考虑任何错误处理功能，否则复杂度会指数上升。可以先假设所有源代码是符合文法标准的，不对考虑任何因源代码错误而出现的编译错误负责。当你的程序能够正确编译符合文法的标准源代码后，再考虑加入错误处理和跳读逻辑。这样做会轻松很多。

2.实现目标代码优化时，采用内联函数的办法可以显著较少函数调用开销，从而在打分环节获得更高的分数。显然，生成的目标代码行数也会显著增加，不过目标代码行数多少不会纳入考核。



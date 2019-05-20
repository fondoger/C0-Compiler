#include <iostream>     // cout
#include <sstream>      // stringstream
#include <cstdlib>      // exit()
#include <cstdarg>      // for variadic arguments
#include <set>          // set
#include <map>
#include <vector>
#include "error.h"
#include "symbol.h"
#include "common.h"
/**
 * 词法错误
 * 1. unrecognized character <Character>(ASCII:) 
 * 1. missing ending single quotation mark \' for char
 * 2. missing ending double quotation mark \" for string
 * 语法错误
 * 不能正确构造语法树
 * 语义错误
 * 1. missing return value for functions
 * 2. return-statement with no value, in function returning 'int/char'
 * 3. [waring] no return statement in function returning non-void
 * 
 */

static std::vector<std::string> cached_errors;

static void printError(std::string msg)
{
    std::stringstream buffer;
    buffer << source_filename << ":" << g_line_no << ":"
        << g_word_pos << ":" << msg << std::endl;
    buffer << g_line;
    for (unsigned int i = 0; i < g_word_pos; i++)
        buffer << " ";
    buffer << "^";
    // NOTE:
    // use explicit type conversion here to avoid
    // negative numbers!
    // it took me 2 hour to find this! :(
    for (int i = 0; i < (int)g_pos - (int)g_word_pos - 2; i++)
        buffer << "~";
    buffer << std::endl;
    cached_errors.push_back(buffer.str());
}

bool printCachedErrors()
{
    if (cached_errors.size() != 0) {
        std::cout << "compile terminated with error(s):" << std::endl;
        int count = 0;
        for (std::string msg : cached_errors) {
            if (count == 6) {
                std::cout << "Ommited " << (cached_errors.size() - 0) 
                    << "+ more errors" << std::endl;
                break;
            }
            std::cout << msg;
            count++;
        }
        return true;
    }
    return false;
}

void skip(Symset fsys)
{
    //bool skipped = false;
    while (!fsys.contains(g_sym)) {
        //skipped = true;
        // TODO: underline for skipped words
        readSymbol();
    }
    //if (skipped) std::cout << std::endl;
}

void test(Symset s1, Symset s2)
{
    if (!s1.contains(g_sym)) {
        std::vector<int> syms = s1.toVector();
        std::stringstream buffer;
        buffer << "error: expected ";
        for (unsigned int i = 0; i < syms.size(); i++) {
            if (i != 0) buffer << "|";
            buffer << sym2str[syms[i]];
        }
        buffer << " before " << sym2str[g_sym];
        printError(buffer.str());
        skip(s1 + s2);
    }
}

void error(std::string err_msg)
{
    printError("error: " + err_msg);
}




/**
 * expect g_sym to to equal to sym
 */
void expect(Symbol sym)
{
    if (g_sym != sym) {
        std::cout << "Error: expect " << sym2str[sym]
            << " but got " << sym2str[g_sym];
        if (g_sym == IDENTSY)
            std::cout << "<" << g_id << ">";
        std::cout << std::endl;
        std::exit(1);
    }
}

/**
 * expect g_sym to be one of multiple symbols
 * * num is number of available symbols
 * for example:
 *      expectMult(3, INTSY, CHARSY, VOIDSY)
 *
 * NOTE: please make sure arguments is type of Symbol enum
 */
void expectMul(int num, ...) 
{
    va_list valist;
    va_start(valist, num);
    for (int i = 0; i < num; i++) {
        Symbol sym = (Symbol)va_arg(valist, int);
        if (g_sym == sym) {
            va_end(valist);
            return;
        }
    }
    va_end(valist);
    std::cout << "Error: expect ";
    va_start(valist, num);
    for (int i = 0; i < num; i++) {
        if (i != 0)
            std::cout << "|";
        std::cout << sym2str[va_arg(valist, int)];
    }
    va_end(valist);
    std::cout << ", but got " << sym2str[g_sym] << std::endl;
    std::exit(1);
}









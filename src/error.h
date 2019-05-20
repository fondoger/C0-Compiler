/**
 * This is error handler
 * Used for printing error messages
 */
#ifndef ERORR_H_
#define ERROR_H_

#include <stack>
#include <set>
#include "symbol.h"
#include "table.h"

/**
 * ERROR_CODE 0 ~ 64 is reserved for
 * missing symbols in Symbol enum
 */

/**
 * 语法错误和词法错误都直接输出错误信息, 无需错误码
 */
#define ERR_PROGRAM_INCOMPLETE      "program incomplete"
#define ERR_WRONG_TYPE_OF_MAIN      "wrong type of main function"
#define ERR_REDUNDENT_CODE          "redundent code after main func"
#define ERR_UNSUPPORTED_CHARACTER   "unsupported character"
#define ERR_STRING_NOT_END          "missing \'\"\' for string"
#define ERR_STRING_INVALID_CHARACTER    "invalid character in string"
#define ERR_DUPLICATE_GLOBAL_IDENTIFIER "duplicate global identifier(global const name, variable name or function name collision)"
#define ERR_DUPLICATE_LOCAL_IDENTIFIER  "duplicate local identifier(local const name, variable name or function parameter name collision)"
#define ERR_UNDEFINED_IDENTIFIER    "reference of undefined identifier"
#define ERR_EXPECT_NON_VOID_FUNCTION    "expect function call with return value"
#define ERR_NOT_A_FUNCTION          "use function call on a not function identifier"
#define ERR_EXPECT_VARIABLE_IDENTIFIER  "expect varaible identifier as left value of an assignment statement"
#define ERR_TYPE_NOT_MATCH          "type not match"
#define ERR_WRONG_LEFT_VALUE        "left value should be variable name"
#define ERR_WRONG_STATEMENT         "wrong statement"
#define ERR_EXPECT_ARGUMENTS        "should provide arguments for function call"
#define ERR_LESS_ARGUMENTS          "arguments too less"
#define ERR_MORE_ARGUMENTS          "arguments too more"
#define ERR_WRONG_TYPE_OF_ARGUMENT  "wrong type of argument"
#define ERR_COMPARE_TYPE_NOT_MATCH  "comparison type not match"
#define ERR_EXPECT_INT_TYPE_SINGLE_CONDITION "expect int type in single expression condition"
#define ERR_SWITCH_TYPE_NOT_MATCH   "switched-value and cased-value's type not match"
#define ERR_DUPLICATE_SWITCH_VALUE  "duplicate switched value"
#define ERR_WRONG_RETURN_TYPE       "wrong type of return value"
#define ERR_WRONG_VARIABLE_TYPE     "variable's type can't be void"
#define ERR_WRONG_TYPE_OF_SCANF     "scanf's arguments must be int or char variable"
#define ERR_SCANF_NO_ARGUMENTS      "expect at least one arguments for scanf()"
#define ERR_MISSING_PARAMETERS      "expect at least one argument for function definition with parathesis"
#define ERR_MISSING_ARGUMENTS       "expect at least one argument for function call with parathesis"
#define ERR_ARRAY_SIZE_ZERO         "array size must > 0"
#define ERR_EXPECT_INT_ARRAY_INDEX  "array's index type should be int"
#define ERR_LEFT_VALUE_NOT_VARIABLE "left value of assignment must be variable or array element"
#define ERR_EXPECT_ARRAY_ELEMENT    "expect a array element not an array"
#define ERR_ARRAY_INDEX_OVERFLOW    "array index overflow"
#define ERR_NOT_AN_ARRAY            "array-like operate on a not-array identifier"


/**
 * Don't use `errno` as parameter name
 * because it is a reserve word
 */

void error(std::string msg);
void expect(Symbol sym);
void expectMul(int num, ...);
bool printCachedErrors();

void skip(Symset fsys, int err);

void test(Symset s1, Symset s2);
void testDT(DTset s1, DTset s2);


/**
 * Using macro function here because
 * we need to `return` in it's caller function
 * in some casees
 *
 * `test1` differs from `test` in that
 * the latter will terminate function
 * if the symbol is not in s1
 */
#define test1(s1, s2) \
    do { \
        test(s1, s2); \
        if (!s1.contains(g_sym)) \
            return; \
    } while (0)

/**
 * `test2` differs from `test1` in that
 * the latter will read one more symbol 
 * if the symbol is not in s1
 */
#define test2(s1, s2) \
    do { \
        test(s1, s2); \
        if (!s1.contains(g_sym)) { \
            readSymbol(); \
            return; \
        } \
    } while (0)

/**
 * This is a shortcut that will skip words until 
 * RBRACE and SEMICOLON if expected symbols are 
 * not found.
 *
 * `test3(Symset({SYM1, SYM2}))`
 * is equvialent to
 * `test2(Symset({SYM1, SYM2}), Symset({RBRACE, SEMICOLON}))`
 */
#define test3(s1) \
    do { \
        test(s1, Symset({RBRACE, SEMICOLON})); \
        if (!s1.contains(g_sym)) { \
            readSymbol(); \
            return; \
        } \
    } while (0);

#endif // ERROR_H_


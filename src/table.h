/**
 * This is symbol table management module
 */

#ifndef TABLE_H_
#define TABLE_H_

#include <unordered_map>    // unordered_map
#include "symbol.h"

enum IdentScope {
    GLOBAL,
    LOCAL,
};

/* IT is short of identifier type */
enum IdentType {
    IT_CONST,
    IT_VARIABLE,
    IT_FUNCTION, 
    IT_ARRAY,
};

/* DT is short of data type */
enum DataType {
    DT_VOID = 0,     /* used for functions without return value */
    DT_INT,
    DT_CHAR,
};

typedef Symset ITset; // identifier type set
typedef Symset DTset; // data type type set

static std::string itype2str[] = { 
    "IT_CONST", "IT_VARIABLE", "IT_FUNCTION"
};
static std::string dtype2str[] = { 
    "DT_VOID", "DE_INT", "DT_CHAR", "DT_ARRAY" 
};
static std::string dtype2midtype[] = {
    "void", "int", "char", "array",
};

/**
 * scope: 
 *   identifier's scope
 *
 * itype: 1. const identifier(IT_CONST)
 *        2. variable identifier(IT_VARIABLE)
 *        3. function identifier(IT_FUNCTION)
 *        4. array identifier(IT_ARRAY)
 * dtype: 
 *   for consts: DT_CHAR for char const
 *               DT_INT for int const
 *   for variables: DT_CHAR for char variable
 *                  DT_INT for int variable
 *   for functions: DT_CHAR for char functions(return char value)
 *                  DT_INT for int functions(return int value)
 *                  DT_VOID for void functions(no return value)
 * value:
 *   for consts: 
 *      * value of int or ASCII value of char
 *   for arrays:
 *      * array size
 * addr:
 *   for arrays and variables:
 *      * allocated relative address in function
 */
typedef struct _TabEntry {
    IdentScope  scope;
    IdentType   itype;
    DataType    dtype;
    int         value;
    int         addr;
} TabEntry;

void tabInsert(const std::string &id, const TabEntry &entry);
bool tabFind(const std::string &id, TabEntry &entry);
void tabClear(IdentScope scope);

void tabInsertParam(const std::string &id, DataType &dtype);
const std::vector<DataType> & tabGetParams(const std::string &id);

void printSymbolTable(IdentScope scope);

std::string string2label(const std::string &str);

#endif // TABLE_H_

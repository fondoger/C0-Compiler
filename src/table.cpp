#include <unordered_map>    // unordered_map
#include <cassert>          // assert
#include <iomanip>          // setw
#include <map>				// map
#include <sstream>          // stringstream
#include "symbol.h"
#include "table.h"
#include "error.h"



/* For global variables */
static std::unordered_map<std::string, TabEntry> symbolTableG;
/* For local variables */
static std::unordered_map<std::string, TabEntry> symbolTableL;

static std::unordered_map<std::string, std::vector<DataType>> funcParams;

/**
 * Insert a parameter's type definition in to a function's
 * parameter list, which is used to type checking for 
 * function calls.
 */
void tabInsertParam(const std::string &id, DataType &dtype)
{
    std::unordered_map<std::string, std::vector
        <DataType>>::iterator it;
    if ((it = funcParams.find(id)) == funcParams.end()) {
        std::vector<DataType> params;
        params.push_back(dtype);
        funcParams[id] = params;
    } 
    else {
        std::vector<DataType> &params = (*it).second;
        params.push_back(dtype);
    }
}


const std::vector<DataType> & tabGetParams(const std::string &id) 
{
    const static std::vector<DataType> EMPTY_PARAMS;
    std::unordered_map<std::string, std::vector
        <DataType>>::iterator it;
    if ((it = funcParams.find(id)) != funcParams.end()) {
        return (*it).second;
    }
    else return EMPTY_PARAMS;
}


/**
 * TODO: might use pointer to improve perfomance 
 */
bool tabFind(const std::string &id, TabEntry &entry)
{
    std::unordered_map<std::string, TabEntry>::iterator it;
    // find in local
    if ((it = symbolTableL.find(id)) != symbolTableL.end()) {
        entry = (*it).second;
        return true;
    }
    // find in global
    if ((it = symbolTableG.find(id)) != symbolTableG.end()) {
        entry = (*it).second;
        return true;
    }
    return false;
}

static void printTabEntry(const std::string &id, 
        const TabEntry &entry)
{
    std::cout << id 
        << ", " << itype2str[entry.itype]
        << ", " << dtype2str[entry.dtype] 
        << ", ";
    if (entry.itype == IT_CONST) {
        if (entry.dtype == DT_INT)
            std::cout << entry.value;
        else // DT_CHAR
            std::cout << (char)entry.value;
    }
    else
        std::cout << entry.value;
    std::cout << ", " << entry.addr;
    std::cout << std::endl;
}

void printSymbolTable(IdentScope scope)
{
    std::cout << (scope == GLOBAL ? 
        "##############Global table##############\n":
        "##############Local table###############\n");
    const std::unordered_map<std::string, TabEntry> &symbolTable =
        (scope == GLOBAL) ? symbolTableG : symbolTableL;
    int count = 0;
    for (auto const& item : symbolTable) {
        count++;
        std::cout << count << ":";
        printTabEntry(item.first, item.second);
    }
    std::cout << "################Table End###########\n";
    count = 0;
}

void tabInsert(
    const std::string &id, 
    const TabEntry &entry)
{
    // TODO: 
    // local variable name can't be same with the name
    // of the function where the local variable is defined in.
    // (What a stupid rule!)
    std::unordered_map<std::string, TabEntry> &symbolTable =
        (entry.scope == GLOBAL) ? symbolTableG : symbolTableL;
    if (symbolTable.find(id) != symbolTable.end()) {
        error(entry.scope == GLOBAL ? ERR_DUPLICATE_GLOBAL_IDENTIFIER :
                ERR_DUPLICATE_LOCAL_IDENTIFIER);
        return;
    }
    symbolTable[id] = entry;
}


void tabClear(IdentScope scope)
{
    assert(scope == LOCAL);
    symbolTableL.clear();
}


std::map<std::string, std::string> strings_table;
static int strings_count = 0;
/**
 * Insert string into strings table and generate
 * a label for the string.
 */
std::string string2label(const std::string &str)
{
    std::map<std::string, std::string>::iterator it;
    if ((it = strings_table.find(str)) != strings_table.end()) {
        return (*it).second;
    }
    std::stringstream ss;
    ss << "$STRING_" << strings_count;
    std::string label = ss.str();
    strings_table[str] = label;
    strings_count++;
    return label;
}

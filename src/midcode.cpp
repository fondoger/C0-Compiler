#include <iostream>     // cout
#include <cassert>      // assert
#include <stack>        // stack
#include <vector>       // vector
#include <sstream>      // stringstream
#include <cctype>       // isalpha, isdigit
#include <map>          // map
#include <string>       // stoi
#include "common.h"
#include "midcode.h"


void genMidCode(const OpCode &op, const std::string &a,
        const std::string &b, const std::string &res);
void pushMidCodeCacheStack();
void startCachingMidCode();
void pauseCachingMidCode();
void flushCachedMidCode();
std::string genTempVar();
std::string genLabel();
std::string genLabelIf();
std::string genLabelElse();
std::string genLabelIfEnd();
static std::string convertFormat(const FourTuple &ft);
bool isConstValue(const std::string &t, int &val);
void functionBegin();
void functionEnd();

std::vector<FourTuple> mid_codes;

// `cache_depth` will always be 0 unless 
// we met a switch-case statement
static int cache_depth = 0;
static std::vector<std::vector<FourTuple>> cachedMidCode;

void genMidCode(
    const OpCode &op,
    const std::string &a,
    const std::string &b,
    const std::string &res)
{
    FourTuple t = { op, a, b, res };
    // cache_depth usually is 0
    if (cache_depth != 0) {
        // NOTE: do use reference here
        std::vector<FourTuple> &cache = 
            cachedMidCode[cache_depth - 1];
        cache.push_back(t);
        return;
    }
    midcode_stream << convertFormat(t) << std::endl;
    mid_codes.push_back(t);
}

void pushMidCodeCacheStack()
{
    std::vector<FourTuple> cache;
    cachedMidCode.push_back(cache);
}
void startCachingMidCode()
{
    cache_depth++;
}
void pauseCachingMidCode()
{
    cache_depth--;
    assert(cache_depth >= 0);
}
void flushCachedMidCode()
{
    int t = cachedMidCode.size();
    assert(t > 0); // t must > 0
    std::vector<FourTuple> &cached = cachedMidCode[t - 1];
    for (const auto &item : cached) {
        genMidCode(item.op, item.a, item.b, item.res);
    }
    cachedMidCode.pop_back();
}

/**
 * Note: as user-defined variable names contain only
 * digits and letters, so temporary variable names
 * won't get conflict with user-defined variable names.
 */
static int temp_count = 0;
std::string genTempVar()
{
    std::stringstream res;
    res << "$t_" << temp_count++;
    return res.str();
}

static int labels_count = 0;
std::string genLabel()
{
    std::stringstream res;
    res << "$LABEL_" << labels_count++;
    return res.str();
}

static int if_statements_count = 0;
std::string genLabelIf()
{
    static std::string t = "$IF_";
    if_statements_count++;
    return t + std::to_string(if_statements_count);
}
std::string genLabelElse()
{
    static std::string t = "$ELSE_";
    return t + std::to_string(if_statements_count);
}
std::string genLabelIfEnd()
{
    static std::string t1 = "$IF_";
    static std::string t2 = "_END";
    return t1 + std::to_string(if_statements_count) + t2;
}


static std::string convertFormat(const FourTuple &ft)
{
    std::stringstream ss;
    switch(ft.op) {
        case ASSIGN:    
            ss << ft.res << " = " << ft.a; 
            break;
        case ADD:       
            ss << ft.res << " = " << ft.a << " + " << ft.b; 
            break;
        case SUB:
            ss << ft.res << " = " << ft.a << " - " << ft.b;
            break;
        case MUL:
            ss << ft.res << " = " << ft.a << " * " << ft.b;
            break;
        case DIV:
            ss << ft.res << " = " << ft.a << " / " << ft.b;
            break;
        case WARRAY:    
            ss << ft.a << "[" << ft.b << "]" << " = " << ft.res;
            break;
        case RARRAY:
            ss << ft.res << " = " << ft.a << "[" << ft.b << "]";
            break;
        case COMPARE:
            ss << ft.a << " " << ft.b << " " << ft.res;
            break;
        case FUNC:  ss << ft.a << " " << ft.b << "()"; break;
        case PARA:  ss << "para " << ft.a << " " << ft.b; break;
        case GVAR:
        case VAR:   ss << "var " << ft.a << " " << ft.b
                       << " " << ft.res; break;
        case PUSH:  ss << "push " << ft.a << " " << ft.b; break;
        case CALL:  ss << "call " << ft.a; break;
        case RET:   ss << "ret " << ft.a; break;
        case GETRET:ss << "getret " << ft.res; break;
        case WRITE: ss << "printf " << ft.a << " " << ft.b; break;
        case READ:  ss << "scanf " << ft.a << " " << ft.b; break;
        case END:   ss << "end"; break;
        case LABEL: ss << "label " << ft.a; break;
        case GOTO:  ss << "goto " << ft.a; break;
        case BZ:    ss << "bz " << ft.a << " " << ft.b 
                       << " " << ft.res; 
                    break;
        case BNZ:   ss << "bnz " << ft.a << " " << ft.b 
                       << " " << ft.res; 
                    break;
        case TEMP:  ss << "temp " << ft.a << " " << ft.b; break;
        default:    std::cout << "FUCK: unknown!" << std::endl;
    }
    return ss.str();
}

// both const int and const char are const values
bool isConstValue(const std::string &t, int &val)
{
    if (t[0] == '\'') {
        assert(t[2] == '\'');
        val = t[1];
        return true;
    } else if (std::isdigit(t[0]) || t[0] == '-') {
        val = std::stoi(t);
        return true;
    }
    return false;
}



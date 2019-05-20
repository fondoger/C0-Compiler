#ifndef MIDCODE_H_
#define MIDCODE_H_

#include <string>       //std::string
#include "table.h"

/**
 * This module does:
 *  * generate quadruple(4-tuple) middle code
 */

/**
 * Note: 
 * 1. all consts defined no matter globally or locally,
 * will be replaced by their literal values. So we don't
 * need operations for defining consts in middle-code.
 * 2. global variables should be allocate statically
 * in mips, but local variables should be allocated
 * dynamically in mips code. So we use different
 * operations.
 */
enum OpCode {
    ASSIGN,
    ADD, SUB, MUL, DIV,
    FUNC, PARA, 
    GVAR, VAR,
    PUSH, CALL, 
    RET, GETRET,
    WARRAY, RARRAY,         // write array, read array
    WRITE, READ,
    COMPARE,
    END,                    // function complete
    /*
    EQUAL, NOT_EQUAL, LESS,
    LESS_EQUAL, GRATER, GRATER_EQUAL,
    */
    LABEL, GOTO, 
    BZ,                     // branch if previous compare is zero
    BNZ,                    // branch if previous compare is not zero
    TEMP,                   // temp variable 
};

static std::string op2str[] = {
    "ASSIGN",
    "ADD", "SUB", "MUL", "DIV",
    "FUNC", "PARA", 
    "GVAR", "VAR",
    "PUSH", "CALL", 
    "RET", "GETRET",
    "WARRAY", "RARRAY",
    "WRITE", "READ",
    "COMPARE",
    "END",                  // function complete
    "LABEL", "GOTO",
    "BEQ", "BNE",
    "BZ", "BNZ",
};


typedef struct _FourTuple {
    OpCode op;
    std::string a;
    std::string b;
    std::string res;
} FourTuple;

const std::string NONE = "";
void genMidCode(
    const OpCode &op, 
    const std::string &a, 
    const std::string &b, 
    const std::string &res
);


/**
 * Following 3 functions is used to generate middle
 * code for switch-case statement
 *
 * Why we need this?
 * Considering following code:
 *
 *      switch(val) {
 *          case 1: statement1;
 *          case 2: statement2;
 *          default: default_statement;
 *      }
 *      other_statement;
 * 
 * What we expected is following mid-code:
 *
 *      compare 1 and val, if equal goto label_1
 *      compare 2 and val, if equal goto label_2
 *   label_default:
 *      default_statement;
 *      goto_label_end
 *   label_1:                   // cached
 *      statement1;             // cached      
 *      goto label_end          // cached    
 *   label_2:                   // cached       
 *      statement2;             // cached 
 *      goto label_end          // cached
 *   label_end:
 *      other_statement;
 *
 * However, our grammar analyzer can only move forward, 
 * it can't go back. Thus we can't generate above code
 * directly. But we can easily generate below mid-code:
 *
 *      compare 1 and val, if equal goto label_1
 *   label_1:                                       // cache this
 *      statement1;                                 // cache this
 *      goto label_end                              // cache this
 *      compare 2 and val, if equal goto label_2
 *   label_2:                                       // cache this
 *      statement2;                                 // cache this
 *      goto label_end                              // cache this
 *   label_default:             
 *      statement
 *      goto_label_end
 *   // put cached statement here
 *   label_end:
 *      other_statement;
 *
 * We cache mid-codes marked with `cached this`, and 
 * put those codes before `label_end`, then mid-code 
 * became what we expected.
 *
 * For nested switch-case statement, we can use
 * a stack-like data structure to cached mid-code.
 */
void pushMidCodeCacheStack();
void startCachingMidCode();
void pauseCachingMidCode();
void flushCachedMidCode();

/**
 * Generate temporary variable name.
 * Format: $t1, $t2, $t3
 */
std::string genTempVar();

/**
 * Generate labels for goto-like operations
 * Format: $label_1, $label_2, $label_3
 */
std::string genLabel();

std::string genLabelIf();
std::string genLabelElse();
std::string genLabelIfEnd();

bool isConstValue(const std::string &t, int &val);


extern std::vector<FourTuple> mid_codes;

#endif // MIDCODE_H_

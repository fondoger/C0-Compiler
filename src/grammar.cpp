#include <iostream>     // cout
#include <cstdio>       // printf
#include <cassert>      // assert
#include <sstream>      // stingstream
#include "symbol.h"    
#include "grammar.h"
#include "table.h"
#include "error.h"
#include "common.h"
#include "midcode.h"


/**
 * This module does:
 *  1.syntax checking
 *  2.semantics checking
 *  3.mid-code generating
 */

#define NU "     "

static void pConstDefinitions(IdentScope scope);    // <常量说明>
static void pConstDefinition(IdentScope scope);     // <常量定义>
static void pLocalVariableDefinitions();      
static void pGlobalVariableDefinitionItem(
        DataType dtype, std::string identifier);
static void pFunctionDefinition(const DataType &dtype, const std::string &id);
static void pMainFunctionDefinition();
static void pParametersList(const std::string &id); // 形参<参数表>
static void pArgumentsList(const std::vector<DataType> &params);    // 实参<值参数表>
static void pStatementsList();                      // 语句列
static void pStatement();
static void pIfElseStatement(); 
static void pSwitchCaseStatement();
static void pCaseItem(const std::string &switched_val, const DataType &dtype, 
    const std::string &end_label);
static void pDoWhileStatement();
static void pPrintfStatement();
static void pScanfStatement();
static void pReturnStatement();
static void pEmptyStatement();
static void pAssignmentStatement(const std::string &id);
static void pArrayRead(std::string &res, DataType &res_dtype,
        const std::string &id, const TabEntry &entry);
static void pArrayAssignmentStatement(const std::string &id);
static void pNonVoidFunctionCall(std::string &res, DataType &res_dtype, 
        const std::string &id, const TabEntry &entry);// <有返回值函数调用语句>
static void pFunctionCallStatement(const std::string &id);
static void pCondition();
static void pExpression(std::string &res, DataType &res_dtype);
static void pTerm(std::string &res, DataType &res_dtype);
static void pFactor(std::string &res, DataType &res_dtype);
static void pSignedInteger();


// file-scope global varaibles
static TabEntry current_function_tabEntry;     // for check return statement's type

void pProgram()
{
    readSymbol();
    pConstDefinitions(GLOBAL);
    while (true) {
        test(Symset({INTSY, CHARSY, VOIDSY}), Symset({}));
        DataType dtype = (g_sym == INTSY ? DT_INT:
                g_sym == CHARSY ? DT_CHAR : DT_VOID);
        readSymbol();
        if (g_sym == MAINSY) {
            if (dtype != DT_VOID) {
                error(ERR_WRONG_TYPE_OF_MAIN);
            }
            pMainFunctionDefinition();
            // this loop ends only when we found main
            // function's definitions
            break;
        }
        test(Symset({IDENTSY}), Symset({INTSY, CHARSY, VOIDSY}));
        if (g_sym != IDENTSY)
            continue;
        std::string id = g_id;
        readSymbol();
        test(Symset({COMMA, SEMICOLON, LBRACK, LBRACE, LPARENT}), Symset({}));
        if (g_sym == COMMA || g_sym == SEMICOLON || g_sym == LBRACK) {
            // must be variable definition
            pGlobalVariableDefinitionItem(dtype, id);
        } else {
            // must be function definition
            pFunctionDefinition(dtype, id);
            tabClear(LOCAL);    // clear local symbol table
        }
    }
    extraCodeChecking();
}

static void pFunctionDefinition(const DataType &dtype, const std::string &id)
{
    TabEntry entry = { GLOBAL, IT_FUNCTION, dtype, -1, -1 };
    tabInsert(id, entry);
    current_function_tabEntry = entry;
    std::string t = (dtype == DT_INT ? "int" :
            dtype == DT_CHAR ? "char" : "void");
    genMidCode(FUNC, t, id, NONE);

    if (g_sym == LPARENT) {
        pParametersList(id);
    }
    test(Symset({LBRACE}), Symset({}));
    readSymbol();
    pConstDefinitions(LOCAL);
    pLocalVariableDefinitions();
    pStatementsList();
    genMidCode(END, NONE, NONE, NONE);
}

/**
 * If we don't meet main function's definition in source code,
 * then a PROGRAM_INCOMPLETE error will terminate compiling
 */
static void pMainFunctionDefinition()
{
    assert(g_sym == MAINSY);
    TabEntry entry = { GLOBAL, IT_FUNCTION, DT_VOID, -1, -1 };
    tabInsert("main", entry);
    genMidCode(FUNC, "void", "main", NONE);

    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    test3(Symset({RPARENT}));
    readSymbol();
    test3(Symset({LBRACE}));
    readSymbol();
    pConstDefinitions(LOCAL);
    pLocalVariableDefinitions();
    // process statements list
    while (g_sym != RBRACE) {
        pStatement();
    }
    assert(g_sym == RBRACE);
    // NOTE: don't call readSymbol() here because 
    // source program might have no more words
    genMidCode(END, NONE, NONE, NONE);
}

static void pStatementsList()
{
    while (g_sym != RBRACE) {
        pStatement();
    }
    readSymbol();
}

static void pStatement()
{
    std::string id;
    switch (g_sym) {
        case SEMICOLON: pEmptyStatement(); break;
        case LBRACE:    readSymbol(); pStatementsList(); break;
        case IFSY:      pIfElseStatement(); break;
        case DOSY:      pDoWhileStatement(); break;
        case SWITCHSY:  pSwitchCaseStatement(); break;
        case PRINTFSY:  pPrintfStatement(); break;
        case SCANFSY:   pScanfStatement(); break;
        case RETURNSY:  pReturnStatement(); break;
        case IDENTSY:
            // must be function call statement or assignment statement
            // should be determined from symbol table
            id = g_id;
            readSymbol();
            switch (g_sym) {
                case LBRACK:    pArrayAssignmentStatement(id); break;
                case BECOMES:   pAssignmentStatement(id); break;
                case LPARENT:   
                case SEMICOLON: pFunctionCallStatement(id); break;
                default:        error(ERR_WRONG_STATEMENT); break;
            }
            break;
        default:
            test(Symset({SEMICOLON, LBRACE, IFSY, DOSY, SWITCHSY,
                PRINTFSY, SCANFSY, RETURNSY, IDENTSY}), Symset({}));
            break;
    }
}

static void pIfElseStatement()
{
    std::string if_label = genLabelIf();
    std::string else_label = genLabelElse();
    std::string end_label  = genLabelIfEnd();

    assert(g_sym == IFSY);
    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    pCondition();
    test3(Symset({RPARENT}));
    readSymbol();
    // bz: brach if not satisfy
    genMidCode(BZ, else_label, NONE, NONE);
    // if body
    genMidCode(LABEL, if_label, NONE, NONE);
    pStatement();
    genMidCode(GOTO, end_label, NONE, NONE);
    test3(Symset({ELSESY}));
    readSymbol();
    // else body
    genMidCode(LABEL, else_label, NONE, NONE);
    pStatement();
    genMidCode(LABEL, end_label, NONE, NONE);
}

/**
 * don't return value, because we will use $v0 register
 * to store result.
 */
static void pCondition()
{
    std::string left_val, right_val;
    DataType left_dtype, right_dtype;
    pExpression(left_val, left_dtype);
    if ( g_sym == EQL || g_sym == NEQ || g_sym == LSS ||
         g_sym == LEQ || g_sym == GTR || g_sym == GEQ) {
        std::string op = tokens[g_sym];
        readSymbol();
        pExpression(right_val, right_dtype);
        if (left_dtype != right_dtype) {
            error(ERR_COMPARE_TYPE_NOT_MATCH);
        }
        genMidCode(COMPARE, left_val, op, right_val);
    } else {
        // must be DT_INT
        if (left_dtype != DT_INT) {
            error(ERR_EXPECT_INT_TYPE_SINGLE_CONDITION);
        }
        genMidCode(COMPARE, left_val, NONE, NONE);
    }
}

/**
 * TODO: check duplicate cased-values
 */
static void pSwitchCaseStatement()
{
    std::string switched_val;
    DataType switched_dtype;
    std::string end_label = genLabel(); // switch end label

    assert(g_sym == SWITCHSY);
    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    pExpression(switched_val, switched_dtype);
    test3(Symset({RPARENT}));
    readSymbol();
    test3(Symset({LBRACE}));
    readSymbol(); // skip LBRACE
    pushMidCodeCacheStack();
    do {
        pCaseItem(switched_val, switched_dtype, end_label);
    } while (g_sym != RBRACE);
    // flush cached mid code here
    flushCachedMidCode();
    genMidCode(LABEL, end_label, NONE, NONE);
    readSymbol(); // skip RBRACE
}

static void pCaseItem(const std::string &switched_val, 
        const DataType &switched_dtype,
        const std::string &end_label)
{
    // TODO: default clause must be behind of case clause, 
    // which is prescribed by C0 grammar rules.
    // TODO: values in cases can't be equal
    // TODO: might sort case item's and apply binary search
    // to improve switch-case's performance.
    std::string cased_val;
    DataType cased_dtype;
    std::string case_label = genLabel();

    test3(Symset({CASESY, DEFAULTSY}));
    if (g_sym == CASESY) {
        readSymbol();
        // cased-value must be int or char literal, can't be 
        // const identifier
        test3(Symset({CHARVALUE, MINUS, PLUS, INTVALUE}));
        if (g_sym == CHARVALUE) {
            std::stringstream ss;
            ss << "\'" << g_char << "\'";
            cased_val = ss.str();
            cased_dtype = DT_CHAR;
            readSymbol();
        } else { // g_sym == INTVALUE
            pSignedInteger();
            cased_val = std::to_string(g_num);
            cased_dtype = DT_INT;
        }
        if (switched_dtype != cased_dtype) {
            error(ERR_SWITCH_TYPE_NOT_MATCH);
            std::stringstream ss;
            ss << "$witched=" << switched_val << "(" << dtype2str[switched_dtype]
               << ") $cased=" << cased_val << "(" << dtype2str[cased_dtype]
               << ")" << std::endl;
            error(ss.str());
            return;
        }
        test3(Symset({COLON}));
        readSymbol();
        case_label = genLabel();
        // generate if...goto... without cache
        // TODO: optimize switch, we don't have to load
        // switched_val every time when compare 
        genMidCode(COMPARE, switched_val, tokens[EQL], cased_val);
        genMidCode(BNZ, case_label, NONE, NONE);
        // generate labels and statements body in cache stack
        startCachingMidCode();
        genMidCode(LABEL, case_label, NONE, NONE); 
        pStatement();
        genMidCode(GOTO, end_label, NONE, NONE);
        // stop cache
        pauseCachingMidCode();

    } else {
        // default:
        readSymbol();
        test3(Symset({COLON}));
        readSymbol();
        // this label is not necessary, but it improves
        // readability of generated mid code and final 
        // mips code.
        case_label = genLabel();
        genMidCode(LABEL, case_label, NONE, NONE);
        // no cache for default clause
        pStatement();
        genMidCode(GOTO, end_label, NONE, NONE);
    }

}

static void pDoWhileStatement()
{
    std::string begin_label = genLabel();

    assert(g_sym == DOSY);
    readSymbol();
    genMidCode(LABEL, begin_label, NONE, NONE);
    pStatement();
    test3(Symset({WHILESY}));
    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    pCondition();
    // BNZ: branch if satisfy
    genMidCode(BNZ, begin_label, NONE, NONE);
    test3(Symset({RPARENT}));
    readSymbol();
}

static void pPrintfStatement()
{
    std::string arg1, arg2;
    DataType temp_dtype;
    std::stringstream ss;
    assert(g_sym == PRINTFSY);
    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    if (g_sym == STRVALUE) {
        // insert string to strings table and get a
        // label for this string.
        arg1 = string2label(g_str);
        genMidCode(WRITE, "str", arg1, NONE);
        readSymbol();
        if (g_sym == COMMA) {
            readSymbol();
            pExpression(arg2, temp_dtype);
            std::string t = (temp_dtype == DT_INT ? "int" : "char");
            genMidCode(WRITE, t, arg2, NONE);
        }
    } else {
        pExpression(arg1, temp_dtype);
        std::string t = (temp_dtype == DT_INT ? "int" : "char");
        genMidCode(WRITE, t, arg1, NONE);
    }
    // break line for each printf
    //std::string newline = string2label("\\n");
    //genMidCode(WRITE, "str", newline, NONE);
    test3(Symset({RPARENT}));
    readSymbol();
    test3(Symset({SEMICOLON}));
    readSymbol();
}

static void pScanfStatement()
{
    TabEntry entry;

    assert(g_sym == SCANFSY);
    readSymbol();
    test3(Symset({LPARENT}));
    readSymbol();
    int count = 0;
    while (true) {
        count++;
        test3(Symset({IDENTSY}));
        readSymbol();
        if (!tabFind(g_id, entry)) {
            error(ERR_UNDEFINED_IDENTIFIER);
        } 
        else if (entry.itype != IT_VARIABLE || (
                 entry.dtype != DT_INT &&
                 entry.dtype != DT_CHAR)) {
            error(ERR_WRONG_TYPE_OF_SCANF);
        }
        genMidCode(READ, entry.dtype == DT_INT ? "int" : "char", g_id, NONE);
        if (g_sym != COMMA) {
            break;
        }
        readSymbol();
    }
    if (count == 0) {
        error(ERR_SCANF_NO_ARGUMENTS);
    }
    test3(Symset({RPARENT}));
    readSymbol();
    test3(Symset({SEMICOLON}));
    readSymbol();
}

static void pReturnStatement()
{
    std::string ret_val;
    DataType ret_dtype;

    assert(g_sym == RETURNSY);
    readSymbol();
    test3(Symset({LPARENT, SEMICOLON}));
    if (g_sym == LPARENT) {
        readSymbol();
        pExpression(ret_val, ret_dtype);
        genMidCode(RET, ret_val, NONE, NONE);
        test3(Symset({RPARENT}));
        readSymbol();
    } else {
        ret_dtype = DT_VOID;
        genMidCode(RET, NONE, NONE, NONE);
    }
    if (ret_dtype != current_function_tabEntry.dtype) {
        error(ERR_WRONG_RETURN_TYPE);
    }
    test3(Symset({SEMICOLON}));
    readSymbol();
}

static void pEmptyStatement()
{
    assert(g_sym == SEMICOLON);
    readSymbol();
}

static void pAssignmentStatement(const std::string &id)
{
    TabEntry entry;

    assert(g_sym == BECOMES);
    readSymbol();
    if (!tabFind(id, entry)) {
        error(ERR_UNDEFINED_IDENTIFIER);
        return;
    }
    if (entry.itype != IT_VARIABLE) {
        // entry.itype might be IT_ARRAY
        error(ERR_LEFT_VALUE_NOT_VARIABLE);
        return;
    }
    std::string rvalue;     // right value
    DataType rvalue_dtype;  // right value's data type
    pExpression(rvalue, rvalue_dtype);
    if (rvalue_dtype != entry.dtype) {
        error(ERR_TYPE_NOT_MATCH);
        return;
    }
    genMidCode(ASSIGN, rvalue, NONE, id);
    test3(Symset({ SEMICOLON }));
    readSymbol();
}

static void pArrayAssignmentStatement(const std::string &id)
{
    TabEntry entry;

    assert(g_sym == LBRACK);
    if (!tabFind(id, entry)) {
        error(ERR_UNDEFINED_IDENTIFIER);
        return;
    }
    if (entry.itype != IT_ARRAY) {
        error(ERR_NOT_AN_ARRAY);
        return;
    }
    readSymbol(); // skip left bracket
    // handle index
    std::string index;    // index
    DataType index_dtype; // index's data type
    pExpression(index, index_dtype);
    if (index_dtype != DT_INT) {
        // array's index must be int type
        error(ERR_EXPECT_INT_ARRAY_INDEX);
    }
    // overflow check for const indexes
    int val;
    if (isConstValue(index, val) &&
        (val < 0 || val >= entry.value)) {
        error(ERR_ARRAY_INDEX_OVERFLOW);
    }
    readSymbol(); // skip right bracket
    test3(Symset({ BECOMES }));
    readSymbol(); // skip = 
    // handle right value
    std::string rvalue;
    DataType rvalue_dtype;
    pExpression(rvalue, rvalue_dtype);
    if (rvalue_dtype != entry.dtype) {
        error(ERR_TYPE_NOT_MATCH);
    }
    // generate middle code
    genMidCode(WARRAY, id, index, rvalue);
    test3(Symset({ SEMICOLON }));
    readSymbol();
}

static void pArrayRead(std::string &res, DataType &res_dtype,
        const std::string &id, const TabEntry &entry)
{
    if (g_sym != LBRACK) {
        error(ERR_EXPECT_ARRAY_ELEMENT);
    }
    test3(Symset({LBRACK}));
    readSymbol();  // skip left bracket
    // handle index
    std::string index;    // index
    DataType index_dtype; // index's data type
    pExpression(index, index_dtype);
    if (index_dtype != DT_INT) {
        // array's index must be int type
        error(ERR_EXPECT_INT_ARRAY_INDEX);
    }
    // overflow check for const indexes
    int val;
    if (isConstValue(index, val) &&
        (val < 0 || val >= entry.value)) {
        error(ERR_ARRAY_INDEX_OVERFLOW);
    }
    res = genTempVar();
    res_dtype = entry.dtype;
    genMidCode(TEMP, (res_dtype == DT_INT ? "int": "char"), res, NONE);
    genMidCode(RARRAY, id, index, res);
    test3(Symset({RBRACK}));
    readSymbol();
}

/**
 * This function handles single function call statement,
 * regardless of void or non-void functions.
 */
static void pFunctionCallStatement(const std::string &id)
{
    TabEntry entry;
    if (!tabFind(id, entry)) {
        error(ERR_UNDEFINED_IDENTIFIER);
        return;
    }
    if (entry.itype != IT_FUNCTION) {
        error(ERR_NOT_A_FUNCTION);
        return;
    }
    const std::vector<DataType> &params = tabGetParams(id);
    if (g_sym == LPARENT) {
        pArgumentsList(params);
    } else if (params.size() != 0) {
        error(ERR_EXPECT_ARGUMENTS);
        return;
    }
    genMidCode(CALL, id, std::to_string(params.size()), NONE);
    // as we don't call about return value, so we don't 
    // need to copy return value from $RET to some varaible
    test2(Symset({SEMICOLON}), Symset({}));
    readSymbol();
}

/**
 * Non-void function call is only used in <expression>.
 * Functions called in expressions must be non-void function.
 * :res         containing return value of function call
 * :res_dtype   containing value's data type
 * :id          current function's identifier 
 * :entry       symbol table entry of current function
 */
static void pNonVoidFunctionCall(std::string &res, DataType &res_dtype, 
        const std::string &id, const TabEntry &entry)
{
    assert(entry.itype == IT_FUNCTION);
    if (entry.dtype == DT_VOID) {
        error(ERR_EXPECT_NON_VOID_FUNCTION);
        return;
    }
    const std::vector<DataType> &params = tabGetParams(id);

    if (g_sym == LPARENT) {
        pArgumentsList(params);
    } else if (params.size() != 0) {
        error(ERR_EXPECT_ARGUMENTS);
        return;
    }
    genMidCode(CALL, id, std::to_string(params.size()), NONE);
    // create a temp variable for holding return value
    res = genTempVar(); 
    res_dtype = entry.dtype;
    genMidCode(TEMP, (res_dtype == DT_INT ? "int": "char"), res, NONE);
    // return statement in non-void functions will always
    // write return value to a fixed register $v0
    // get return value from $t0 and save it to a temp variable
    genMidCode(GETRET, NONE, NONE, res);
}

/**
 * pParametersList is used for function's definition
 * pArgumentsList is used for function call
 * id: function's identifier
 */
static void pArgumentsList(const std::vector<DataType> &params)
{
    std::string temp_var;
    DataType temp_dtype;
    unsigned int args_count = 0;
    std::vector<FourTuple> to_be_pushed;

    assert(g_sym == LPARENT);
    readSymbol();
    while (true) {
        pExpression(temp_var, temp_dtype);
        if (++args_count > params.size()) {
            error(ERR_MORE_ARGUMENTS);
            return;
        }
        // check argument's data type
        if (params[args_count - 1] != temp_dtype) {
            error(ERR_WRONG_TYPE_OF_ARGUMENT);
            return;
        }
        std::string type = (temp_dtype == DT_INT ? "int" : "char");
        startCachingMidCode();
        to_be_pushed.push_back(FourTuple({PUSH, type, temp_var, NONE}));
        pauseCachingMidCode();
        if (g_sym != COMMA) {
            break;
        }
        readSymbol();
    }
    for (const auto &ft: to_be_pushed) {
        genMidCode(ft.op, ft.a, ft.b, ft.res);
    }
    if (args_count < params.size()) {
        error(ERR_LESS_ARGUMENTS);
        return;
    }
    test3(Symset({RPARENT}));
    readSymbol();
}

/**
 * pParametersList is used for function's definition
 * pArgumentsList is used for function call
 * id: function's identifier
 */
static void pParametersList(const std::string &id)
{
    int count = 0;

    assert(g_sym == LPARENT);
    while (g_sym != RPARENT) { 
        count++;

        readSymbol(); // g_sym==COMMA or LPARENT
        test2(Symset({INTSY, CHARSY}), Symset({RPARENT}));
        DataType dtype = g_sym == INTSY ? DT_INT: DT_CHAR;

        readSymbol(); // identifier
        test2(Symset({IDENTSY}), Symset({RPARENT}));
        TabEntry entry = { LOCAL, IT_VARIABLE, dtype, -1, -1 };
        tabInsert(g_id, entry);
        tabInsertParam(id, dtype);
        genMidCode(PARA, dtype == DT_INT ? "int" : "char", g_id, NONE);

        readSymbol();
        test3(Symset({COMMA, RPARENT}));
        if (g_sym != COMMA) 
            break;
    }
    if (count == 0) {
        error(ERR_MISSING_PARAMETERS);
    }
    readSymbol();
}

static void pGlobalVariableDefinitionItem(DataType dtype, std::string id) 
{
    if (dtype == DT_VOID) {
        error(ERR_WRONG_VARIABLE_TYPE);
    }
    std::string type_str = (dtype == DT_INT ? "int": "char");
    while (true) {
        // g_sym always points to the one after identifier
        if (g_sym == LBRACK) {       // array definition
            readSymbol(); // skip left bracket
            test3(Symset({INTVALUE}));
            int array_size = g_num;
            assert(g_num >= 0);
            if (array_size == 0) {
                error(ERR_ARRAY_SIZE_ZERO);
            }
            TabEntry entry = { GLOBAL, IT_ARRAY, dtype, array_size, -1 };
            tabInsert(id, entry);
            genMidCode(GVAR, type_str, id, std::to_string(array_size));
            readSymbol(); // skip size number
            test3(Symset({RBRACK}));
            readSymbol(); 
        } else {                    // normal variable definition
            TabEntry entry = { GLOBAL, IT_VARIABLE, dtype, -1, -1 };
            tabInsert(id, entry);
            genMidCode(GVAR, type_str, id, NONE);
        }
        if (g_sym != COMMA) {
            break;
        }
        readSymbol();
        test3(Symset({IDENTSY}));
        id = g_id;
        readSymbol();
    }
    test3(Symset({SEMICOLON}));
    readSymbol();
}

static void pLocalVariableDefinitions()
{
    while (g_sym == INTSY || g_sym == CHARSY) {
        DataType dtype = (g_sym == INTSY ? DT_INT : DT_CHAR);
        std::string type_str = (dtype == DT_INT ? "int" : "char");
        while (true) {
            // g_sym always points to the one before idnetifier
            readSymbol(); // skip comma or INTSY or CHARSY
            test3(Symset({IDENTSY}));
            std::string id = g_id;
            readSymbol(); // skip identifier
            if (g_sym == LBRACK) {      // array definition
                readSymbol(); // skip left bracket
                test3(Symset({INTVALUE}));
                int array_size = g_num;
                TabEntry entry = { LOCAL, IT_ARRAY, dtype, array_size, -1 };
                tabInsert(id, entry);
                genMidCode(VAR, type_str, id, std::to_string(array_size));
                readSymbol(); // skip size number
                test3(Symset({RBRACK}));
                readSymbol();
            } else {                    // normal variable definition
                TabEntry entry = { LOCAL, IT_VARIABLE, dtype, -1, -1 };
                tabInsert(id, entry);
                genMidCode(VAR, type_str, id, NONE);
            }
            if (g_sym != COMMA) {
                break;
            }
        }
        test3(Symset({SEMICOLON}));
        readSymbol();
    }
}

/* process one const definition */
static void pConstDefinition(IdentScope scope)
{
    assert(g_sym == CONSTSY);
    readSymbol();
    test3(Symset({INTSY, CHARSY}));
    DataType dtype = g_sym == INTSY ? DT_INT : DT_CHAR;
    TabEntry entry;
    while (true) {
        readSymbol(); // skip INTSY, CHARSY or COMMA
        test3(Symset({IDENTSY}));
        std::string id = g_id;
        readSymbol();
        test3(Symset({BECOMES}));
        readSymbol();
        if (dtype == DT_INT) {
            pSignedInteger();
            entry = { scope, IT_CONST, DT_INT, g_num, -1 };
        } else { // DT_CHAR
            test3(Symset({CHARVALUE}));
            entry = { scope, IT_CONST, DT_CHAR, g_char, -1 };
            readSymbol();
        }
        tabInsert(id, entry);
        if (g_sym != COMMA) {
            break;
        }
    }
    test(Symset({SEMICOLON}), Symset({SEMICOLON, RBRACE, CONSTSY}));
    if (g_sym == CONSTSY)
        return;
    test3(Symset({SEMICOLON}));
    readSymbol();
}

static void pConstDefinitions(IdentScope scope)
{
    // g_sym always points to the first word after semicolon
    while (g_sym == CONSTSY) {
        pConstDefinition(scope);
    }
}


/**
 * Process signed integer
 *
 * g_num is an unsigned integer read in by readSymbol()
 * there might be '+' or '-' in front of it.
 * if so, we combile they together and get a singed integer
 * literal. Store it back to g_num
 */
static void pSignedInteger() 
{
    int negtive = 1;
    if (g_sym == PLUS || g_sym == MINUS) {
        negtive = g_sym == MINUS ? -1 : 1;
        readSymbol();
    }
    test3(Symset({INTVALUE}));
    g_num *= negtive;
    readSymbol();
}

/**
 * res and res_dtype is used to return
 * code became so ugly because we need to 
 */
static void pExpression(std::string &res, DataType &res_dtype)
{
    std::string temp_var;
    DataType temp_dtype;
    if (g_sym == PLUS || g_sym == MINUS) {
        bool isMinus = (g_sym == MINUS);
        readSymbol();
        if (isMinus) {
            pTerm(temp_var, temp_dtype); // no use of tempvar_dtype
            int val;
            if (isConstValue(temp_var, val)) {
                res = std::to_string(-val);
                res_dtype = DT_INT;
            } else {
                res = genTempVar();
                res_dtype = DT_INT;
                genMidCode(TEMP, "int", res, NONE);
                genMidCode(SUB, "0", temp_var, res);
            }
        } else {
            // no need to generate code for '+'
            pTerm(res, res_dtype);
            int t;
            if (isConstValue(res, t)) {
                res = std::to_string(t);
            }
            res_dtype = DT_INT;
        }
    } else {
        pTerm(res, res_dtype);
    }
    while (g_sym == PLUS || g_sym == MINUS) {
        OpCode op = (g_sym == PLUS ? ADD : SUB);
        readSymbol();
        pTerm(temp_var, temp_dtype); // no use of tempvar_dtype
        int t1, t2;
        if (isConstValue(res, t1) && isConstValue(temp_var, t2)) {
            // if both operands ares const value, we can calculate it
            res = std::to_string( op == ADD ? t1 + t2 : t1 - t2 );
            res_dtype = DT_INT;
        }
        else {
            std::string new_res = genTempVar();
            genMidCode(TEMP, "int", new_res, NONE);
            genMidCode(op, res, temp_var, new_res);
            res = new_res;
            res_dtype = DT_INT;
        }
    }
}

/* Note that res and res_dtype are reference */
static void pTerm(std::string &res, DataType &res_dtype)
{
    std::string temp_var;
    DataType temp_dtype;
    pFactor(res, res_dtype);
    while (g_sym == STAR || g_sym == SLASH) {
        OpCode op = (g_sym == STAR ? MUL : DIV);
        readSymbol();
        pFactor(temp_var, temp_dtype);
        int t1, t2;
        if (isConstValue(res, t1) && isConstValue(temp_var, t2)) {
            res = std::to_string( op == MUL ? t1 * t2 : t1 / t2 );
            res_dtype = DT_INT;
        }
        else {
            std::string new_res = genTempVar();
            genMidCode(TEMP, "int", new_res, NONE);
            genMidCode(op, res, temp_var, new_res);
            res = new_res;
            res_dtype = DT_INT;
        }
    }
}

/* Note that res and res_dtype are reference */
static void pFactor(std::string &res, DataType &res_dtype)
{
    std::string id, temp_var;
    std::stringstream ss;
    TabEntry entry;
    DataType temp_dtype;
    // For conveniency, we assume all identifiers here are non-void-function-call
    test3(Symset({IDENTSY, CHARVALUE, LPARENT, PLUS, MINUS, INTVALUE}));
    switch (g_sym) {
        case IDENTSY:   id = g_id;
                        readSymbol(); 
                        if (!tabFind(id, entry)) {
                            // undefined identifier
                            error(ERR_UNDEFINED_IDENTIFIER);
                            return;
                        }
                        // entry.itype might be:
                        // IT_FUNCTION, IT_ARRAY, IT_VARIABLE, IT_CONST

                        // 1. handle functions
                        if (entry.itype == IT_FUNCTION) {
                            pNonVoidFunctionCall(res, res_dtype, id, entry);
                            return;
                        } else if (g_sym == LPARENT) { 
                            // use function call on a not-a-function identifier
                            error(ERR_NOT_A_FUNCTION);
                            return;
                        }
                        // 2. handle arrays
                        if (entry.itype == IT_ARRAY) {
                            pArrayRead(res, res_dtype, id, entry);
                            return;
                        }
                        assert(entry.itype == IT_CONST || entry.itype == IT_VARIABLE);
                        assert(entry.dtype == DT_CHAR || entry.dtype == DT_INT);
                        // 3. handle consts variable
                        if (entry.itype == IT_CONST) {
                            // return consts' character directly
                            if (entry.dtype == DT_CHAR) {
                                res_dtype = DT_CHAR;
                                ss << '\'' << (char)entry.value << '\'';
                            } else {
                                res_dtype = DT_INT;
                                ss << entry.value;
                            }
                            res = ss.str();
                        } 
                        // 4. handle normal variable (int or char)
                        else {
                            // return identifier directly
                            res = id;
                            res_dtype = entry.dtype;
                        }
                        break;
        case CHARVALUE: readSymbol();
                        ss << '\'' << g_char << '\'';
                        res = ss.str();
                        res_dtype = DT_CHAR;
                        break;
        case LPARENT:   readSymbol();
                        pExpression(res, temp_dtype);
                        res_dtype = DT_INT;           // ('p') should be convert to int
                        test3(Symset({RPARENT}));
                        readSymbol();
                        break;
        default:        pSignedInteger();  // a signed integer is stored at g_num
                        ss << g_num;
                        res = ss.str();
                        res_dtype = DT_INT;
                        break;
    }
}


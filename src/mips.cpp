#include <iostream>         // cout, ostream    
#include <cassert>          // assert
#include <sstream>          // stringstream
#include <streambuf>        // streambuf
#include <utility>          // swap
#include <stack>
#include "mips.h"
#include "common.h"
#include "midcode.h"
#include "table.h"


#define HT  "\t"
#define SIZE_INT      4
#define SIZE_CHAR     4

/**
 * Runtime Stack
 * |***********Top**********|  
 * |-------   $ra   --------|  1. return address  
 * |-------  para1  --------|  2. params area     
 * |-------  para2  --------|  
 * |-------  para3  --------|   
 * |-------   ...   --------|
 * |------variable 1--------|  3. varialbes area
 * |------varialbe 2--------|
 * |------varialbe 3--------|
 * |------    ...   --------| 
 * |------temp var 1--------|  4. temp varialbes area
 * |------temp var 2--------|
 * |------temp var 3--------|
 * |------    ...   --------|
 *
 */


extern std::vector<FourTuple>               mid_codes;
static std::vector<FourTuple>::iterator     m;
static int                                  indent_num;                     
static std::stringstream                    T;

static std::string  cur_func_id;
static int          cur_func_size;

static int          prev_para_addr;
/**
 * How do i pass parameters for function call ?
 *
 * Before a function call, we directly push arguments
 * to callee function's parameter area. For example:
 *
 *                    PUSH 1          li   $v0, 1
 *                                    sw   $v0, -8($sp)
 * add(1, 3);  ===>   PUSH 3    ===>  li   $v0, 3
 *                                    sw   $v0, -12($sp) 
 *                    call add        jal  add
 *
 * 0($sp) is current function's last variable's address.
 * -4($sp)  is $ra backup's addresss of `call()` function
 * -8($sp)  is the first parameter's address of `call()` function
 * -12($sp) is the second parameter's address of `call()` function
 * 
 * Remember to reset next_para_addr like below:
 *  next_paran_addr = 0;
 */

void convertToMIPS();
static void MIPS(std::ostream &ss);
static void gen_global_variables();
static void gen_strings();
static void gen_start_code();
static void gen_FUNC();
static void gen_PUSH(const FourTuple &ft);
static void gen_CALL(const FourTuple &ft);
static void gen_WRITE(const FourTuple &ft); // printf
static void gen_READ(const FourTuple &ft);  // scanf
static void gen_ADD_SUB_MUL_DIV(const FourTuple &ft);
static void gen_RARRAY_WARRAY(const FourTuple &ft);
static void gen_ASSIGN(const FourTuple &ft);
static void gen_GETRET(const FourTuple &ft);
static void gen_GOTO(const FourTuple &ft);
static void gen_LABEL(const FourTuple &ft);
static void gen_RET(const FourTuple &ft);
static void gen_END();
static void loadToReg(const std::string &reg, const std::string &t);
static std::string getVariableAddr(const std::string &t);
static void gen_COMPARE(const FourTuple &ft);
// ident to make mips code more beautiful
static void indent();
static void unindent();
static void indent4();
static void unindent4();

static void MIPS(std::ostream &ss)
{
    static std::string prev_op = "";
    static std::string prev_code = "";
    std::stringstream indent_blanks;
    for (int i = 0; i < indent_num; i++) {
        mipscode_stream << ' ';
    }
    std::ostringstream oss;
    oss << ss.rdbuf();
    std::string mips_code = oss.str();
    mipscode_stream << mips_code << std::endl;
    bool should_omit = false;
    if (mips_code.substr(0, 2) == "lw") {
        if (prev_op == "sw" && mips_code.substr(2) == prev_code.substr(2)) {
            should_omit = true;
        } else {
            prev_op = "lw";
            prev_code = mips_code;
        }
    } else if (mips_code.substr(0, 2) == "sw") {
        if (prev_op == "lw" && mips_code.substr(2) == prev_code.substr(2)) {
            should_omit = true;
        } else {
            prev_op = "sw";
            prev_code = mips_code;
        }
    } else {
        prev_op = "";
    }
    if (!should_omit) {
        for (int i = 0; i < indent_num; i++) {
            opt_mipscode_stream << ' ';
        }
        opt_mipscode_stream << mips_code << std::endl;
    }
}

static void gen_global_variables()
{
    while ((*m).op == GVAR) {
        const FourTuple &ft = *m;
        // format: GVAR, int|char, id, NONE
        assert(ft.a == "int" || ft.a == "char");
        // use same size for char and int
        // TODO: might use .byte for char
        if (ft.res != "") {
            // array, ft.res is the size of array
            MIPS(T << ft.b << ":" << HT << ".word" << HT << "0:" << ft.res);
        } else {
            // normal variable
            MIPS(T << ft.b << ":" << HT << ".word" << HT << "0");
        }
        m++;
    }
}

static void gen_strings()
{
    extern std::map<std::string, std::string> strings_table;
    for (auto const& item : strings_table) {
        MIPS(T << item.second << ": " << ".asciiz \""
               << item.first << "\"");
    }
}

static void gen_start_code()
{
    MIPS(T << "jal"<< HT << "main");
    // use syscall to tell mars simulator that program finished
    MIPS(T << "li" << HT << "$v0, " << 10);
    MIPS(T << "syscall");
}

void convertToMIPS()
{
    m = mid_codes.begin();

    MIPS(T << ".data");
    indent();
    gen_global_variables();
    gen_strings();
    unindent();
    MIPS(T << ".text");
    // jump to main function
    indent();
    gen_start_code();
    unindent();
    // conver functions
    assert((*m).op == FUNC);
    while (m != mid_codes.end() && (*m).op == FUNC)
        gen_FUNC();
}

/**
 * All local variables, parameters and temp variables
 * are inserted to symbol table and are given a relative
 * address.
 *
 * `cur_func_size` hold current functions memory size, which
 * is needed for function call
 */
static void buildSymbolTable()
{
    assert((*m).op == FUNC);

    tabClear(LOCAL);
    cur_func_id = (*m).b;
    cur_func_size = 4;  // reserved for $ra

    std::stack<TabEntry> entries;
    for (auto t = m + 1; (*t).op != END; t++) {
        const FourTuple &ft = *t;
        if (ft.op == PARA || ft.op == VAR || ft.op == TEMP) {
            DataType dtype = ft.a == "int" ? DT_INT : DT_CHAR;
            int data_type_size = (dtype == DT_INT) ? SIZE_INT: SIZE_CHAR;
            int array_size = 1; // array size
            if (ft.res != "") { 
                array_size = std::stoi(ft.res);
            }
            cur_func_size += data_type_size * array_size;
        }
    }

    // Important
    prev_para_addr = -4;

    int addr = cur_func_size - 4;
    for (auto t = m + 1; (*t).op != END; t++) {
        const FourTuple &ft = *t;
        if (ft.op == PARA || ft.op == VAR || ft.op == TEMP) {
            DataType dtype = ft.a == "int" ? DT_INT : DT_CHAR;
            int data_type_size = (dtype == DT_INT) ? SIZE_INT : SIZE_CHAR;
            int array_size = 1;
            bool isArray = false;
            if (ft.res != "") {
                array_size = std::stoi(ft.res);
                isArray = true;
            }
            addr -= data_type_size * array_size;
            IdentType itype = (isArray ? IT_ARRAY : IT_VARIABLE);
            TabEntry entry = { LOCAL, itype, dtype, -1, addr };
            tabInsert(ft.b, entry);
        }
    }
}

static void gen_FUNC()
{
    // mid-code format: FUNC, int|char|void, id
    assert((*m).op == FUNC);

    // generate label for function
    // (*m).b if the function name
    MIPS(T << (*m).b << ":"); 

    buildSymbolTable();
    assert((*m).op == FUNC);
    m++;

    indent();
    // allocate memory from stack
    MIPS(T << "addiu" << HT << "$sp, $sp, " << -cur_func_size);
    MIPS(T << "sw" << HT << "$ra, "<< (cur_func_size - 4) << "($sp)");

    while ((*m).op != END) {
        switch ((*m).op) {
            case ADD:
            case SUB: 
            case MUL:
            case DIV:   gen_ADD_SUB_MUL_DIV(*m); break;
            case WARRAY:
            case RARRAY:gen_RARRAY_WARRAY(*m); break;
            case PUSH:  gen_PUSH(*m); break;
            case CALL:  gen_CALL(*m); break;
            case WRITE: gen_WRITE(*m); break;
            case READ:  gen_READ(*m); break;
            case ASSIGN:gen_ASSIGN(*m); break;
            case GETRET:gen_GETRET(*m); break;
            case GOTO:  gen_GOTO(*m); break;
            case LABEL: gen_LABEL(*m); break;
            case RET:   gen_RET(*m); break;
            case COMPARE: gen_COMPARE(*m); break;
            case VAR:   break;
            case PARA:  break;
            case TEMP:  break;
            default:    MIPS(T << "unhandled:" << op2str[(*m).op]); break;
        }
        m++;
    }
    gen_END();

    unindent();
    m++;
}


static void gen_PUSH(const FourTuple &ft)
{
    DataType dtype = (ft.a == "int" ? DT_INT : DT_CHAR);
    prev_para_addr -= (dtype == DT_INT ? SIZE_INT : SIZE_CHAR);
    loadToReg("$v0", ft.b);
    MIPS(T << "sw" << HT << "$v0, " << prev_para_addr << "($sp)");
}

static void gen_CALL(const FourTuple &ft)
{
    MIPS(T << "jal" << HT << ft.a);
    // Important: reset this variable for
    // next function call
    prev_para_addr = -4;
}

static void gen_WRITE(const FourTuple &ft)
{
    assert(ft.a == "int" || ft.a == "str" || ft.a == "char");
    if (ft.a == "str") {
        MIPS(T << "la" << HT << "$a0, " << ft.b);
        MIPS(T << "li" << HT << "$v0, 4");
    }
    else if (ft.a == "int") {
        loadToReg("$a0", ft.b);
        MIPS(T << "li" << HT << "$v0, 1");
    }
    else { // ft.a == "char"
        loadToReg("$a0", ft.b);
        MIPS(T << "li" << HT << "$v0, 11");
    }
    MIPS(T << "syscall");
}

static void gen_READ(const FourTuple &ft)
{
    assert(ft.a == "int" || ft.a == "char");
    if (ft.a == "int") {
        MIPS(T << "li" << HT << "$v0, 5");
    } 
    else {
        MIPS(T << "li" << HT << "$v0, 12");
    }
    MIPS(T << "syscall");
    MIPS(T << "sw" << HT << "$v0, " << getVariableAddr(ft.b));
}

static void gen_ADD_SUB_MUL_DIV(const FourTuple &ft)
{
    int ignored;

    /* use pseudo instructions here */
    std::string op = (ft.op == ADD ? "addu" :
            ft.op == SUB ? "subu" : 
            ft.op == MUL ? "mul" :
            ft.op == DIV ? "div" : "FUCK");
    // can't be both const value
    assert(!(isConstValue(ft.a, ignored) && isConstValue(ft.b, ignored)));


    loadToReg("$v0", ft.a);
    std::string operand1 = "$v0";
    std::string operand2 = "$v1";
    if (isConstValue(ft.b, ignored)) {
        operand2 = std::to_string(ignored);
    } else {
        loadToReg("$v1", ft.b);
    }

    // result is always stored into $v0
    MIPS(T << op << HT << "$v0, " << operand1 << ", " << operand2);
    // write result back
    MIPS(T << "sw" << HT << "$v0, " << getVariableAddr(ft.res));
}

static void gen_ASSIGN(const FourTuple &ft)
{
    loadToReg("$v0", ft.a);
    MIPS(T << "sw" << HT << "$v0, " << getVariableAddr(ft.res));
}

static void gen_GETRET(const FourTuple &ft)
{
    MIPS(T << "sw" << HT << "$v0, " << getVariableAddr(ft.res));
}

static void gen_GOTO(const FourTuple &ft)
{
    MIPS(T << "j" << HT << ft.a);
}

static void gen_LABEL(const FourTuple &ft)
{
    unindent4();
    MIPS(T << ft.a << ":");
    indent4();
}

static void gen_RET(const FourTuple &ft)
{
    if (ft.a != "") {
        loadToReg("$v0", ft.a);
    }
    gen_END();
}

static void gen_END()
{
    // loads $ra from memory
    MIPS(T << "lw" << HT << "$ra, " << cur_func_size - 4 << "($sp)");
    // restore $sp 
    MIPS(T << "addiu" << HT << "$sp, $sp, " << cur_func_size);
    MIPS(T << "jr" << HT << "$ra");
}

static void gen_RARRAY_WARRAY(const FourTuple &ft) 
{
    // format:
    //      * WARRAY, arr, idx, value
    //      * RARRAY, arr, idx, target
    assert(ft.op == RARRAY || ft.op == WARRAY);
    TabEntry entry;
    
    bool flag = tabFind(ft.a, entry);
    assert(flag == true);
    assert(entry.itype == IT_ARRAY);

    // Step 1: load index(offset) to register $v0
    int idx_val;
    if (isConstValue(ft.b, idx_val)) {
        // for const values, we calculate it's actual
        // offset without a multiplication
        loadToReg("$v0", std::to_string(idx_val * 4));
    } else {
        loadToReg("$v0", ft.b);
        // might use shift operate to improve performance
        MIPS(T << "mul" << HT << "$v0, $v0, 4");
    }

    // Step 2-1: handle global arrays
    if (entry.scope == GLOBAL) {
        if (ft.op == RARRAY) {
            MIPS(T << "lw" << HT << "$v1, " << ft.a << "($v0)");
            MIPS(T << "sw" << HT << "$v1, " << getVariableAddr(ft.res));
        } else {
            loadToReg("$v1", ft.res);
            MIPS(T << "sw" << HT << "$v1, " << ft.a << "($v0)");
        }
        return;
    } 

    assert(entry.scope == LOCAL);
    // Step 2-2: handle local arrays
    // calculate array element's memory address
    MIPS(T << "addu" << HT << "$v0, $v0, $sp");
    if (ft.op == RARRAY) {
        MIPS(T << "lw" << HT << "$v1, " << entry.addr << "($v0)");
        MIPS(T << "sw" << HT << "$v1, " << getVariableAddr(ft.res));
    } else {
        loadToReg("$v1", ft.res);
        MIPS(T << "sw" << HT << "$v1, " << entry.addr << "($v0)");
    }
}

/**
 * Load a const value or a variable to a register
 */
static void loadToReg(const std::string &reg, const std::string &t)
{
    TabEntry entry;
    int val;

    if (isConstValue(t, val)) {
        MIPS(T << "li" << HT << reg << ", " << val);
        return;
    }
    bool flag = tabFind(t, entry);
    assert(flag == true);
    if (entry.scope == GLOBAL) {
        MIPS(T << "lw" << HT << reg << ", " << t);
    } else {
        MIPS(T << "lw" << HT << reg << ", " << entry.addr << "($sp)");
    }
}

/**
 * Get variable's address, 
 *    global variable,
 *    local variable, parameters, temp variables,
 */
static std::string getVariableAddr(const std::string &t)
{
    TabEntry entry;
    bool flag = tabFind(t, entry);
    if (!flag) {
        std::cout << "Did you forget the TEMP mid-code for temp var?"
                  << std::endl;
    }
    assert(flag == true);
    if (entry.scope == GLOBAL) {
        return t;
    } else {
        return std::to_string(entry.addr) + "($sp)";
    }
}

/**
 * This function is a little bit ugly, because there're
 * too many cases to be considered, and i wan't to generate
 * less mips code.
 *
 *
 * TODO: consider the two operand in comparison are equal, like
 * good >= good  -->   always true
 * bad !=  bad   -->   always false
 */
static void gen_COMPARE(const FourTuple &ft)
{
    int val1, val2;
    m++;
    assert((*m).op == BZ || (*m).op == BNZ);
    // For const values, we can use a goto directly
    if (isConstValue(ft.a, val1) && 
        (ft.b == "" || isConstValue(ft.res, val2))) {
        if (ft.b != "") {
            val1 = (ft.b == "EQL" ? val1 == val2 :
                    ft.b == "NEQ" ? val1 != val2 :
                    ft.b == "LSS" ? val1 <  val2 :
                    ft.b == "LEQ" ? val1 <= val2 :
                    ft.b == "GTR" ? val1 >  val2 :
                    ft.b == "GEQ" ? val1 >= val2 :
                    -1);
            assert(val1 == 0 || val1 == 1);
        }
        if (((*m).op == BZ && val1 == 0) ||
            ((*m).op == BNZ && val1 != 0)) {
            MIPS(T << "j" << HT << (*m).a);
        }
        return;
    }

    // no comparision
    if (ft.b == "") { 
        assert(isConstValue(ft.a, val1) == false);
        loadToReg("$v0", ft.a);
        if ((*m).op == BZ) {
            MIPS(T << "beq" << HT << "$v0, $zero, " << (*m).a);
        } else {
            MIPS(T << "bne" << HT << "$v0, $zero, " << (*m).a);
        }
        return;
    }

    assert(ft.b != "" && ft.res != "");
    assert(!(isConstValue(ft.a, val1) && isConstValue(ft.res, val2)));

    std::string operand1 = "$v0";
    if (isConstValue(ft.a, val1) && val1 == 0) {
        // if ft.a is a const zero value, we can 
        // use $zero to reduce a load operation
        operand1 = "$zero";
    } else {
        loadToReg("$v0", ft.a);
    }
    std::string operand2 = "$v1";
    if (isConstValue(ft.res, val2) && val2 == 0) {
        // if ft.res is a const zero value, we can
        // use $zero to reduce a load operation
        operand2 = "$zero";
    } else {
        loadToReg("$v1", ft.res);
    }

    if (ft.b == "EQL" || ft.b == "NEQ") {
        std::string op = ((ft.b == "EQL") ^ ((*m).op == BZ)) ?
            "beq" : "bne";
        MIPS( T << op << HT << operand1 << ", " << operand2
                << ", " << (*m).a);
        return;
    }

    assert(!(operand1 == "$zero" && operand2 == "$zero"));

    // reduce a substract operation, this can be removed freely
    if (operand2 == "$zero") {
        std::string op = (
            ft.b == "LSS" ? ((*m).op == BZ ? "bgez": "bltz"):
            ft.b == "LEQ" ? ((*m).op == BZ ? "bgtz": "blez"):
            ft.b == "GTR" ? ((*m).op == BZ ? "blez": "bgtz"):
            ft.b == "GEQ" ? ((*m).op == BZ ? "bltz": "bgez"):
            "");
        assert(op != "");
        MIPS(T << op << HT << "$v0 ," << (*m).a);
        return;
    }

    // reduce a substract operation, this can be removed freely
    if (operand1 == "$zero") {
        std::string op = (
            ft.b == "LSS" ? ((*m).op == BZ ? "bltz": "bgez"):
            ft.b == "LEQ" ? ((*m).op == BZ ? "blez": "bgtz"):
            ft.b == "GTR" ? ((*m).op == BZ ? "bgtz": "blez"):
            ft.b == "GEQ" ? ((*m).op == BZ ? "bgez": "bltz"):
            "");
        assert(op != "");
        MIPS(T << op << HT << "$v1 ," << (*m).a);
        return;
    }

    MIPS(T << "subu" << HT << "$v0, " << operand1 << ", " << operand2);
    std::string op = (
        ft.b == "LSS" ? ((*m).op == BZ ? "bgez": "bltz"):
        ft.b == "LEQ" ? ((*m).op == BZ ? "bgtz": "blez"):
        ft.b == "GTR" ? ((*m).op == BZ ? "blez": "bgtz"):
        ft.b == "GEQ" ? ((*m).op == BZ ? "bltz": "bgez"):
        "");
    assert(op != "");
    MIPS(T << op << HT << "$v0, " << (*m).a);
}


static void indent()
{
    indent_num += 8;
}
static void unindent()
{
    indent_num -= 8;
    assert(indent_num >= 0);
}
static void indent4()
{
    indent_num += 4;
}
static void unindent4()
{
    indent_num -= 4;
    assert(indent_num >= 0);
}


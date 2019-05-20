#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include <fstream>
#include <unordered_map>
#include <map>
#include "symbol.h"
#include "table.h"
#include "midcode.h"

/**
 * This file hold all global variables
 */

/*
 * initialized at "main.cpp"
 */
extern Symbol           char2sym[128];  // map from single character to symbol
extern std::unordered_map<std::string, Symbol> key2sym; // identifier to symbol
extern std::string      source_filename;    // filename of source code
extern std::ifstream    source_stream;      // source code input stream
extern std::ostream     midcode_stream;     // middle code output stream
extern std::ostream     mipscode_stream;    // mips code output stream
extern std::ostream     opt_midcode_stream; // optimized midddle code output
extern std::ostream     opt_mipscode_stream;// optimized mips code output
extern std::ostream     debug_stream;       // debug


/**
 * initialized at "symbol.cpp"
 */
extern Symbol           g_sym;          // last symbol 
extern std::string      g_id;           // used if g_sym==IDENTSY
extern int              g_num;          // used if g_sym==INTVALUE
extern std::string      g_str;          // used if g_sym==STRVALUE
extern char             g_char;         // used if g_sym==CHARVALUE
/* below are used by error handling */
extern std::string      g_line;         // string of current line
extern unsigned int     g_pos;          // the pos of next character
extern unsigned int     g_line_no;      // the No. of current line
extern unsigned int     g_word_pos;     // the pos of current word



/**
 * initialized at "table.cpp"
 */
extern std::unordered_map<std::string, TabEntry>    g_table;
extern std::unordered_map<std::string, TabEntry>    b_table;
extern std::map<std::string, std::string>           strings_table;

#endif // COMMON_H_

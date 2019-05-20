#include <string>       // string
#include <iostream>     // cout
#include <cctype>       // isalpha(), isdigit(), tolower()
#include <cstdlib>      // exit()
#include <cstdio>       // printf()
#include <sstream>      // stringstream
#include <iomanip>      // setw()
#include "common.h"
#include "symbol.h"
#include "error.h"


/* initialize global variables */
Symbol          g_sym;          // Type of word read in
std::string     g_id;           // used if g_sym==IDENT
char            g_char;         // used if g_sym==CHARVALUE
int             g_num;          // used if g_sym==INTVALUE
std::string     g_str;          // used if g_sym==STRVALUE

std::string     g_line = "";    // last line from source code
unsigned int    g_pos = 0;      // the pos of next character
unsigned int    g_line_no = 0;  // the No. of current line
unsigned int    g_word_pos;     // the start pos of current word

/* file-scope global variable */
static char         ch = ' ';   // last character read in

static bool nextch();
static void readUnsignedInteger();
static void readIdentifier();
static void readString();
static void readCharacter();
static bool isBlankCharacter(char c);


void readSymbol() 
{
goto_label:
    while (isBlankCharacter(ch))
        nextch();
    g_word_pos = g_pos - 1;
    if (std::isdigit(ch)) {
        readUnsignedInteger();
        return;
    }
    if (std::isalpha(ch) || ch == '_') {
        readIdentifier();
        return;
    }
    switch(ch) {
        case '\"': readString(); break;
        case '\'': readCharacter(); break;
        case '>': nextch();
                  if (ch == '=') {
                      g_sym = GEQ;
                      nextch();
                  } else g_sym = GTR;
                  break;
        case '<': nextch();
                  if (ch == '=') {
                      g_sym = LEQ;
                      nextch();
                  } else g_sym = LSS;
                  break;
        case '!': nextch();
                  if (ch != '=') {
                      std::cout << "operater ! is not allowed" 
                           << std::endl;
                      std::exit(1);
                  }
                  g_sym = NEQ;
                  nextch();
                  break;
        case '=': nextch();
                  if (ch == '=') {
                      g_sym = EQL;
                      nextch();
                  } else g_sym = BECOMES;
                  break;
        default: if (char2sym[(int)ch] == 0) {
                     error(ERR_UNSUPPORTED_CHARACTER);
                     nextch();
                     goto goto_label;
                 }
                 g_sym = char2sym[(int)ch];
                 nextch();
    }
}


static bool isBlankCharacter(char c) 
{
    /**
     * 32 is space  ' '
     * 9 is HT(Horizontal Tab) '\t'
     * 10 is NL(Next line) '\n'
     * NOTE: don't use 09 as 09 is octal
     */
    return (c == 32 || c == 9 || c == 10);
}


static void readIdentifier() 
{
    /**
     * Important: identifiers is not case-sensitve.
     */
    g_id.clear();
    while (std::isdigit(ch) || std::isalpha(ch)
            || ch == '_') {
        ch = std::tolower(ch);
        g_id.push_back(ch);
        nextch();
    }
    // map will return 0 if key is not in map,
    // which is exactly what we want, because
    // user-defined identifier's enum value is 0
    g_sym = key2sym[g_id];
}


static void readCharacter() 
{
    /**
     * Valid characters of char are:
     * +, -, *, /, _, alpha, digit
     */
    nextch();
    if (ch != '+' && ch != '-' && ch != '*' && 
            ch != '/' && ch != '_' && 
            !std::isdigit(ch) && !std::isalpha(ch)) {
        std::cout << "Invalid ASCII character in char " 
            << ch << std::endl;
    }
    g_char = ch;
    nextch();
    if (ch != '\'') {
        std::cout << "Single quotation mark missing at char "
            << ch << std::endl;
    }
    g_sym = CHARVALUE;
    nextch();
}


static void readString()
{
    /**
     * Valid ASCII characters of string are:
     * 32(space), 33('!'), 35-126
	 * use `should_skip_line_break` to control whether
	 * nextch() will skip line break characters.
     */
    g_str.clear();
    nextch();
    while (ch != '\"') {
		if (ch == '\n') {
            error(ERR_STRING_NOT_END);
            break;
            // may go to next line and comtinue compiling
		}
        if (!(ch == 32 || ch == 33 ||
              (ch >= 35 && ch <= 126))) {
            error(ERR_STRING_INVALID_CHARACTER);
            nextch();
            continue;
        }
        g_str.push_back(ch);
        //if (ch == '\\')
        //    g_str.push_back('\\');
        nextch();
    }
    g_sym = STRVALUE;
    // next character must be right quotation mark, skip it
    nextch();
}

static void readUnsignedInteger() 
{
    g_num = 0;
    while (ch >= '0' && ch <= '9') {
        g_num = 10 * g_num + ch - '0';
        nextch();
    }
    g_sym = INTVALUE;
}

/**
 * Used to handle different line break like "\r\n" and "\n"
 * From: https://stackoverflow.com/questions/6089231/
 */
static std::istream& safeGetline(std::istream& is, std::string& t)
{
    t.clear();
    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();
    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n':
            return is;
        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;
        case std::streambuf::traits_type::eof():
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;
        default:
            t += (char)c;
        }
    }
}


/**
 * use `check_remaining_flag` to control whether
 * compiler program should exit when no more code
 *
 * return value is used only when check_remaining_flag=true
 */
static bool check_remaining_flag = false;
static bool nextch() 
{
    if (g_pos == g_line.size()) {
        if (!safeGetline(source_stream, g_line)) {
            if (check_remaining_flag) {
                return false;
            }
            error(ERR_PROGRAM_INCOMPLETE);
            printCachedErrors();
            std::exit(1);
        }
        g_line.push_back('\n');
        
        g_pos = 0;
        g_line_no++;

        std::cout << std::setw(4) << (int)g_line_no 
            << " " << g_line;
    } 
    ch = g_line[g_pos];
    g_pos++;
    return true;
}

void extraCodeChecking()
{
    check_remaining_flag = true;
    while (nextch()) {
        if (!isBlankCharacter(ch)) {
            error(ERR_REDUNDENT_CODE);
            printCachedErrors();
            std::exit(1);
        }
    }
}


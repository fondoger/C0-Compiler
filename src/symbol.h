/**
 * This module is lexical analyzer
 * it mainly provides getSymbol() function
 */
#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <string>
#include <bitset>           // bitset
#include <initializer_list> // initializer_list
#include <iostream>
#include <vector>

/**
 * Declaring word type enum
 * Types:
 *      1. user-defined identifiers
 *          * function name
 *          * const name
 *          * variable name
 *      2. reserve words
 *      3. delimeters and operators
 */
enum Symbol {
    /* User-defined identifiers */
    IDENTSY=0,  // identifiers
    /* Reserve Words (or call them key word) */
    CONSTSY,    // const
    INTSY,      // int
    CHARSY,     // char
    VOIDSY,     // void
    IFSY,       // if
    ELSESY,     // else
    DOSY,       // do
    WHILESY,    // while
    SWITCHSY,   // switch
    CASESY,     // case
    DEFAULTSY,  // default
    SCANFSY,    // scanf
    PRINTFSY,   // printf
    RETURNSY,   // return
    MAINSY=15,     // main
    /* Literals */
    CHARVALUE,  // character integer literal
    INTVALUE,   // unsinged integer literal
    STRVALUE,   // string literal
    /* Delimeters or Operators */
    PLUS=19, MINUS, STAR, SLASH,
    EQL=23, NEQ, LSS, LEQ, GTR, GEQ, 
    LPARENT=29, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE,
    BECOMES=35, COMMA, COLON, SEMICOLON=38, 
};


/**
 * This is a custom set class for symbol.
 * Designed for write less code related to error handling.
 * Usage:
 *      // constructor
 *      Symset s1 = { IDENTSY, CONSTY, CHARSY };
 *      Symset s2 = { DOSY, CASESY, WHIlESY };
 *      Symset s3({SEMICOLON, RBRACE, RPARENT});
 *      // add two sets
 *      Symset s4 = s1 + s2;
 *      // check if constains 
 *      s4.contains(IDENTSY);  // true
 */
class Symset {
private:
    // `long long` at least 64 bits, enough for symbols
    long long bitmap;
public:
    Symset():bitmap(0LL) {}
    Symset(std::initializer_list<int> il) {
        bitmap = 0LL;
        for (auto itm : il) {
            if (itm < 0 || itm >= 64) {
                std::cout << "values of symset must between 1 and 64"
                    << std::endl;
                exit(1);
            }
            // (long long) is important
            bitmap |= (long long)1 << itm;
        }
    }
    bool contains(int item) {
        return (bool)(((long long)1 << item) & bitmap);
    }
    Symset operator+(const Symset &t1) {
        Symset res;
        res.bitmap = bitmap | t1.bitmap;
        return res;
    }
    void printBits() const {
        std::bitset<64> t(bitmap);
        std::cout << t << std::endl;
    }
    std::vector<int> toVector() {
        std::vector<int> res;
        for (int i = 0; i < 64; i++) {
            if (contains(i))
                res.push_back(i);
        }
        return res;
    }
};


/** NOTE:
 * If there are no static modifier, we will get 
 * multiple definition error, because each file 
 * who includes this will make a app-scrop global
 * variable. This won't cause error when compiling
 * each single .cpp file, but will cause error when
 * linker trys to link .o files.
 * We have two ways to eliminate this error:
 * 1. add static modifier, so that each .cpp file
 *    holds a file-scope global variable. This is
 *    a litttle bit of wasting memory.
 * 2. add extern modifier and define it once in any
 *    one of .cpp files.
 * */
static std::string tokens[] = {
    "IDENTSY", "CONSTSY", "INTSY", "CHARSY", "VOIDSY", "IFSY", // 0~5
    "ELSESY", "DOSY", "WHILESY", "SWITCHSY", "CASESY",         // 6~10
    "DEFAULTSY", "SCANFSY", "PRINTFSY", "RETURNSY", "MAINSY",  // 11~15
    "CHARVALUE", "INTVALUE", "STRVALUE",                       // 16~18
    "PLUS", "MINUS", "STAR", "SLASH",                          // 19~22
    "EQL", "NEQ", "LSS", "LEQ", "GTR", "GEQ",                  // 23~28
    "LPARENT", "RPARENT", "lBRACK", "RBRACK", "LBRACE", "RBRACE",
    "BECOMES", "COMMA", "COLON", "SEMICOLON",
};
static std::string sym2str[] = {
    "identifier", "const", "int", "char", "void", "if",
    "else", "do", "while", "switch", "case",
    "default", "scanf", "printf", "return", "main",
    "char-literal", "int-literal", "str-literal",
    "\'+\'", "\'-\'", "\'*\'", "\'/\'",
    "\'==\'", "\'!=\'", "\'<\'", "\'<=\'", "\'>\'", "\'>=\'",
    "\'(\'", "\')\'", "\'[\'", "\']\'", "\'{\'", "\'}\'", 
    "\'=\'", "\',\'", "\':\'", "\';\'",
};

void readSymbol();
/**
 * For checking remaining code after main function's definition
 * There should be no more extra non-emtpy characters
 */
void extraCodeChecking();


#endif // SYMBOL_H_

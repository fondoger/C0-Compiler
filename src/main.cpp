#include <iostream>         // cout
#include <cstdio>           // printf
#include <unordered_map>    // unsorted_map
#include <iomanip>          // setw
#include <fstream>          // ifstream
#include "symbol.h"
#include "error.h"
#include "common.h"
#include "grammar.h"
#include "mips.h"



/* initialize global variables */
Symbol          char2sym[128];
std::unordered_map<std::string, Symbol> key2sym;
std::string     source_filename;
std::ifstream   source_stream;
std::ostream    midcode_stream(NULL);
std::ostream    mipscode_stream(NULL);
std::ostream    opt_midcode_stream(NULL);
std::ostream    opt_mipscode_stream(NULL);
std::ostream    debug_stream(NULL);


static void initialize();

int main(int argc, char *argv[]) {
    initialize();

    source_filename = argc > 1 ? argv[1] : "hello_world.txt";
    source_stream.open(source_filename);

    //midcode_stream.rdbuf(std::cout.rdbuf());
    std::string midcode_filename = "mid_code.txt";
    std::string mipscode_filename = "mips_code.txt";
    std::filebuf buffer1, buffer2;
    buffer1.open(midcode_filename, std::ios_base::out);
    buffer2.open(mipscode_filename, std::ios_base::out);
    midcode_stream.rdbuf(&buffer1);
    mipscode_stream.rdbuf(&buffer2);

    // debug messages
    debug_stream.rdbuf(std::cout.rdbuf());

    // Do syntax check and generate mid-code
    pProgram();

    bool has_error = printCachedErrors();
    if (has_error)
        exit(1);
        
    std::cout << "compile success!\n";
    std::cout << "mid code at: " << midcode_filename << std::endl;
    // convert mid-code to MIPS code
    convertToMIPS();
    std::cout << "mips code at: " << mipscode_filename << std::endl;
    std::cout << "\nIf you want to execute this mips program, using following command:\n"
        << "    $ java -jar mars.jar nc mips_code.txt" << std::endl;

    source_stream.close();
    return 0;
}

static void initialize() 
{
    /* Initialize reserve word map */
    key2sym["const"] = CONSTSY;     key2sym["int"] = INTSY;
    key2sym["char"] = CHARSY;       key2sym["void"] = VOIDSY;
    key2sym["if"] = IFSY;           key2sym["else"] = ELSESY;
    key2sym["do"] = DOSY;           key2sym["while"] = WHILESY;
    key2sym["switch"] = SWITCHSY;   key2sym["case"] = CASESY;   
    key2sym["default"] = DEFAULTSY; key2sym["scanf"] = SCANFSY; 
    key2sym["printf"] = PRINTFSY;   key2sym["return"] = RETURNSY; 
    key2sym["main"] = MAINSY;
    /* Initialize single character(special symbols) map */
    char2sym['+'] = PLUS;       char2sym['-'] = MINUS;
    char2sym['*'] = STAR;       char2sym['/'] = SLASH;
    char2sym['<'] = LSS;        char2sym['>'] = GTR;
    char2sym['('] = LPARENT;    char2sym[')'] = RPARENT;
    char2sym['['] = LBRACK;     char2sym[']'] = RBRACK;
    char2sym['{'] = LBRACE;     char2sym['}'] = RBRACE;
    char2sym['='] = BECOMES;    char2sym[','] = COMMA;
    char2sym[':'] = COLON;      char2sym[';'] = SEMICOLON;
}


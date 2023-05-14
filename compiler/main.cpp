#include "apm.h"
#include "errors.h"
#include "json.h"
#include "lexer.h"
#include "parser.h"
#include "resolver.h"
#include "source.h"
#include "token.h"
#include "utilty.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// Output to JSON

void output_program(ptr<Program> program, string file_name)
{
    std::ofstream output;
    output.open("local/" + file_name + ".json");
    if (output.is_open())
    {
        output << to_json(program);
        cout << "Saved APM to local/" + file_name + ".json" << endl;
        output.close();
    }
    else
    {
        cout << "Error attempting to save APM to local/" + file_name + ".json" << endl;
    }
}

// Main

int main(int argc, char *argv[])
{
    // FIXME: Allow for compilation of multiple source files.

    // FIXME: Remove this default value! I only have it for now for ease of testing
    string source_path = (argc == 2)
                             ? (string)argv[1] + ".gambit"
                             : "local/main.gambit";

    Source source(source_path);

    try
    {
        cout << "\nLEXING" << endl;
        Lexer lexer;
        lexer.tokenise(source);

        // for (auto t : tokens)
        //     cout << to_string(t) << endl;
        // cout << endl;

        // for (auto t : tokens)
        //     cout << t.str << " ";
        // cout << endl;

        cout << "\nPARSING" << endl;
        Parser parser;
        auto program = parser.parse(source);
        output_program(program, "parser_output");

        cout << "\nRESOLVER" << endl;
        Resolver resolver;
        resolver.resolve(source, program);
        output_program(program, "resolver_output");
    }
    catch (CompilerError error)
    {
        cout << "\nCOMPILER ERROR: " << endl;
        cout << "(This is an issue with the Gambit compiler, not with your program!)" << endl;
        cout << error.what() << endl;
    }

    cout << "\nERRORS" << endl;
    for (auto error : source.errors)
        cout << present_error(error) << endl;
    cout << endl;

    cout << "Compilation complete" << endl;

    return 0;
}
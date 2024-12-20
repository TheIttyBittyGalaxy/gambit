#include "apm.h"
#include "checker.h"
#include "converter.h"
#include "errors.h"
#include "generator.h"
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

// Output to C

void output_c_source(string source, string file_name)
{
    std::ofstream output;
    output.open("local/" + file_name + ".c");
    if (output.is_open())
    {
        output << source;
        cout << "Saved C source code to local/" + file_name + ".c" << endl;
        output.close();
    }
    else
    {
        cout << "Error attempting to save C source code to local/" + file_name + ".c" << endl;
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

    ptr<Program> program = nullptr;

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
        program = parser.parse(source);
        output_program(program, "parser_output");

        cout << "\nRESOLVER" << endl;
        Resolver resolver;
        resolver.resolve(source, program);
        output_program(program, "resolver_output");

        cout << "\nCHECKER" << endl;
        Checker checker;
        checker.check(source, program);
        output_program(program, "checker_output");

        if (source.errors.size() > 0)
        {
            cout << "\nERRORS" << endl;
            for (auto error : source.errors)
                cout << present_error(&source, error) << endl;
            cout << endl;
        }
        else
        {
            cout << "\nCONVERTER" << endl;
            Converter converter;
            auto representation = converter.convert(program);
            // TODO: Output as JSON

            cout << "\nGENERATOR" << endl;
            Generator generator;
            auto source = generator.generate(representation);
            output_c_source(source, "generated");
        }

        cout << "Compilation complete" << endl;
    }
    catch (CompilerError error)
    {
        cout << "\nCOMPILER ERROR: " << endl;
        cout << "(This is an issue with the Gambit compiler, not with your program!)" << endl;
        cout << error.what() << endl;
    }

    return 0;
}
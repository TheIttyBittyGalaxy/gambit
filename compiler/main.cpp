#include "apm.h"
#include "errors.h"
#include "json.h"
#include "lexing.h"
#include "parser.h"
#include "token.h"
#include "utilty.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// MAIN //

int main(int argc, char *argv[])
{
    // FIXME: Allow the path to the source project to be passed directly, rather than infered.
    //        I've just made it easier this way for now to make testing easier for myself.

    // FIXME: Allow for generation of multiple source files.

    string game = (argc == 2) ? (string)argv[1] : "tic-tac-toe";
    string src_path = "game/" + game + "/main.gambit";

    // File loading

    ifstream src_file;
    src_file.open(src_path, ios::in);
    if (!src_file)
    {
        cout << "Source file " << src_path << " could not be loaded" << endl;
        return 0;
    }

    string src((istreambuf_iterator<char>(src_file)), istreambuf_iterator<char>());
    src_file.close();

    // Compile

    cout << "\nLEXING" << endl;
    auto tokens = generate_tokens(src);

    // for (auto t : tokens)
    //     cout << to_string(t) << endl;
    // cout << endl;

    // for (auto t : tokens)
    //     cout << t.str << " ";
    // cout << endl;

    cout << "\nPARSING" << endl;
    Parser parser;
    auto program = parser.parse(tokens);

    std::ofstream parser_output;
    parser_output.open("local/parser_output.json");
    parser_output << to_json(program);
    cout << "Saved parser output to local/parser_output.json" << endl;

    cout << "\nERRORS" << endl;
    for (auto err : errors)
        cout << err << endl;
    cout << endl;

    return 0;
}
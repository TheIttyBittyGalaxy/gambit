#include "errors.h"

vector<string> gambit_errors;

void emit_gambit_error(string msg, size_t line, size_t column)
{
    gambit_errors.emplace_back("[Error at " + to_string(line) + ":" + to_string(column) + "] " + msg);
}

void emit_gambit_error(string msg, Token token)
{
    emit_gambit_error(msg, token.line, token.column);
}
#include "errors.h"

vector<string> errors;

void emit_error(string msg)
{
    errors.emplace_back("[Error] " + msg);
}

void emit_error(string msg, size_t line, size_t column)
{
    errors.emplace_back("[Error at " + to_string(line) + ":" + to_string(column) + "] " + msg);
}

void emit_error(string msg, Token t)
{
    emit_error(msg, t.line, t.column);
}
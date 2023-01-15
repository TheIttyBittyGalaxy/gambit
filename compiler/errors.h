#ifndef ERRORS_H
#define ERRORS_H

#include "token.h"
#include <string>
#include <vector>
using namespace std;

extern vector<string> errors;

void emit_error(string msg);
void emit_error(string msg, size_t line, size_t column);
void emit_error(string msg, Token t);

#endif
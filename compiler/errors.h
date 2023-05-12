#pragma once
#ifndef ERRORS_H
#define ERRORS_H

#include "token.h"
#include <exception>
#include <optional>
#include <string>
#include <vector>
using namespace std;

extern vector<string> gambit_errors;
void emit_gambit_error(string msg, size_t line, size_t column);
void emit_gambit_error(string msg, Token token);

class GambitError : public exception
{
public:
    string msg;
    Token token;

    GambitError(string msg, Token token) : msg(msg), token(token)
    {
        emit_gambit_error(msg, token);
    };

    string what()
    {
        return msg;
    }
};

// FIXME: Whenever the compiler is at a point where every node has a `span` (something that maps
//        the node to a segment of the source code), replace the `optional<token>` with `span`.
class CompilerError : public exception
{
public:
    string msg;
    optional<Token> token;

    CompilerError(string msg, optional<Token> token = {}) : msg(msg), token(token){};

    string what()
    {
        string err = msg;
        if (token.has_value())
            err += " " + to_string(token.value());
        return err;
    }
};

#endif
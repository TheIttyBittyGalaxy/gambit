#pragma once
#ifndef ERRORS_H
#define ERRORS_H

#include "span.h"
#include "token.h"
#include <exception>
#include <optional>
#include <string>
#include <vector>
using namespace std;

// FIXME: Make the various different ways in which errors are presented more consistent.

extern vector<string> gambit_errors;
void emit_gambit_error(string msg, size_t line, size_t column);
void emit_gambit_error(string msg, Token token);

// FIXME: Move method definitions into error.cpp, in the hopes it will help with compilation time
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

// FIXME: Move method definitions into error.cpp, in the hopes it will help with compilation time
class CompilerError : public exception
{
public:
    string msg;
    optional<Span> span_one;
    optional<Span> span_two;

    CompilerError(string msg, optional<Span> span_one = {}, optional<Span> span_two = {})
        : msg(msg),
          span_one(span_one),
          span_two(span_two){};

    string what()
    {
        string err = msg;

        if (span_one.has_value())
        {
            auto span = span_one.value();
            err += "\n\n";
            if (span.source == nullptr)
                err += "invalid span";
            else
                err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->get_file_path() + (span.multiline ? "\n" : "  ") + span.get_source_substr();
        }

        if (span_two.has_value())
        {
            auto span = span_two.value();
            err += "\n\n";
            if (span.source == nullptr)
                err += "invalid span";
            else
                err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->get_file_path() + (span.multiline ? "\n" : "  ") + span.get_source_substr();
        }

        return err;
    }
};

#endif
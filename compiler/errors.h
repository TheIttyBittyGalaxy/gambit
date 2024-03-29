#pragma once
#ifndef ERRORS_H
#define ERRORS_H

#include "span.h"
#include "token.h"
#include <exception>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
using namespace std;

struct GambitError
{
    string msg;
    vector<Span> spans;

    size_t line;
    size_t column;

    GambitError(string msg, size_t line, size_t column, initializer_list<Span> spans = {})
        : msg(msg),
          line(line),
          column(column),
          spans(spans){};

    GambitError(string msg, Token token)
        : GambitError(msg, token.line, token.column){};

    GambitError(string msg, Span span)
        : GambitError(msg, span.line, span.column, {span}){};

    // FIXME: There will be an error if an empty initializer_list is passed to this.
    //        Either prevent this from being possible or handle it gracefully.
    GambitError(string msg, initializer_list<Span> spans)
        : GambitError(msg, (*(spans.begin())).line, (*(spans.begin())).column, spans){};
};

// FIXME: CompilerError is a hang-over of an hold error handling system. Asses how it is used throughout the
//        codebase, and consider if there is a better alternative? At the very least, it may be more useful
//        to replace `span_one` `span_two` with just some helpful utility to turn tokens, spans, and nodes
//        into error message strings that are actually useful for debugging.
class CompilerError : public exception
{
public:
    string msg;
    optional<Span> span_one;
    optional<Span> span_two;

    CompilerError(string msg, optional<Span> span_one = {}, optional<Span> span_two = {});
    string what();
};

#endif
#pragma once
#ifndef ERRORS_H
#define ERRORS_H

// STABILISE: Forward declaration to allow for a source pointer, without creating a cyclic include dependency.
struct Source;

#include "span.h"
#include "token.h"
#include <exception>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
using namespace std;

// FIXME: Currently how errors are displayed is inconsistent, and sometimes messy.
//        Perhaps it would make more sense to turn these into plain old data structures,
//        and then have the call site decide how to display the errors? That would make
//        it possible to things like, for example, grouping errors from the same source
//        file, instead of printing the file path of each and every error.

// STABILISE: Never throw a gambit error! Doing do breaks assumptions about how the
//            APM nodes are parsed/resolved/whatever, which cascades errors and makes
//            the behaviour of the compiler unpredictable.

struct GambitError
{
    string msg;
    vector<Span> spans;

    Source *source;
    size_t line;
    size_t column;

    GambitError(string msg, Source *source, size_t line, size_t column, initializer_list<Span> spans = {})
        : msg(msg),
          source(source),
          line(line),
          column(column),
          spans(spans){};

    GambitError(string msg, Source *source, Token token)
        : GambitError(msg, source, token.line, token.column){};

    GambitError(string msg, Source *source, Span span)
        : GambitError(msg, source, span.line, span.column, {span}){};

    // FIXME: There will be an error if an empty initializer_list is passed to this.
    //        Either prevent this from being possible or handle it gracefully.
    GambitError(string msg, Source *source, initializer_list<Span> spans)
        : GambitError(msg, source, (*(spans.begin())).line, (*(spans.begin())).column, spans){};
};

string present_error(GambitError error);

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
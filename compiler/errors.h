#pragma once
#ifndef ERRORS_H
#define ERRORS_H

// STABILISE: Forward declaration to allow for a source pointer, without creating a cyclic include dependency.
struct Source;

#include "span.h"
#include "token.h"
#include <exception>
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

class GambitError : public exception
{
public:
    string msg;
    size_t line;
    size_t column;
    Source *source;
    optional<Span> span_one;
    optional<Span> span_two;

    GambitError(string msg, size_t line, size_t column, Source *source);
    GambitError(string msg, Token token, Source *source);
    GambitError(string msg, Span span_one, optional<Span> span_two = {});
    string what();
};

class CompilerError : public exception
{
public:
    string msg;
    optional<Span> span_one;
    optional<Span> span_two;

    CompilerError(string msg, optional<Span> span_one = {}, optional<Span> span_two = {});
    string what();
};

extern vector<GambitError> gambit_errors;

#endif
#pragma once
#ifndef SPAN_H
#define SPAN_H

#include "source.h"
#include "token.h"
#include <string>
using namespace std;

// FIXME: For the purposes of displaying errors, would it make the most sense
//        if multiline spans included the entirely of the lines that they span,
//        instead of only a segment? Or at least have the option to generate this
//        string where it would be useful?

// STABILISE: Remove any possibility of null spans! null pointers cause errors.

struct Span
{
    size_t line;
    size_t column;
    size_t position;
    size_t length;
    bool multiline;
    Source *source;

    Span()
        : line(0),
          column(0),
          position(0),
          length(0),
          multiline(false),
          source(nullptr){};

    Span(size_t line,
         size_t column,
         size_t position,
         size_t length,
         bool multiline,
         Source *source)
        : line(line),
          column(column),
          position(position),
          length(length),
          multiline(multiline),
          source(source){};

    Span(Token t, Source *source)
        : line(t.line),
          column(t.column),
          position(t.position),
          length(t.str.length()),
          multiline(t.kind == Token::Line),
          source(source){};

    Span(Span start, Span end);

    string get_source_substr();
};

#endif
#pragma once
#ifndef SPAN_H
#define SPAN_H

#include "token.h"
#include "source.h"
using namespace std;

struct Span
{
    // FIXME: There's probably little benefit of having every span store it's string.
    //        Probably worth replacing with a `determine_string_from_source` method
    //        that fetches the string only as it's needed?

    string str;
    size_t line;
    size_t column;
    size_t position;
    size_t length;
    const Source *source;

    Span() : str(""),
             line(0),
             column(0),
             position(0),
             length(0),
             source(nullptr){};

    Span(Token t) : str(t.str),
                    line(t.line),
                    column(t.column),
                    position(t.position),
                    length(t.str.length()),
                    source(t.source){};

    Span(Span start, Span end);
    Span(Token start, Span end);
    Span(Span start, Token end);
    Span(Token start, Token end);
};

#endif
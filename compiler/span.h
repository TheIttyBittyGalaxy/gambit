#pragma once
#ifndef SPAN_H
#define SPAN_H

#include <string>

// Forward declaration of source. (Not included to avoid cyclic dependency.)
struct Source;

using namespace std;

// FIXME: For the purposes of displaying errors, would it make the most sense
//        if multiline spans included the entirely of the lines that they span,
//        instead of only a segment? Or at least have the option to generate this
//        string where it would be useful?

struct Span
{
    size_t line;
    size_t column;
    size_t position;
    size_t length;
    bool multiline;
    Source *source;

    // TODO: 'null spans' (spans with source = nullptr) are likely to produce
    //       segmentation faults and other errors if not handled correctly.
    //       Consider if there a viable way of preventing this, or if not,
    //       check access sites to see if there are any potential errors.
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

    string get_source_substr();
};

Span merge(Span start, Span end);

#endif
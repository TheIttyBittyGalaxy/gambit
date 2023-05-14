#include "span.h"
#include "errors.h"

Span::Span(Span start, Span end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources", start, end);

    line = start.line;
    column = start.column;
    position = start.position;
    length = (end.position + end.length) - start.position;
    multiline = start.multiline || end.multiline || start.line != end.line;
    source = start.source;
}

string Span::get_source_substr()
{
    return source->substr(position, length);
}
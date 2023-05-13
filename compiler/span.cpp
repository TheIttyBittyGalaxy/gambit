#include "span.h"
#include "errors.h"

Span::Span(Span start, Span end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources");

    line = start.line;
    column = start.column;
    position = start.position;
    length = (end.position + end.length) - start.position;
    source = start.source;
    str = source->substr(position, length);
}

Span::Span(Token start, Span end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources");

    line = start.line;
    column = start.column;
    position = start.position;
    length = (end.position + end.length) - start.position;
    source = start.source;
    str = source->substr(position, length);
}

Span::Span(Span start, Token end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources");

    line = start.line;
    column = start.column;
    position = start.position;
    length = (end.position + end.str.length()) - start.position;
    source = start.source;
    str = source->substr(position, length);
}

Span::Span(Token start, Token end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources");

    line = start.line;
    column = start.column;
    position = start.position;
    length = (end.position + end.str.length()) - start.position;
    source = start.source;
    str = source->substr(position, length);
}
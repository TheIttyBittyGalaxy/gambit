#include "errors.h"
#include "source.h"
#include "span.h"

string Span::get_source_substr()
{
    if (source == nullptr)
        return "[invalid span]";
    return source->substr(position, length);
}

Span merge(Span start, Span end)
{
    if (start.line > end.line || start.line == end.line && start.column > end.column)
        start, end = end, start;

    if (start.source == nullptr || end.source == nullptr)
        throw CompilerError("Attempt to merge null span");

    if (start.source != end.source)
        throw CompilerError("Attempt to merge spans from different sources", start, end);

    return Span(
        start.line,
        start.column,
        start.position,
        (end.position + end.length) - start.position,
        start.multiline || end.multiline || start.line != end.line,
        start.source);
}

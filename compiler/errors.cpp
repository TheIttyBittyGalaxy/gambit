#include "errors.h"
#include "source.h"

CompilerError::CompilerError(string msg, optional<Span> span_one, optional<Span> span_two)
    : msg(msg),
      span_one(span_one),
      span_two(span_two){};

string CompilerError::what()
{
    string err = msg;

    if (span_one.has_value())
    {
        auto span = span_one.value();
        err += "\n\n";
        if (span.source == nullptr)
            err += "[invalid span]";
        else
            err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->file_path + (span.multiline ? "\n" : "  ") + span.get_source_substr();
    }

    if (span_two.has_value())
    {
        auto span = span_two.value();
        err += "\n\n";
        if (span.source == nullptr)
            err += "[invalid span]";
        else
            err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->file_path + (span.multiline ? "\n" : "  ") + span.get_source_substr();
    }

    return err;
}

#include "errors.h"
#include "source.h"

string present_error(GambitError error)
{
    string str = "[" + to_string(error.line) + ":" + to_string(error.column) + "] " + error.msg;

    if (error.spans.size() == 0)
        return str;

    bool error_spans_multiple_sources = false;
    for (auto span : error.spans)
    {
        if (span.source != error.source)
        {
            error_spans_multiple_sources = true;
            break;
        }
    }

    for (auto span : error.spans)
    {
        str += "\n\n";

        Source *source = span.source;
        if (source == nullptr)
        {
            str += "[invalid span]";
            continue;
        }

        if (error_spans_multiple_sources)
            str += source->file_path + "  " + to_string(span.line) + ":" + to_string(span.column) + "\n";

        str += span.get_source_substr();
    }

    str += "\n";

    return str;
}

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

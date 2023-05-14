#include "errors.h"

GambitError::GambitError(string msg, size_t line, size_t column, Source *source)
    : msg(msg),
      line(line),
      column(column),
      source(source)
{
    gambit_errors.push_back(*this);
};

GambitError::GambitError(string msg, Token token) : GambitError(msg, token.line, token.column, token.source){};

GambitError::GambitError(string msg, Span span_one, optional<Span> span_two)
    : msg(msg),
      line(span_one.line),
      column(span_one.column),
      span_one(span_one),
      span_two(span_two),
      source(span_one.source)
{
    gambit_errors.push_back(*this);
};

string GambitError::what()
{
    string err;

    if (span_one.has_value())
    {
        auto span = span_one.value();
        if (span.source == nullptr)
            err += "[invalid span] ";
        else
            err += "[" +
                   span.source->get_file_path() + " " +
                   to_string(span.line) + ":" + to_string(span.column) +
                   "] ";
    }
    else if (source == nullptr)
    {
        err += "[invalid span] ";
    }
    else
    {
        err += "[" +
               source->get_file_path() + " " +
               to_string(line) + ":" + to_string(column) +
               "] ";
    }

    err += msg;

    if (span_one.has_value())
    {
        auto span = span_one.value();
        err += "\n\n";
        if (span.source == nullptr)
        {
            err += "[invalid span] ";
        }
        else
        {
            if (span_two.has_value())
                err += span.source->get_file_path() + " " + to_string(span.line) + ":" + to_string(span.column) + "\n";
            err += span.get_source_substr();
        }
    }

    if (span_two.has_value())
    {
        auto span = span_two.value();
        err += "\n\n";
        if (span.source == nullptr)
        {
            err += "[invalid span] ";
        }
        else
        {
            err += span.source->get_file_path() + " " + to_string(span.line) + ":" + to_string(span.column) + "\n";
            err += span.get_source_substr();
        }
    }

    if (span_one.has_value() || span_two.has_value())
        err += "\n";

    return err;
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
            err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->get_file_path() + (span.multiline ? "\n" : "  ") + span.get_source_substr();
    }

    if (span_two.has_value())
    {
        auto span = span_two.value();
        err += "\n\n";
        if (span.source == nullptr)
            err += "[invalid span]";
        else
            err += to_string(span.line) + ":" + to_string(span.column) + "  " + span.source->get_file_path() + (span.multiline ? "\n" : "  ") + span.get_source_substr();
    }

    return err;
}

vector<GambitError> gambit_errors;
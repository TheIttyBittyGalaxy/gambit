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
    string err = "[" + source->get_file_path() + " " + to_string(line) + ":" + to_string(column) + "] " + msg;

    if (span_one.has_value() && span_two.has_value())
    {
        auto span_one_value = span_one.value();
        auto span_two_value = span_two.value();

        if (span_one_value.source == span_two_value.source)
        {
            return "[" + source->get_file_path() + "] " +
                   msg +

                   "\n\n" +
                   to_string(span_one_value.line) + ":" + to_string(span_one_value.column) +
                   span_one_value.get_source_substr() +

                   "\n\n" +
                   to_string(span_two_value.line) + ":" + to_string(span_two_value.column) +
                   span_two_value.get_source_substr()

                   + "\n";
        }

        return "[" + source->get_file_path() + "] " +
               msg +

               "\n\n" +
               span_one_value.source->get_file_path() + " " +
               to_string(span_one_value.line) + ":" + to_string(span_one_value.column) +
               span_one_value.get_source_substr() +

               "\n\n" +
               span_two_value.source->get_file_path() + " " +
               to_string(span_two_value.line) + ":" + to_string(span_two_value.column) +
               span_two_value.get_source_substr()

               + "\n";
    }
    else if (span_one.has_value())
    {
        auto span = span_one.value();
        return "[" +
               source->get_file_path() + " " +
               to_string(line) + ":" + to_string(column) +
               "] " +

               msg + "\n\n" +
               span.get_source_substr()

               + "\n";
    }

    return "[" +
           source->get_file_path() + " " +
           to_string(line) + ":" + to_string(column) +
           "] " +
           msg;
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
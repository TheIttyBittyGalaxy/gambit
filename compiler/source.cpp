#include "source.h"
#include <fstream>

Source::Source(string file_path)
{
    this->file_path = file_path;
    ifstream src_file;
    src_file.open(file_path, ios::in);
    if (!src_file)
        throw CompilerError("Source file " + file_path + " could not be loaded");

    content = string((istreambuf_iterator<char>(src_file)), istreambuf_iterator<char>());
    src_file.close();

    length = content.length();
}

string Source::substr(size_t position)
{
    return content.substr(position);
}

string Source::substr(size_t position, size_t n)
{
    return content.substr(position, n);
}

void Source::log_error(string msg, size_t line, size_t column, initializer_list<Span> spans)
{
    errors.emplace_back(msg, line, column, spans);
}

void Source::log_error(string msg, Token token)
{
    errors.emplace_back(msg, token);
}

void Source::log_error(string msg, Span span)
{
    errors.emplace_back(msg, span);
}

void Source::log_error(string msg, initializer_list<Span> spans)
{
    errors.emplace_back(msg, spans);
}

string present_error(Source *original_source, GambitError error)
{
    string str = "[" + to_string(error.line) + ":" + to_string(error.column) + "] " + error.msg;

    if (error.spans.size() == 0)
        return str;

    bool error_spans_multiple_sources = false;
    for (auto span : error.spans)
    {
        if (span.source != original_source)
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
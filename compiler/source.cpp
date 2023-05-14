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
    errors.emplace_back(msg, this, line, column, spans);
}

void Source::log_error(string msg, Token token)
{
    errors.emplace_back(msg, this, token);
}

void Source::log_error(string msg, Span span)
{
    errors.emplace_back(msg, this, span);
}

void Source::log_error(string msg, initializer_list<Span> spans)
{
    errors.emplace_back(msg, this, spans);
}

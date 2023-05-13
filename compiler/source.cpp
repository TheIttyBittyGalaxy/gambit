#include "errors.h"
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

string Source::get_file_path() const
{
    return file_path;
}

string Source::get_content() const
{
    return content;
}

size_t Source::get_length() const
{
    return length;
}

string Source::substr(size_t position) const
{
    return content.substr(position);
}

string Source::substr(size_t position, size_t n) const
{
    return content.substr(position, n);
}

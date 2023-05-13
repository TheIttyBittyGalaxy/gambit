#pragma once
#ifndef SOURCE_H
#define SOURCE_H

#include <string>
using namespace std;

struct Source
{
private:
    string file_path;
    string content;
    size_t length;

public:
    Source(string file_path);

    string get_file_path() const;
    string get_content() const;
    size_t get_length() const;

    string substr(size_t position) const;
    string substr(size_t position, size_t n) const;
};

#endif

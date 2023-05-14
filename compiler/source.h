#pragma once
#ifndef SOURCE_H
#define SOURCE_H

#include <string>
using namespace std;

// FIXME: No other data structure in the compiler is implemented in this 'read-only' kind of way.
//        Perhaps it would be clearer and more direct to just make it a plain-old-data structure?
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

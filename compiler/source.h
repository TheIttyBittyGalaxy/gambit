#pragma once
#ifndef SOURCE_H
#define SOURCE_H

#include "errors.h"
#include "span.h"
#include "token.h"
#include <initializer_list>
#include <string>
#include <vector>
using namespace std;

struct Source
{
    string file_path;
    string content;
    size_t length;
    vector<Token> tokens;
    vector<GambitError> errors;

    Source(string file_path);

    string substr(size_t position);
    string substr(size_t position, size_t n);

    void log_error(string msg, size_t line, size_t column, initializer_list<Span> spans = {});
    void log_error(string msg, Token token);
    void log_error(string msg, Span span);
    void log_error(string msg, initializer_list<Span> spans);
};

string present_error(Source *original_source, GambitError error);

#endif

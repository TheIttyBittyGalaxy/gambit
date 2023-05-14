#pragma once
#ifndef SOURCE_H
#define SOURCE_H

#include "token.h"
#include "errors.h"
#include <vector>
#include <string>
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
};

#endif

#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "source.h"
using namespace std;

class Lexer
{
public:
    void tokenise(Source &source);
};

#endif
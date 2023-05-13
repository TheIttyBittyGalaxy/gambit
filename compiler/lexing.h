#pragma once
#ifndef LEXING_H
#define LEXING_H

#include "source.h"
#include "token.h"
#include <vector>
using namespace std;

vector<Token> generate_tokens(const Source &src);

#endif
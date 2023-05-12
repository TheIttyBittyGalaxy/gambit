#pragma once
#ifndef LEXING_H
#define LEXING_H

#include "token.h"
#include <string>
#include <vector>
using namespace std;

vector<Token> generate_tokens(string src);

#endif
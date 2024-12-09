#pragma once
#ifndef GENERATOR_H
#define GENERATOR_H

#include "ir.h"
#include <string>
using namespace std;

class Generator
{
public:
    string generate(C_Program representation);

private:
    string source;
    void write(string token);

    void generate_program(C_Program program);
    void generate_function_signature(C_Function funct);
    void generate_function_declaration(C_Function funct);
};

#endif

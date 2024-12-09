#pragma once
#ifndef GENERATOR_H
#define GENERATOR_H

#include "apm.h"
using namespace std;

class Generator
{
public:
    string generate(ptr<Program> program);

private:
    ptr<Program> program = nullptr;
    string source;

    void write(string token);

    void generate_program(ptr<Program> program);

    void generate_procedure_signature(ptr<Procedure> procedure);
    void generate_function_property_signature(ptr<FunctionProperty> function_property);

    void generate_code_block(ptr<CodeBlock> code_block);
    void generate_statement(Statement stmt);
    void generate_expression(Expression expr);

    void write_pattern_as_c_type(Pattern pattern);
};

#endif
#pragma once
#ifndef CONVERTER_H
#define CONVERTER_H

#include <unordered_set>
#include "apm.h"
#include "ir.h"
using namespace std;

class Converter
{
public:
    C_Program convert(ptr<Program> program);

private:
    ptr<Program> program = nullptr;
    C_Program ir;

    unordered_set<string> identities_used;
    string create_identity(string identity);

    void convert_procedure(ptr<Procedure> procedure);

    size_t create_statement(C_Statement::Kind kind);
    size_t convert_statement(Statement statement);
    size_t convert_expression(Expression expression);
};

#endif
/*
ir.h

Defines the nodes of the Intermediate Representation as well as a range of utility methods.
This should model a C program, so that we can convert to the Abstract Program Model to the IR,
then from the IR to source code.
*/

#pragma once
#ifndef IR_H
#define IR_H

#include <string>
#include <vector>
using namespace std;

// FORWARD DECLARATIONS

// Program
struct C_Program;
struct C_Function;

// Statements
struct C_Statement;

// PROGRAM

struct C_Program
{
    vector<C_Function> functions;
    vector<C_Statement> statements;
};

struct C_Function
{
    string identity;
    size_t body;
};

// STATEMENTS

struct C_Statement
{
    enum Kind
    {
        IF_STATEMENT,
        FOR_LOOP,
        WHILE_LOOP,
        RETURN_STATEMENT,
        VARIABLE_DECLARATION,

        CODE_BLOCK,
        EXPRESSION_STATEMENT
    };

    Kind kind;
    union
    {
        struct
        {
            size_t statement_count;
        };
    };
};

#endif
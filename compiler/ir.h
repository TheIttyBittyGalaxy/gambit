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

// Expressions
struct C_Expression;

// PROGRAM

struct C_Program
{
    vector<C_Function> functions;
    vector<C_Statement> statements;
    vector<C_Expression> expressions;
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
        INVALID,

        IF_STATEMENT,
        ELSE_IF_STATEMENT,
        ELSE_STATEMENT,
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
        struct
        {
            size_t expression;
        };
    };
};

// EXPRESSIONS

struct C_Expression
{
    enum Kind
    {
        INVALID,

        DOUBLE_LITERAL,
        INT_LITERAL,
        BOOL_LITERAL,
        STRING_LITERAL,

        BINARY_ADD,
        BINARY_SUB,
        BINARY_MUL,
        BINARY_DIV,
        BINARY_EQUAL,
        BINARY_AND,
        BINARY_OR,
    };

    Kind kind;
    union
    {
        struct
        {
            size_t lhs;
            size_t rhs;
        };
        struct
        {
            double double_value;
        };
        struct
        {
            int int_value;
        };
        struct
        {
            bool bool_value;
        };
    };

    string string_value; // TODO: Ideally this would go in the union, but that takes some effort to do!
};

#endif
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

// PROGRAM

struct C_Program
{
    vector<C_Function> functions;
};

struct C_Function
{
    string identity;
};

#endif
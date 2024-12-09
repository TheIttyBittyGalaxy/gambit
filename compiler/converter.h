#pragma once
#ifndef CONVERTER_H
#define CONVERTER_H

#include "apm.h"
#include "ir.h"
using namespace std;

class Converter
{
public:
    C_Program convert(ptr<Program> program);

private:
    ptr<Program> program = nullptr;
    C_Program representation;

    void convert_procedure(ptr<Procedure> procedure);
};

#endif
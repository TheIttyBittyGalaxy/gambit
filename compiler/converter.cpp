#include "converter.h"

C_Program Converter::convert(ptr<Program> program)
{
    for (auto entry : program->global_scope->lookup)
    {
        auto value = entry.second;
        if (IS_PTR(value, Procedure))
            convert_procedure(AS_PTR(value, Procedure));
    }

    return representation;
}

void Converter::convert_procedure(ptr<Procedure> procedure)
{
    C_Function funct;
    funct.identity = "_proc_" + procedure->identity;

    representation.functions.push_back(funct);
}
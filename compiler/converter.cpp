#include "converter.h"

C_Program Converter::convert(ptr<Program> program)
{
    // Reserve identities that will be used in the C program
    identities_used.insert("GambitEntity");
    identities_used.insert("main");

    // Convert everything in global scope
    for (auto entry : program->global_scope->lookup)
    {
        auto value = entry.second;
        if (IS_PTR(value, Procedure))
            convert_procedure(AS_PTR(value, Procedure));
    }

    return representation;
}

string Converter::create_identity(string identity)
{
    if (identities_used.find(identity) == identities_used.end())
    {
        identities_used.insert(identity);
        return identity;
    }

    size_t identity_counter = 1;
    string new_identity;
    do
    {
        new_identity = identity + "_" + to_string(identity_counter++);
    } while (identities_used.find(new_identity) != identities_used.end());

    return new_identity;
}

void Converter::convert_procedure(ptr<Procedure> procedure)
{
    C_Function funct;
    funct.identity = create_identity(procedure->identity);

    representation.functions.push_back(funct);
}
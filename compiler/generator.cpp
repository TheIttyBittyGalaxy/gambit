#include "generator.h"

string Generator::generate(C_Program representation)
{
    generate_program(representation);
    return source;
}

void Generator::write(string token)
{
    source += token + " ";
}

void Generator::generate_program(C_Program program)
{
    // Includes
    write("#include <cstddef>\n");
    write("#include <stdbool.h>\n");
    write("#include <string>\n");

    // Gambit types
    write("#define GambitEntity int\n");

    // Function forward declarations
    for (auto funct : program.functions)
    {
        generate_function_signature(funct);
        write(";");
    }

    // Function declarations
    for (auto funct : program.functions)
    {
        generate_function_declaration(funct);
    }
}
void Generator::generate_function_signature(C_Function funct)
{
    write("void"); // TODO: Return type
    write(funct.identity);
    write("()"); // TODO: Parameters
}

void Generator::generate_function_declaration(C_Function funct)
{
    generate_function_signature(funct);
    write("{}"); // TODO: Generate body
}

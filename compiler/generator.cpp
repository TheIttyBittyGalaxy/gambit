#include "errors.h"
#include "generator.h"

string Generator::generate(C_Program representation)
{
    ir = representation;
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

    size_t first_stmt = funct.body + 1;
    auto block = ir.statements[funct.body];
    size_t last_stmt = funct.body + block.statement_count;

    vector<size_t> close_block_after;
    close_block_after.push_back(last_stmt);

    write("{");
    for (size_t i = first_stmt; i <= last_stmt; i++)
    {
        C_Statement stmt = ir.statements[i];
        switch (stmt.kind)
        {
        case C_Statement::IF_STATEMENT:
        {
            // TODO: Implement
            write("if ()");
            break;
        }

        case C_Statement::FOR_LOOP:
        {
            // TODO: Implement
            write("for ()");
            break;
        }

        case C_Statement::WHILE_LOOP:
        {
            // TODO: Implement
            write("while ()");
            break;
        }

        case C_Statement::RETURN_STATEMENT:
        {
            // TODO: Implement
            write("return;");
            break;
        }

        case C_Statement::VARIABLE_DECLARATION:
        {
            // TODO: Implement
            write("int _ = 0;");
            break;
        }

        case C_Statement::CODE_BLOCK:
        {
            if (stmt.statement_count > 1)
            {
                write("{");
                close_block_after.push_back(i + stmt.statement_count);
            }
            break;
        }

        case C_Statement::EXPRESSION_STATEMENT:
        {
            // TODO: Implement
            write("//epxr\n");
            break;
        }

        default:
            throw CompilerError("Could not generate C_Statement");
        }

        if (close_block_after.back() == i)
        {
            close_block_after.pop_back();
            write("}");
        }
    }
}

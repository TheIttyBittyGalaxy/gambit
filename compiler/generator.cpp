#include "json.h"
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
    // FIXME: This will cause a segmentation fault if ever `last_stmt` is 0
    close_block_after.push_back(last_stmt);

    write("{");
    for (size_t i = first_stmt; i <= last_stmt; i++)
    {
        C_Statement stmt = ir.statements[i];
        switch (stmt.kind)
        {
        case C_Statement::INVALID:
            throw CompilerError("Attempt to generate Invalid C Statement");

        case C_Statement::IF_STATEMENT:
        {
            write("if (");
            generate_expression(stmt.expression);
            write(")");
            break;
        }

        case C_Statement::ELSE_IF_STATEMENT:
        {
            write("else if (");
            generate_expression(stmt.expression);
            write(")");
            break;
        }

        case C_Statement::ELSE_STATEMENT:
        {
            write("else");
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
            write("while (true)");
            break;
        }

        case C_Statement::RETURN_STATEMENT:
        {
            write("return");
            generate_expression(stmt.expression);
            write(";");
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
            if (stmt.statement_count != 1)
            {
                write("{");
                close_block_after.push_back(i + stmt.statement_count);
            }
            break;
        }

        case C_Statement::EXPRESSION_STATEMENT:
        {
            generate_expression(stmt.expression);
            write(";");
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

void Generator::generate_expression(size_t expression_index)
{
    auto &expr = ir.expressions.at(expression_index);

    switch (expr.kind)
    {

    case C_Expression::INVALID:
    {
        // TODO: This should be an error
        write("__NULL_EXPR__");
        break;
    }

    case C_Expression::DOUBLE_LITERAL:
    {
        write(to_string(expr.double_value));
        break;
    }
    case C_Expression::INT_LITERAL:
    {
        write(to_string(expr.int_value));
        break;
    }
    case C_Expression::BOOL_LITERAL:
    {
        write(expr.bool_value ? "true" : "false");
        break;
    }
    case C_Expression::STRING_LITERAL:
    {
        // TODO: Use a dedicated string serialisation function, rather than using the JSON one
        write(to_json(expr.string_value));
        break;
    }
    case C_Expression::BINARY_ADD:
    {
        generate_expression(expr.lhs);
        write("+");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_SUB:
    {
        generate_expression(expr.lhs);
        write("-");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_MUL:
    {
        generate_expression(expr.lhs);
        write("*");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_DIV:
    {
        generate_expression(expr.lhs);
        write("/");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_EQUAL:
    {
        generate_expression(expr.lhs);
        write("==");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_AND:
    {
        generate_expression(expr.lhs);
        write("&&");
        generate_expression(expr.rhs);
        break;
    }
    case C_Expression::BINARY_OR:
    {
        generate_expression(expr.lhs);
        write("||");
        generate_expression(expr.rhs);
        break;
    }

    default:
        throw CompilerError("Could not generate C_Expression " + to_string(expr.kind));
    }
}
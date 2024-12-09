/*
#include <string>
#include "generator.h"

string Generator::generate(ptr<Program> program)
{
    generate_program(program);
    return source;
}

void Generator::write(string token)
{
    source += token + " ";
}

void Generator::generate_program(ptr<Program> program)
{
    // Includes
    write("#include <cstddef>\n");
    write("#include <stdbool.h>\n");
    write("#include <string>\n");

    // Gambit types
    write("#define GambitEntity int\n");

    // Procedure & function forward declarations
    for (auto index : program->global_scope->lookup)
    {
        auto value = index.second;
        if (IS_PTR(value, Procedure))
        {
            generate_procedure_signature(AS_PTR(value, Procedure));
            write(";");
        }
        else if (IS_PTR(value, FunctionProperty))
        {
            generate_function_property_signature(AS_PTR(value, FunctionProperty));
            write(";");
        }
        else if (IS_PTR(value, Scope::OverloadedIdentity))
        {
            auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
            for (auto overload : overloaded_identity->overloads)
            {
                if (IS_PTR(overload, Procedure))
                {
                    generate_procedure_signature(AS_PTR(overload, Procedure));
                    write(";");
                }
                else if (IS_PTR(overload, FunctionProperty))
                {
                    generate_function_property_signature(AS_PTR(overload, FunctionProperty));
                    write(";");
                }
            }
        }
    }

    // Procedure & function declarations
    for (auto index : program->global_scope->lookup)
    {
        auto value = index.second;
        if (IS_PTR(value, Procedure))
        {
            auto procedure = AS_PTR(value, Procedure);
            generate_procedure_signature(procedure);
            generate_code_block(procedure->body);
        }
        else if (IS_PTR(value, FunctionProperty))
        {
            auto function_property = AS_PTR(value, FunctionProperty);
            if (function_property->body.has_value())
            {
                generate_function_property_signature(function_property);
                generate_code_block(function_property->body.value());
            }
        }
        else if (IS_PTR(value, Scope::OverloadedIdentity))
        {
            auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
            for (auto overload : overloaded_identity->overloads)
            {
                if (IS_PTR(overload, Procedure))
                {
                    auto procedure = AS_PTR(overload, Procedure);
                    generate_procedure_signature(procedure);
                    generate_code_block(procedure->body);
                }
                else if (IS_PTR(overload, FunctionProperty))
                {
                    auto function_property = AS_PTR(overload, FunctionProperty);
                    if (function_property->body.has_value())
                    {
                        generate_function_property_signature(function_property);
                        generate_code_block(function_property->body.value());
                    }
                }
            }
        }
    }
}

void Generator::generate_procedure_signature(ptr<Procedure> procedure)
{
    write("void"); // TODO: Correct return type
    write(procedure->identity);
    write("(");

    for (size_t i = 0; i < procedure->parameters.size(); i++)
    {
        auto param = procedure->parameters[i];
        if (i > 0)
            write(",");

        write_pattern_as_c_type(param->pattern);
        write(param->identity);
    }

    write(")");
}

void Generator::generate_function_property_signature(ptr<FunctionProperty> function_property)
{
    write_pattern_as_c_type(function_property->pattern);
    write(function_property->identity);
    write("(");

    for (size_t i = 0; i < function_property->parameters.size(); i++)
    {
        auto param = function_property->parameters[i];
        if (i > 0)
            write(",");

        write_pattern_as_c_type(param->pattern);
        write(param->identity);
    }

    write(")");
}

void Generator::generate_code_block(ptr<CodeBlock> code_block)
{
    write("{");
    for (auto stmt : code_block->statements)
        generate_statement(stmt);
    write("}");
}

void Generator::generate_statement(Statement stmt)
{
    if (IS_PTR(stmt, IfStatement))
    {
        auto if_stmt = AS_PTR(stmt, IfStatement);
        for (size_t i = 0; i < if_stmt->rules.size(); i++)
        {
            auto rule = if_stmt->rules.at(i);
            write(i == 0 ? "if" : "if else");
            write("(");
            generate_expression(rule.condition);
            write(")");
            generate_code_block(rule.code_block);
        }

        if (if_stmt->else_block.has_value())
        {
            write("else");
            generate_code_block(if_stmt->else_block.value());
        }
    }

    else if (IS_PTR(stmt, ForStatement))
    {
        auto for_stmt = AS_PTR(stmt, ForStatement);
        write("for () // TODO: Implement loop condition\n");
        generate_code_block(for_stmt->body);
    }

    else if (IS_PTR(stmt, LoopStatement))
    {
        auto loop_stmt = AS_PTR(stmt, LoopStatement);
        write("while (true)");
        generate_code_block(loop_stmt->body);
    }

    else if (IS_PTR(stmt, ReturnStatement))
    {
        auto return_stmt = AS_PTR(stmt, ReturnStatement);
        write("return");
        generate_expression(return_stmt->value);
        write(";");
    }

    else if (IS_PTR(stmt, WinsStatement))
    {
        write("// TODO: Implement WinsStatement generation\n");
    }

    else if (IS_PTR(stmt, DrawStatement))
    {
        write("// TODO: Implement DrawStatement generation\n");
    }

    else if (IS_PTR(stmt, AssignmentStatement))
    {
        auto assignment_stmt = AS_PTR(stmt, AssignmentStatement);
        generate_expression(assignment_stmt->subject);
        write("=");
        generate_expression(assignment_stmt->value);
        write(";");
    }

    else if (IS_PTR(stmt, VariableDeclaration))
    {
        auto variable_declaration = AS_PTR(stmt, VariableDeclaration);
        auto variable = variable_declaration->variable;
        write_pattern_as_c_type(variable->pattern);
        write(variable->identity);
        if (variable_declaration->value.has_value())
        {
            write("=");
            generate_expression(variable_declaration->value.value());
        }
        write(";");
    }

    else if (IS_PTR(stmt, CodeBlock))
    {
        auto code_block = AS_PTR(stmt, CodeBlock);
        generate_code_block(code_block);
    }

    else if (IS(stmt, Expression))
    {
        generate_expression(AS(stmt, Expression));
        write(";");
    }

    else
    {
        // FIXME: This should be a compiler error
        write("// UNABLE TO GENERATE STATEMENT\n");
    }
}

void Generator::generate_expression(Expression expr)
{

    if (IS(expr, UnresolvedLiteral))
    {
        // FIXME: This should be a compiler error
        write("// UNABLE TO GENERATE EXPRESSION\n");
    }
    else if (IS_PTR(expr, ExpressionLiteral))
    {
        generate_expression(AS_PTR(expr, ExpressionLiteral)->expr);
    }

    else if (IS_PTR(expr, PrimitiveValue))
    {
        auto primitive = AS_PTR(expr, PrimitiveValue);
        if (IS(primitive->value, double))
            write(to_string(AS(primitive->value, double)));
        if (IS(primitive->value, int))
            write(to_string(AS(primitive->value, int)));
        if (IS(primitive->value, bool))
            write(AS(primitive->value, bool) ? "true" : "false");
        if (IS(primitive->value, string))
            write(AS(primitive->value, string));
    }
    else if (IS_PTR(expr, ListValue))
    {
        auto list_value = AS_PTR(expr, ListValue);

        write("{");
        if (list_value->values.size() > 0)
        {
            generate_expression(list_value->values[0]);
            for (size_t i = 1; i < list_value->values.size(); i++)
            {
                write(",");
                generate_expression(list_value->values[i]);
            }
        }
        write("}");
    }
    else if (IS_PTR(expr, EnumValue))
    {
        write("EnumValue"); // TODO: Implement
    }
    else if (IS_PTR(expr, Variable))
    {
        auto variable = AS_PTR(expr, Variable);
        write(variable->identity);
    }

    else if (IS_PTR(expr, Unary))
    {
        auto unary = AS_PTR(expr, Unary);
        write(unary->op);
        generate_expression(unary->value);
    }
    else if (IS_PTR(expr, Binary))
    {
        auto binary = AS_PTR(expr, Binary);
        generate_expression(binary->lhs);
        if (binary->op == "and")
            write("&&");
        else if (binary->op == "or")
            write("||");
        else
            write(binary->op);
        generate_expression(binary->rhs);
    }

    else if (IS_PTR(expr, InstanceList))
    {
        auto instance_list = AS_PTR(expr, InstanceList);
        write("(");
        generate_expression(instance_list->values[0]);
        for (size_t i = 1; i < instance_list->values.size(); i++)
        {
            write(",");
            generate_expression(instance_list->values[i]);
        }
        write(")");
    }
    else if (IS_PTR(expr, IndexWithExpression))
    {
        auto expression_index = AS_PTR(expr, IndexWithExpression);
        generate_expression(expression_index->subject);
        write("[(");
        generate_expression(expression_index->index);
        write(")-1]");
    }
    else if (IS_PTR(expr, IndexWithIdentity))
    {
        auto property_index = AS_PTR(expr, IndexWithIdentity);
        auto property = property_index->property;
        if (IS_PTR(property, FunctionProperty))
        {
            auto function_property = AS_PTR(property, FunctionProperty);
            write(function_property->identity);
            generate_expression(property_index->expr);
        }
        else
        {
            auto state_property = AS_PTR(property, StateProperty);
            write(state_property->identity);
            generate_expression(property_index->expr);
        }
    }

    else if (IS_PTR(expr, Call))
    {
        write("Call"); // TODO: Implement
    }

    else if (IS_PTR(expr, ChooseExpression))
    {
        auto choose_expression = AS_PTR(expr, ChooseExpression);
        write("choose");
        write("(");
        generate_expression(choose_expression->player);
        write(",");
        generate_expression(choose_expression->choices);
        write(",");
        generate_expression(choose_expression->prompt);
        write(")");
    }

    else if (IS_PTR(expr, IfExpression))
    {
        write("IfExpression"); // TODO: Implement
    }
    else if (IS_PTR(expr, MatchExpression))
    {
        write("MatchExpression"); // TODO: Implement
    }

    else if (IS_PTR(expr, InvalidExpression))
    {
        // FIXME: This should be a compiler error
        write("// UNABLE TO GENERATE EXPRESSION\n");
    }

    else
    {
        // FIXME: This should be a compiler error
        write("// UNABLE TO GENERATE EXPRESSION\n");
    }
}

void Generator::write_pattern_as_c_type(Pattern pattern)
{
    // Literals
    // if (IS(pattern, UnresolvedLiteral)) // TODO: This should be a compiler error

    if (IS_PTR(pattern, PatternLiteral))
        write_pattern_as_c_type(AS_PTR(pattern, PatternLiteral)->pattern);

    // Any pattern
    else if (IS_PTR(pattern, AnyPattern))
        write("void");

    // Primitive value
    else if (IS_PTR(pattern, PrimitiveValue))
    {
        auto value = AS_PTR(pattern, PrimitiveValue);
        write_pattern_as_c_type(value->type);
    }

    // Enum value
    else if (IS_PTR(pattern, EnumValue))
    {
        auto value = AS_PTR(pattern, EnumValue);
        write_pattern_as_c_type(value->type);
    }

    // Primitive type
    else if (IS_PTR(pattern, PrimitiveType))
    {
        auto type = AS_PTR(pattern, PrimitiveType);
        write(type->cpp_identity);
    }

    // List type
    else if (IS_PTR(pattern, ListType))
    {
        auto list_type = AS_PTR(pattern, ListType);
        write_pattern_as_c_type(list_type->list_of);
        write("[]");
    }

    // Enum type
    else if (IS_PTR(pattern, EnumType))
    {
        auto type = AS_PTR(pattern, EnumType);
        write(type->identity);
    }

    // Entity type
    else if (IS_PTR(pattern, EntityType))
        write("GambitEntity");

    // Uninferred pattern
    // else if (IS_PTR(pattern, UninferredPattern)) // TODO: This should be a compiler error

    // Invalid pattern
    // else if (IS_PTR(pattern, InvalidPattern)) // TODO: This should be a compiler error

    else
        write("void"); // TODO: This should be a compiler error
}
*/
#include "errors.h"
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

    return ir;
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
    funct.body = convert_statement(procedure->body);

    ir.functions.push_back(funct);
}

size_t Converter::create_statement(C_Statement::Kind kind)
{
    ir.statements.emplace_back();
    ir.statements.back().kind = kind;
    return ir.statements.size() - 1;
}

size_t Converter::convert_statement(Statement statement)
{
    if (IS_PTR(statement, IfStatement))
    {
        auto if_statement = AS_PTR(statement, IfStatement);
        auto s = create_statement(C_Statement::IF_STATEMENT);

        // TODO: Implement correctly
        for (auto &rule : if_statement->rules)
        {
            convert_statement(rule.code_block);
        }

        if (if_statement->else_block.has_value())
        {
            convert_statement(if_statement->else_block.value());
        }

        return s;
    }

    if (IS_PTR(statement, ForStatement))
    {
        auto for_statement = AS_PTR(statement, ForStatement);
        auto s = create_statement(C_Statement::FOR_LOOP);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, LoopStatement))
    {
        auto loop_statement = AS_PTR(statement, LoopStatement);
        auto s = create_statement(C_Statement::WHILE_LOOP);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, ReturnStatement))
    {
        auto return_statement = AS_PTR(statement, ReturnStatement);
        auto s = create_statement(C_Statement::RETURN_STATEMENT);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, WinsStatement))
    {
        auto wins_statement = AS_PTR(statement, WinsStatement);
        auto s = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, DrawStatement))
    {
        auto draw_statement = AS_PTR(statement, DrawStatement);
        auto s = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, AssignmentStatement))
    {
        auto assignment_statement = AS_PTR(statement, AssignmentStatement);
        auto s = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, VariableDeclaration))
    {
        auto variable_declaration = AS_PTR(statement, VariableDeclaration);
        auto s = create_statement(C_Statement::VARIABLE_DECLARATION);
        // TODO: Implement
        return s;
    }

    if (IS_PTR(statement, CodeBlock))
    {
        auto code_block = AS_PTR(statement, CodeBlock);
        auto s = create_statement(C_Statement::CODE_BLOCK);

        for (auto stmt : code_block->statements)
        {
            convert_statement(stmt);
        }

        ir.statements.at(s).statement_count = ir.statements.size() - (s + 1);
        return s;
    }

    if (IS(statement, Expression))
    {
        auto expr = AS(statement, Expression);
        auto s = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return s;
    }

    throw CompilerError("Could not convert APM Statement, variant not recognised.");
}
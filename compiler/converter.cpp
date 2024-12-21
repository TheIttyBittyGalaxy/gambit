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
    ir.statements.emplace_back().kind = kind;
    return ir.statements.size() - 1;
}

#define STMT ir.statements.at(stmt)

size_t Converter::convert_statement(Statement apm)
{
    auto statement_index = ir.statements.size();

    if (IS_PTR(apm, IfStatement))
    {
        auto if_statement = AS_PTR(apm, IfStatement);

        for (size_t i = 0; i < if_statement->rules.size(); i++)
        {
            auto &rule = if_statement->rules.at(i);
            auto stmt = create_statement(
                i == 0
                    ? C_Statement::IF_STATEMENT
                    : C_Statement::ELSE_IF_STATEMENT);
            STMT.expression = convert_expression(rule.condition);
            convert_statement(rule.code_block);
        }

        if (if_statement->else_block.has_value())
        {
            create_statement(C_Statement::IF_STATEMENT);
            convert_statement(if_statement->else_block.value());
        }

        return statement_index;
    }

    if (IS_PTR(apm, ForStatement))
    {
        auto for_statement = AS_PTR(apm, ForStatement);
        auto stmt = create_statement(C_Statement::FOR_LOOP);
        // TODO: Convert the range/iterator of the loop
        convert_statement(for_statement->body);
        return statement_index;
    }

    if (IS_PTR(apm, LoopStatement))
    {
        auto loop_statement = AS_PTR(apm, LoopStatement);
        auto stmt = create_statement(C_Statement::WHILE_LOOP);
        convert_statement(loop_statement->body);
        return statement_index;
    }

    if (IS_PTR(apm, ReturnStatement))
    {
        auto return_statement = AS_PTR(apm, ReturnStatement);
        auto stmt = create_statement(C_Statement::RETURN_STATEMENT);
        STMT.expression = convert_expression(return_statement->value);
        return statement_index;
    }

    if (IS_PTR(apm, WinsStatement))
    {
        auto wins_statement = AS_PTR(apm, WinsStatement);
        auto stmt = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return statement_index;
    }

    if (IS_PTR(apm, DrawStatement))
    {
        auto draw_statement = AS_PTR(apm, DrawStatement);
        auto stmt = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return statement_index;
    }

    if (IS_PTR(apm, AssignmentStatement))
    {
        auto assignment_statement = AS_PTR(apm, AssignmentStatement);
        auto stmt = create_statement(C_Statement::EXPRESSION_STATEMENT);
        // TODO: Implement
        return statement_index;
    }

    if (IS_PTR(apm, VariableDeclaration))
    {
        auto variable_declaration = AS_PTR(apm, VariableDeclaration);
        auto stmt = create_statement(C_Statement::VARIABLE_DECLARATION);
        // TODO: Implement
        return statement_index;
    }

    if (IS_PTR(apm, CodeBlock))
    {
        auto code_block = AS_PTR(apm, CodeBlock);
        auto stmt = create_statement(C_Statement::CODE_BLOCK);

        for (auto stmt : code_block->statements)
        {
            convert_statement(stmt);
        }

        STMT.statement_count = ir.statements.size() - (statement_index + 1);
        return statement_index;
    }

    if (IS(apm, Expression))
    {
        auto expr = AS(apm, Expression);
        auto stmt = create_statement(C_Statement::EXPRESSION_STATEMENT);
        STMT.expression = convert_expression(expr);
        return statement_index;
    }

    throw CompilerError("Could not convert APM Statement, variant not recognised.");
}

#undef STMT

size_t Converter::convert_expression(Expression apm)
{
    // Literals
    if (IS(apm, UnresolvedLiteral))
    {
        throw CompilerError("Attempt to convert APM UnresolvedLiteral");
    }

    if (IS_PTR(apm, ExpressionLiteral))
    {
        auto expression_literal = AS_PTR(apm, ExpressionLiteral);
        return convert_expression(expression_literal->expr);
    }

    C_Expression expr;
    expr.kind = C_Expression::INVALID;

    // Values
    if (IS_PTR(apm, PrimitiveValue))
    {
        auto primitive_value = AS_PTR(apm, PrimitiveValue);

        if (IS(primitive_value->value, double))
        {
            expr.kind = C_Expression::DOUBLE_LITERAL;
            expr.double_value = AS(primitive_value->value, double);
        }
        else if (IS(primitive_value->value, int))
        {
            expr.kind = C_Expression::INT_LITERAL;
            expr.int_value = AS(primitive_value->value, int);
        }
        else if (IS(primitive_value->value, bool))
        {
            expr.kind = C_Expression::BOOL_LITERAL;
            expr.bool_value = AS(primitive_value->value, bool);
        }
        else if (IS(primitive_value->value, string))
        {
            expr.kind = C_Expression::STRING_LITERAL;
            expr.string_value = AS(primitive_value->value, string);
        }
        else
        {
            throw CompilerError("Could not convert APM PrimitiveValue.");
        }
    }

    if (IS_PTR(apm, ListValue))
    {
        auto list_value = AS_PTR(apm, ListValue);
        // TODO: Implement
    }

    if (IS_PTR(apm, EnumValue))
    {
        auto enum_value = AS_PTR(apm, EnumValue);
        // TODO: Implement
    }

    if (IS_PTR(apm, Variable))
    {
        auto variable = AS_PTR(apm, Variable);
        // TODO: Implement
    }

    // Operations
    if (IS_PTR(apm, Unary))
    {
        auto unary = AS_PTR(apm, Unary);
        if (false)
            ;
        else
            throw CompilerError("Could not convert Unary " + unary->op);

        expr.lhs = convert_expression(unary->value);
    }

    if (IS_PTR(apm, Binary))
    {
        auto binary = AS_PTR(apm, Binary);
        if (binary->op == "+")
            expr.kind = C_Expression::BINARY_ADD;
        else if (binary->op == "-")
            expr.kind = C_Expression::BINARY_SUB;
        else if (binary->op == "*")
            expr.kind = C_Expression::BINARY_MUL;
        else if (binary->op == "/")
            expr.kind = C_Expression::BINARY_DIV;
        else if (binary->op == "==")
            expr.kind = C_Expression::BINARY_EQUAL;
        else if (binary->op == "and")
            expr.kind = C_Expression::BINARY_AND;
        else if (binary->op == "or")
            expr.kind = C_Expression::BINARY_OR;
        else
            throw CompilerError("Could not convert Binary " + binary->op);

        expr.lhs = convert_expression(binary->lhs);
        expr.rhs = convert_expression(binary->rhs);
    }

    // Indexing
    if (IS_PTR(apm, InstanceList))
    {
        auto instance_list = AS_PTR(apm, InstanceList);
        // TODO: Implement
    }

    if (IS_PTR(apm, IndexWithExpression))
    {
        auto index_with_expression = AS_PTR(apm, IndexWithExpression);
        // TODO: Implement
    }

    if (IS_PTR(apm, IndexWithIdentity))
    {
        auto index_with_identity = AS_PTR(apm, IndexWithIdentity);
        // TODO: Implement
    }

    // Calls
    if (IS_PTR(apm, Call))
    {
        auto call = AS_PTR(apm, Call);
        // TODO: Implement
    }

    if (IS_PTR(apm, PropertyAccess))
    {
        auto property_access = AS_PTR(apm, PropertyAccess);
        // TODO: Implement
    }

    // Keyword expressions
    if (IS_PTR(apm, ChooseExpression))
    {
        auto choose_expression = AS_PTR(apm, ChooseExpression);
        // TODO: Implement
    }

    // "Statement style" expressions
    if (IS_PTR(apm, IfExpression))
    {
        auto if_expression = AS_PTR(apm, IfExpression);
        // TODO: Implement
    }

    if (IS_PTR(apm, MatchExpression))
    {
        auto match_expression = AS_PTR(apm, MatchExpression);
        // TODO: Implement
    }

    // Invalid expression
    if (IS_PTR(apm, InvalidExpression))
    {
        throw CompilerError("Attempt to convert APM InvalidExpression");
    }

    // TODO: Should error here!
    // if (e.kind == INVALID)
    //     throw CompilerError("Could not convert APM Expression, variant not recognised.");

    auto expression_index = ir.expressions.size();
    ir.expressions.emplace_back(expr);
    return expression_index;
}
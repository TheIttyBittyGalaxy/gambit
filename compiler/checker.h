#pragma once
#ifndef CHECKER_H
#define CHECKER_H

#include "apm.h"
#include "source.h"
#include "utilty.h"
using namespace std;

class Checker
{
public:
    void check(Source &source, ptr<Program> program);

private:
    ptr<Program> program = nullptr;
    Source *source = nullptr;

    // PROGRAM STRUCTURE //
    void check_program(ptr<Program> program);
    void check_scope(ptr<Scope> scope);
    void check_scope_lookup_value(Scope::LookupValue value, ptr<Scope> scope);
    void check_code_block(ptr<CodeBlock> code_block);

    // STATEMENTS //
    void check_statement(Statement statement, ptr<Scope> scope);

    void check_if_statement(ptr<IfStatement> stmt, ptr<Scope> scope);
    void check_for_statement(ptr<ForStatement> stmt, ptr<Scope> scope);
    void check_assignment_statement(ptr<AssignmentStatement> stmt, ptr<Scope> scope);
    void check_variable_declaration(ptr<VariableDeclaration> stmt, ptr<Scope> scope);

    // EXPRESSIONS //
    void check_expression(Expression expression, ptr<Scope> scope);

    void check_list_value(ptr<ListValue> list, ptr<Scope> scope);

    void check_unary(ptr<Unary> unary, ptr<Scope> scope);
    void check_binary(ptr<Binary> binary, ptr<Scope> scope);

    void check_instance_list(ptr<InstanceList> list, ptr<Scope> scope);
    void check_expression_index(ptr<ExpressionIndex> expression_index, ptr<Scope> scope);
    void check_property_index(ptr<PropertyIndex> property_index, ptr<Scope> scope);

    void check_call(ptr<Call> call, ptr<Scope> scope);

    void check_if_expression(ptr<IfExpression> if_expression, ptr<Scope> scope);
    void check_match(ptr<MatchExpression> match, ptr<Scope> scope);
};

#endif
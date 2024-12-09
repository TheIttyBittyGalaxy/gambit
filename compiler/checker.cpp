#include "checker.h"
#include "intrinsic.h"

// TODO: Currently, I assume that the checker will never actually modify
//       the APM, only read it. It may be worth formalising that assumption
//       by making the parameters to each method immutable.

// TODO: As an item of curiosity, I'm sure that checking actually has to be
//       done as a tree walk? If the checker never modifies the APM, than the
//       order in which nodes are checked should not effect the outcome.
//
//       In theory then, homogenous nodes could be put in arrays and iterated
//       over to be checked, instead of doing a tree walk. It's not immediately
//       clear to me however if this would be more performant? On one hand, this
//       approach deploy memory more efficiently. On the other, using tree decent
//       may make it easier to avoid duplicate work? (I'm also not sure how
//       homogenous the nodes would really be?)

void Checker::check(Source &source, ptr<Program> program)
{
    this->source = &source;
    check_program(program);
}

// PROGRAM STRUCTURE //

void Checker::check_program(ptr<Program> program)
{
    check_scope(program->global_scope);
}

void Checker::check_scope(ptr<Scope> scope)
{
    for (auto index : scope->lookup)
        check_scope_lookup_value(index.second, scope);
}

void Checker::check_scope_lookup_value(Scope::LookupValue value, ptr<Scope> scope)
{

    if (IS_PTR(value, Scope::OverloadedIdentity))
    {
        auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
        for (auto overload : overloaded_identity->overloads)
            check_scope_lookup_value(overload, scope);

        // TODO: Check that no overloads share the same signature
    }

    else if (IS_PTR(value, Procedure))
    {
        auto proc = AS_PTR(value, Procedure);
        check_code_block(proc->body);

        // FIXME: If the body is a singleton, check the statement as if it were a return expression
    }

    else if (IS_PTR(value, Variable))
        ; // pass

    else if (IS_PTR(value, StateProperty))
    {
        auto state = AS_PTR(value, StateProperty);
        if (state->initial_value.has_value())
        {
            auto initial_value = state->initial_value.value();
            check_expression(initial_value, state->scope);

            auto initial_value_pattern = determine_expression_pattern(initial_value);
            if (!is_pattern_subset_of_superset(initial_value_pattern, state->pattern))
                source->log_error("Default value for state is the incorrect type.", get_span(initial_value));
        }
    }

    else if (IS_PTR(value, FunctionProperty))
    {
        auto funct = AS_PTR(value, FunctionProperty);
        if (funct->body.has_value())
            check_code_block(funct->body.value());

        // FIXME: If the body is a singleton, check the statement as if it were a return expression
    }

    else if (IS(value, Pattern))
        ; // pass

    else
        throw CompilerError("Unable to check Scope Lookup Value");
}

void Checker::check_code_block(ptr<CodeBlock> code_block)
{
    check_scope(code_block->scope);
    for (auto stmt : code_block->statements)
        check_statement(stmt, code_block->scope);
}

// STATEMENTS //

void Checker::check_statement(Statement stmt, ptr<Scope> scope)
{
    if (IS_PTR(stmt, IfStatement))
        check_if_statement(AS_PTR(stmt, IfStatement), scope);
    else if (IS_PTR(stmt, ForStatement))
        check_for_statement(AS_PTR(stmt, ForStatement), scope);
    else if (IS_PTR(stmt, LoopStatement))
        check_loop_statement(AS_PTR(stmt, LoopStatement), scope);
    else if (IS_PTR(stmt, ReturnStatement))
        check_return_statement(AS_PTR(stmt, ReturnStatement), scope);
    else if (IS_PTR(stmt, WinsStatement))
        check_wins_statement(AS_PTR(stmt, WinsStatement), scope);
    else if (IS_PTR(stmt, DrawStatement))
        ; // skip
    else if (IS_PTR(stmt, AssignmentStatement))
        check_assignment_statement(AS_PTR(stmt, AssignmentStatement), scope);
    else if (IS_PTR(stmt, VariableDeclaration))
        check_variable_declaration(AS_PTR(stmt, VariableDeclaration), scope);

    else if (IS_PTR(stmt, CodeBlock))
        check_code_block(AS_PTR(stmt, CodeBlock));
    else if (IS(stmt, Expression))
        check_expression(AS(stmt, Expression), scope);

    else
        throw CompilerError("Cannot check Statement variant.", get_span(stmt));
}

void Checker::check_if_statement(ptr<IfStatement> stmt, ptr<Scope> scope)
{
    for (auto rule : stmt->rules)
    {
        check_expression(rule.condition, scope);
        check_code_block(rule.code_block);

        auto condition_pattern = determine_expression_pattern(rule.condition);
        bool is_bool_condition = is_pattern_subset_of_superset(condition_pattern, Intrinsic::type_bool);
        bool is_optional_condition = is_pattern_optional(condition_pattern);
        if (!is_bool_condition && !is_optional_condition)
            source->log_error("Condition must evaluate either to true or false, or potentially to none. This condition will never be true, false, or none.", rule.span);
    }

    if (stmt->else_block.has_value())
        check_code_block(stmt->else_block.value());
}

void Checker::check_for_statement(ptr<ForStatement> stmt, ptr<Scope> scope)
{
    // TODO: Check that range is actually iterable
    check_expression(stmt->range, scope);
    check_code_block(stmt->body);
}

void Checker::check_loop_statement(ptr<LoopStatement> stmt, ptr<Scope> scope)
{
    // TODO: Check that code block has break condition
    check_code_block(stmt->body);
}

void Checker::check_return_statement(ptr<ReturnStatement> stmt, ptr<Scope> scope)
{
    // TODO: Check that pattern of expression matches return pattern of the function
    check_expression(stmt->value, scope);
}

void Checker::check_wins_statement(ptr<WinsStatement> stmt, ptr<Scope> scope)
{
    // TODO: Check that pattern of expression is a player entity
    check_expression(stmt->player, scope);
}

void Checker::check_assignment_statement(ptr<AssignmentStatement> stmt, ptr<Scope> scope)
{
    auto subject_pattern = determine_expression_pattern(stmt->subject);
    auto value_pattern = determine_expression_pattern(stmt->value);
    if (!is_pattern_subset_of_superset(value_pattern, subject_pattern))
        source->log_error("Assigned value does not match the pattern of the subject.", stmt->span);
}

void Checker::check_variable_declaration(ptr<VariableDeclaration> stmt, ptr<Scope> scope)
{
    if (stmt->value.has_value())
    {
        auto value_pattern = determine_expression_pattern(stmt->value.value());
        if (!is_pattern_subset_of_superset(value_pattern, stmt->variable->pattern))
            source->log_error("Assigned value does not match the pattern of the variable.", stmt->span);
    }
}

// EXPRESSIONS //

void Checker::check_expression(Expression expr, ptr<Scope> scope)
{

    if (IS(expr, UnresolvedLiteral))
        throw CompilerError("Attempt to check UnresolvedLiteral. This should have already been resolved.");
    else if (IS_PTR(expr, ExpressionLiteral))
        check_expression(AS_PTR(expr, ExpressionLiteral)->expr, scope);

    else if (IS_PTR(expr, PrimitiveValue))
        ; // skip
    else if (IS_PTR(expr, ListValue))
        check_list_value(AS_PTR(expr, ListValue), scope);
    else if (IS_PTR(expr, EnumValue))
        ; // skip
    else if (IS_PTR(expr, Variable))
        ; // skip

    else if (IS_PTR(expr, Unary))
        check_unary(AS_PTR(expr, Unary), scope);
    else if (IS_PTR(expr, Binary))
        check_binary(AS_PTR(expr, Binary), scope);

    else if (IS_PTR(expr, InstanceList))
        check_instance_list(AS_PTR(expr, InstanceList), scope);
    else if (IS_PTR(expr, IndexWithExpression))
        check_index_with_expression(AS_PTR(expr, IndexWithExpression), scope);
    else if (IS_PTR(expr, IndexWithIdentity))
        check_index_with_identity(AS_PTR(expr, IndexWithIdentity), scope);

    else if (IS_PTR(expr, Call))
        check_call(AS_PTR(expr, Call), scope);
    else if (IS_PTR(expr, PropertyAccess))
        check_property_access(AS_PTR(expr, PropertyAccess), scope);

    else if (IS_PTR(expr, ChooseExpression))
        check_choose_expression(AS_PTR(expr, ChooseExpression), scope);

    else if (IS_PTR(expr, IfExpression))
        check_if_expression(AS_PTR(expr, IfExpression), scope);
    else if (IS_PTR(expr, MatchExpression))
        check_match(AS_PTR(expr, MatchExpression), scope);

    else if (IS_PTR(expr, InvalidExpression))
        ; // skip

    else
        throw CompilerError("Cannot check Expression variant.", get_span(expr));
}

void Checker::check_list_value(ptr<ListValue> list, ptr<Scope> scope)
{
    for (auto value : list->values)
        check_expression(value, scope);
}

void Checker::check_instance_list(ptr<InstanceList> list, ptr<Scope> scope)
{
    for (auto value : list->values)
        check_expression(value, scope);
}

void Checker::check_call(ptr<Call> call, ptr<Scope> scope)
{
    // TODO: Check callee / arguments

    for (auto &argument : call->arguments)
        check_expression(argument.value, scope);
}

void Checker::check_choose_expression(ptr<ChooseExpression> choose_expression, ptr<Scope> scope)
{
    // TODO: Check player is a player entity
    // TODO: Check prompt is string
    check_expression(choose_expression->choices, scope);
    check_expression(choose_expression->player, scope);
    check_expression(choose_expression->prompt, scope);
}

void Checker::check_if_expression(ptr<IfExpression> if_expression, ptr<Scope> scope)
{
    for (auto &rule : if_expression->rules)
    {
        check_expression(rule.condition, scope);
        check_expression(rule.result, scope);

        auto condition_pattern = determine_expression_pattern(rule.condition);
        bool is_bool_condition = is_pattern_subset_of_superset(condition_pattern, Intrinsic::type_bool);
        bool is_optional_condition = is_pattern_optional(condition_pattern);
        if (!is_bool_condition && !is_optional_condition)
            source->log_error("Condition must evaluate either to true or false, or potentially to none. This condition will never be true, false, or none.", rule.span);
    }
}

void Checker::check_match(ptr<MatchExpression> match, ptr<Scope> scope)
{
    check_expression(match->subject, scope);
    auto subject_pattern = determine_expression_pattern(match->subject);

    for (auto &rule : match->rules)
    {
        // FIXME: Check that rule's pattern is static
        check_expression(rule.result, scope);

        if (!do_patterns_overlap(rule.pattern, subject_pattern))
            source->log_error("This rule's pattern will never match.", get_span(rule.pattern));
    }
}

void Checker::check_index_with_expression(ptr<IndexWithExpression> index_with_expression, ptr<Scope> scope)
{
    check_expression(index_with_expression->subject, scope);
    check_expression(index_with_expression->index, scope);

    // TODO: Check that the subject is an array/list
    // TODO: Check that the index is an integer
}

void Checker::check_index_with_identity(ptr<IndexWithIdentity> index_with_identity, ptr<Scope> scope)
{
    throw CompilerError("Attempt to check IndexWithIdentity expression. This should have already been resolved to a PropertyAccess or EnumValue");
}

void Checker::check_property_access(ptr<PropertyAccess> property_access, ptr<Scope> scope)
{
    check_expression(property_access->subject, scope);
}

void Checker::check_unary(ptr<Unary> unary, ptr<Scope> scope)
{
    check_expression(unary->value, scope);
    // TODO: Pattern check the operand
}

void Checker::check_binary(ptr<Binary> binary, ptr<Scope> scope)
{
    check_expression(binary->lhs, scope);
    check_expression(binary->rhs, scope);
    // TODO: Pattern check the operands
}

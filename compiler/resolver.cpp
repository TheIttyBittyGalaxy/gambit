#include "errors.h"
#include "resolver.h"
#include <optional>

void Resolver::resolve(ptr<Program> program)
{
    resolve_program(program);
}

void Resolver::resolve_program(ptr<Program> program)
{
    resolve_scope(program->global_scope);
}

void Resolver::resolve_code_block(ptr<CodeBlock> code_block, optional<Pattern> pattern_hint)
{
    resolve_scope(code_block->scope);

    if (code_block->singleton_block)
    {
        // The statement of a singleton code block is given the pattern hint,
        // whereas statements in a regular code block are not.
        auto stmt = code_block->statements[0];
        stmt = resolve_statement(stmt, code_block->scope, pattern_hint);
        code_block->statements[0] = stmt;
    }
    else
    {
        for (size_t i = 0; i < code_block->statements.size(); i++)
        {
            auto stmt = code_block->statements[i];
            stmt = resolve_statement(stmt, code_block->scope);
            code_block->statements[i] = stmt;
        }
    }
}

void Resolver::resolve_scope(ptr<Scope> scope)
{
    for (auto index : scope->lookup)
        resolve_scope_lookup_value(index.second, scope);
}

void Resolver::resolve_scope_lookup_value(Scope::LookupValue value, ptr<Scope> scope)
{
    if (IS_PTR(value, StateProperty))
        resolve_state_property(AS_PTR(value, StateProperty), scope);

    if (IS_PTR(value, FunctionProperty))
        resolve_function_property(AS_PTR(value, FunctionProperty), scope);

    if (IS_PTR(value, Scope::OverloadedIdentity))
    {
        auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
        for (auto overload : overloaded_identity->overloads)
            resolve_scope_lookup_value(overload, scope);
    }
}

void Resolver::resolve_state_property(ptr<StateProperty> state, ptr<Scope> scope)
{
    state->pattern = resolve_pattern(state->pattern, scope);

    resolve_pattern_list(state->pattern_list, scope);

    if (state->initial_value.has_value())
    {
        state->initial_value = resolve_expression(state->initial_value.value(), scope, state->pattern);
        // FIXME: Pattern check the default value
    }
}

void Resolver::resolve_function_property(ptr<FunctionProperty> funct, ptr<Scope> scope)
{
    funct->pattern = resolve_pattern(funct->pattern, scope);

    resolve_pattern_list(funct->pattern_list, scope);

    if (funct->body.has_value())
    {
        auto body = funct->body.value();

        for (auto parameter_pattern : funct->pattern_list->patterns)
        {
            auto parameter = CREATE(Variable);
            parameter->identity = parameter_pattern->name;
            parameter->pattern = parameter_pattern->pattern;
            declare(body->scope, parameter);
        }

        resolve_code_block(body, funct->pattern);
    }
}

void Resolver::resolve_named_pattern(ptr<NamedPattern> named_pattern, ptr<Scope> scope)
{
    // FIXME: Should named patterns be responsible for declaring their own variables?
    named_pattern->pattern = resolve_pattern(named_pattern->pattern, scope);
}

void Resolver::resolve_pattern_list(ptr<PatternList> pattern_list, ptr<Scope> scope)
{
    for (auto pattern : pattern_list->patterns)
        resolve_named_pattern(pattern, scope);
}

void Resolver::resolve_optional_pattern(ptr<OptionalPattern> optional_pattern, ptr<Scope> scope)
{
    optional_pattern->pattern = resolve_pattern(optional_pattern->pattern, scope);
}

Pattern Resolver::resolve_pattern(Pattern pattern, ptr<Scope> scope)
{
    if (IS_PTR(pattern, InvalidPattern))
        ; // pass

    else if (IS_PTR(pattern, NativeType) || IS_PTR(pattern, EnumType) || IS_PTR(pattern, Entity))
        ; // pass

    else if (IS_PTR(pattern, NamedPattern))
        resolve_named_pattern(AS_PTR(pattern, NamedPattern), scope);

    else if (IS_PTR(pattern, OptionalPattern))
        resolve_optional_pattern(AS_PTR(pattern, OptionalPattern), scope);

    else if (IS_PTR(pattern, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(pattern, UnresolvedIdentity);
        string id = unresolved_identity->identity;

        if (declared_in_scope(scope, id))
        {
            auto resolved = fetch(scope, id);

            if (IS_PTR(resolved, NativeType))
                return AS_PTR(resolved, NativeType);
            if (IS_PTR(resolved, EnumType))
                return AS_PTR(resolved, EnumType);
            if (IS_PTR(resolved, Entity))
                return AS_PTR(resolved, Entity);

            auto unresolved_identity = AS_PTR(pattern, UnresolvedIdentity);
            emit_error("'" + identity_of(resolved) + "' is not a type", unresolved_identity->token);
        }
        else
        {
            emit_error("'" + id + "' is not defined.", unresolved_identity->token);
        }

        return CREATE(InvalidPattern);
    }

    else
        throw runtime_error("Cannot resolve Pattern variant."); // FIXME: Use an appropriate exception type

    return pattern;
}

Expression Resolver::resolve_expression(Expression expression, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    if (IS_PTR(expression, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
        string id = unresolved_identity->identity;

        if (declared_in_scope(scope, id)) // Resolve identity from scope
        {
            auto resolved = fetch(scope, id);

            if (IS_PTR(resolved, Variable))
                return AS_PTR(resolved, Variable);

            // FIXME: Make error more informative by saying _what_ the resolved object is (e.g. an entity, a type, etc)
            emit_error("Expected value, got '" + id + "'", unresolved_identity->token);

            return CREATE(InvalidValue);
        }

        if (pattern_hint.has_value()) // Resolve identity from pattern hint
        {
            auto the_pattern_hint = pattern_hint.value();
            optional<ptr<EnumType>> enum_type = {};

            if (IS_PTR(the_pattern_hint, EnumType)) // Pattern hint is enum type
            {
                enum_type = AS_PTR(the_pattern_hint, EnumType);
            }
            else if (IS_PTR(the_pattern_hint, OptionalPattern)) // Pattern hint is optional enum type
            {
                auto optional_pattern = AS_PTR(the_pattern_hint, OptionalPattern);
                if (IS_PTR(optional_pattern->pattern, EnumType))
                    enum_type = AS_PTR(optional_pattern->pattern, EnumType);
            }

            if (enum_type.has_value())
                for (const auto &value : enum_type.value()->values)
                    if (value->identity == unresolved_identity->identity)
                        return value;
        }

        emit_error("'" + id + "' is not defined.", unresolved_identity->token);
        return CREATE(InvalidValue);
    }
    else if (IS_PTR(expression, Variable))
        ; // pass
    else if (IS_PTR(expression, Literal))
        ; // pass
    else if (IS_PTR(expression, ListValue))
        resolve_list_value(AS_PTR(expression, ListValue), scope, pattern_hint);
    else if (IS_PTR(expression, InstanceList))
        resolve_instance_list(AS_PTR(expression, InstanceList), scope, pattern_hint);
    else if (IS_PTR(expression, EnumValue))
        ; // pass
    else if (IS_PTR(expression, Unary))
        resolve_unary(AS_PTR(expression, Unary), scope, pattern_hint);
    else if (IS_PTR(expression, Binary))
        resolve_binary(AS_PTR(expression, Binary), scope, pattern_hint);
    else if (IS_PTR(expression, PropertyIndex))
        resolve_property_index(AS_PTR(expression, PropertyIndex), scope, pattern_hint);
    else if (IS_PTR(expression, Match))
        resolve_match(AS_PTR(expression, Match), scope, pattern_hint);
    else
        throw runtime_error("Cannot resolve Expression variant."); // FIXME: Use an appropriate exception type

    return expression;
}

void Resolver::resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
        list->values[i] = resolve_expression(list->values[i], scope, pattern_hint);
}

void Resolver::resolve_instance_list(ptr<InstanceList> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
        list->values[i] = resolve_expression(list->values[i], scope, pattern_hint);
}

void Resolver::resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    match->subject = resolve_expression(match->subject, scope);
    // FIXME: Implement pattern checking on the subject
    // auto subject_type = determine_type(match->subject);

    for (auto &rule : match->rules)
    {
        rule.pattern = resolve_expression(rule.pattern, scope);
        // FIXME: Implement pattern checking and inference on the pattern
        rule.result = resolve_expression(rule.result, scope, pattern_hint);
    }
}

void Resolver::resolve_property_index(ptr<PropertyIndex> property_index, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    property_index->expr = resolve_expression(property_index->expr, scope);
    // TODO: Resolve property
}

void Resolver::resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    unary->value = resolve_expression(unary->value, scope);
    // FIXME: Implement pattern checking on the operands
}

void Resolver::resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    binary->lhs = resolve_expression(binary->lhs, scope);
    binary->rhs = resolve_expression(binary->rhs, scope);
    // FIXME: Implement pattern checking on the operands
}

Statement Resolver::resolve_statement(Statement stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    if (IS_PTR(stmt, CodeBlock))
        resolve_code_block(AS_PTR(stmt, CodeBlock), pattern_hint);
    else if (IS(stmt, Expression))
        return resolve_expression(AS(stmt, Expression), scope, pattern_hint);
    else
        throw runtime_error("Cannot resolve Expression variant."); // FIXME: Use an appropriate exception type

    return stmt;
}
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
    auto resolved_type = resolve_type(state->type, scope);
    state->type = resolved_type.has_value() ? resolved_type.value() : CREATE(InvalidType);

    resolve_pattern_list(state->pattern_list, scope);

    if (state->initial_value.has_value())
    {
        state->initial_value = resolve_expression(state->initial_value.value(), scope, state->type);
        // FIXME: Type check the default value
    }
}

void Resolver::resolve_function_property(ptr<FunctionProperty> funct, ptr<Scope> scope)
{
    auto resolved_type = resolve_type(funct->type, scope);
    funct->type = resolved_type.has_value() ? resolved_type.value() : CREATE(InvalidType);

    resolve_pattern_list(funct->pattern_list, scope);

    if (funct->body.has_value())
        funct->body = resolve_expression(funct->body.value(), scope, funct->type);
}

void Resolver::resolve_pattern(ptr<Pattern> pattern, ptr<Scope> scope)
{
    auto resolved_type = resolve_type(pattern->type, scope);
    pattern->type = resolved_type.has_value() ? resolved_type.value() : CREATE(InvalidType);
}

void Resolver::resolve_pattern_list(ptr<PatternList> pattern_list, ptr<Scope> scope)
{
    for (auto pattern : pattern_list->patterns)
        resolve_pattern(pattern, scope);
}

Type Resolver::resolve_optional_type(ptr<OptionalType> type, ptr<Scope> scope)
{
    auto resolved = resolve_type(type->type, scope);
    if (resolved.has_value())
        type->type = resolved.value();
    return type;
}

optional<Type> Resolver::resolve_type(Type type, ptr<Scope> scope)
{
    if (IS_PTR(type, NativeType) || IS_PTR(type, EnumType) || IS_PTR(type, Entity))
        return type;

    if (IS_PTR(type, OptionalType))
        return resolve_optional_type(AS_PTR(type, OptionalType), scope);

    if (IS_PTR(type, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(type, UnresolvedIdentity);
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

            auto unresolved_identity = AS_PTR(type, UnresolvedIdentity);
            emit_error("'" + identity_of(resolved) + "' is not a type", unresolved_identity->token);
        }
        else
        {
            emit_error("'" + id + "' is not defined.", unresolved_identity->token);
        }

        return {};
    }

    throw runtime_error("Cannot resolve type variant."); // FIXME: Use an appropriate exception type
}

Expression Resolver::resolve_expression(Expression expression, ptr<Scope> scope, optional<Type> type_hint)
{
    if (IS_PTR(expression, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
        string id = unresolved_identity->identity;

        if (declared_in_scope(scope, id)) // Resolve identity from scope
        {
            auto resolved = fetch(scope, id);

            // FIXME: Return resolved value if it makes for a valid expression

            // FIXME: Make error more informative by saying _what_ the resolved object is (e.g. an entity, a type, etc)
            emit_error("Expected value, got '" + id + "'", unresolved_identity->token);

            return CREATE(InvalidValue);
        }

        if (type_hint.has_value()) // Resolve identity from type hint
        {
            auto the_type_hint = type_hint.value();
            optional<ptr<EnumType>> enum_type = {};

            if (IS_PTR(the_type_hint, EnumType)) // Type hint is enum type
            {
                enum_type = AS_PTR(the_type_hint, EnumType);
            }
            else if (IS_PTR(the_type_hint, OptionalType)) // Type hint is optional enum type
            {
                auto opt_type = AS_PTR(the_type_hint, OptionalType);
                if (IS_PTR(opt_type->type, EnumType))
                    enum_type = AS_PTR(opt_type->type, EnumType);
            }

            if (enum_type.has_value())
                for (const auto &value : enum_type.value()->values)
                    if (value->identity == unresolved_identity->identity)
                        return value;
        }

        emit_error("'" + id + "' is not defined.", unresolved_identity->token);
        return CREATE(InvalidValue);
    }
    else if (IS_PTR(expression, Literal))
        ; // pass
    else if (IS_PTR(expression, ListValue))
        resolve_list_value(AS_PTR(expression, ListValue), scope, type_hint);
    else if (IS_PTR(expression, EnumValue))
        ; // pass
    else if (IS_PTR(expression, Unary))
        resolve_unary(AS_PTR(expression, Unary), scope, type_hint);
    else if (IS_PTR(expression, Binary))
        resolve_binary(AS_PTR(expression, Binary), scope, type_hint);
    else if (IS_PTR(expression, Match))
        resolve_match(AS_PTR(expression, Match), scope, type_hint);
    else
        throw runtime_error("Cannot resolve Expression variant."); // FIXME: Use an appropriate exception type

    return expression;
}

void Resolver::resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Type> type_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
        list->values[i] = resolve_expression(list->values[i], scope, type_hint);
}

void Resolver::resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Type> type_hint)
{
    match->subject = resolve_expression(match->subject, scope);
    // FIXME: Implement type checking on the subject
    // auto subject_type = determine_type(match->subject);

    for (auto &rule : match->rules)
    {
        rule.pattern = resolve_expression(rule.pattern, scope);

        // FIXME: Implement type checking and inference on the pattern
        // resolve_expression(rule.pattern, subject_type);
        // auto pattern_type = determine_type(rule.pattern);
        // if (!is_subtype_of(pattern_type, subject_type))
        // {
        //     emit_error("Pattern's type is not compatible with the type of the match subject"); // FIXME: Improve this error message
        // }

        rule.result = resolve_expression(rule.result, scope, type_hint);
    }
}

void Resolver::resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Type> type_hint)
{
    unary->value = resolve_expression(unary->value, scope);
    // FIXME: Implement type checking on the operands
}

void Resolver::resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Type> type_hint)
{
    binary->lhs = resolve_expression(binary->lhs, scope);
    binary->rhs = resolve_expression(binary->rhs, scope);
    // FIXME: Implement type checking on the operands
}
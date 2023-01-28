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
};

void Resolver::resolve_scope(ptr<Scope> scope)
{
    for (auto entry : scope->lookup)
    {
        auto value = entry.second;
        if (IS_PTR(value, Entity))
            resolve_entity_definition(AS_PTR(value, Entity), scope);
    }
};

Type Resolver::resolve_optional_type(ptr<OptionalType> type, ptr<Scope> scope)
{
    auto resolved = resolve_type(type->type, scope);
    if (resolved.has_value())
        type->type = resolved.value();
    return type;
};

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
};

void Resolver::resolve_entity_definition(ptr<Entity> definition, ptr<Scope> scope)
{
    // FIXME: Hoist additive definition that never find a base definition into a base definition in a parent scope.
    //        If no such base definition exists, throw an `Entity cannot be extended as it is not defined` error.
    //        Hoisting should happen before any identity resolution, other an identity could resolve to an entity
    //        defintion that will later get hoisted.
    if (!definition->base_definition_found)
    {
        emit_error("Entity `" + definition->identity + "` does not have a base definition."); // FIXME: Supply the token of the identity
    }

    for (auto entry : definition->fields)
    {
        auto field = entry.second;
        resolve_entity_field(field, definition, scope);
    }
};

void Resolver::resolve_entity_field(ptr<EntityField> field, ptr<Entity> entity, ptr<Scope> scope)
{
    auto resolved_type = resolve_type(field->type, scope);
    field->type = resolved_type.has_value() ? resolved_type.value() : CREATE(InvalidType);

    if (field->initializer.has_value())
    {
        // FIXME: Scope passed into resolve_expression should be scope of the entity definition, not of the parent scope
        field->initializer = resolve_expression(field->initializer.value(), scope, field->type);
        // FIXME: Type check the default value
    }
};

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

            if (IS_PTR(resolved, NativeType) || IS_PTR(resolved, EnumType) || IS_PTR(resolved, Entity))
                emit_error("Expected value, got type '" + identity_of(resolved) + "'", unresolved_identity->token);
            else
                emit_error("Expected value, got '" + identity_of(resolved) + "'", unresolved_identity->token);

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
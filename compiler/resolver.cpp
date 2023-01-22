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

optional<Scope::LookupValue> Resolver::resolve_identity(ptr<UnresolvedIdentity> unresolved_identity, ptr<Scope> scope)
{
    string id = unresolved_identity->identity;
    if (declared_in_scope(scope, id))
        return fetch(scope, id);

    emit_error("'" + id + "' is not defined.", unresolved_identity->token); // FIXME: Supply the token of the identity
    return {};
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
        auto opt_resolved = resolve_identity(AS_PTR(type, UnresolvedIdentity), scope);
        if (opt_resolved.has_value())
        {
            auto resolved = opt_resolved.value();
            if (IS_PTR(resolved, NativeType))
                return AS_PTR(resolved, NativeType);
            if (IS_PTR(resolved, EnumType))
                return AS_PTR(resolved, EnumType);
            if (IS_PTR(resolved, Entity))
                return AS_PTR(resolved, Entity);

            auto unresolved_identity = AS_PTR(type, UnresolvedIdentity);
            emit_error("'" + identity_of(resolved) + "' is not a type", unresolved_identity->token);
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

    for (auto entry : definition->fields)
    {
        auto field = entry.second;
        resolve_entity_field(field, definition, scope);
    }
};

void Resolver::resolve_entity_field(ptr<EntityField> field, ptr<Entity> entity, ptr<Scope> scope)
{
    auto resolved_type = resolve_type(field->type, scope);
    if (resolved_type.has_value())
        field->type = resolved_type.value();
};

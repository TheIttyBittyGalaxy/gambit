#include <stdexcept>
#include "apm.h"

string identity_of(Scope::LookupValue value)
{
    if (IS_PTR(value, NativeType))
        return AS_PTR(value, NativeType)->identity;

    if (IS_PTR(value, EnumType))
        return AS_PTR(value, EnumType)->identity;

    if (IS_PTR(value, Entity))
        return AS_PTR(value, Entity)->identity;

    if (IS_PTR(value, State))
        return AS_PTR(value, State)->identity;

    // FIXME: Throw an appropriate error object
    throw runtime_error("Cannot get identity of Scope::LookupValue");
}

bool directly_declared_in_scope(ptr<Scope> scope, string identity)
{
    return scope->lookup.find(identity) != scope->lookup.end();
}

bool declared_in_scope(ptr<Scope> scope, string identity)
{
    while (!directly_declared_in_scope(scope, identity) && !scope->parent.expired())
        scope = ptr<Scope>(scope->parent);

    return directly_declared_in_scope(scope, identity);
}

void declare(ptr<Scope> scope, Scope::LookupValue value)
{
    string identity = identity_of(value);

    // FIXME: Throw an appropriate error object
    if (directly_declared_in_scope(scope, identity))
        throw runtime_error("Cannot declare " + identity + " in scope, as " + identity + " already exists.");

    scope->lookup.insert({identity, value});
}

Scope::LookupValue fetch(ptr<Scope> scope, string identity)
{
    while (!directly_declared_in_scope(scope, identity) && !scope->parent.expired())
        scope = ptr<Scope>(scope->parent);

    if (directly_declared_in_scope(scope, identity))
        return scope->lookup.at(identity);

    // FIXME: Throw an appropriate error object
    throw runtime_error("Cannot fetch " + identity + " in scope, as it does not exist.");
}

ptr<NativeType> fetch_native_type(ptr<Scope> scope, string identity)
{
    auto found = fetch(scope, identity);
    if (!IS_PTR(found, NativeType))
        // FIXME: Throw an appropriate error object
        throw runtime_error("'" + identity + "' is not a NativeType");
    return AS_PTR(found, NativeType);
}

ptr<EnumType> fetch_enum_type(ptr<Scope> scope, string identity)
{
    auto found = fetch(scope, identity);
    if (!IS_PTR(found, EnumType))
        // FIXME: Throw an appropriate error object
        throw runtime_error("'" + identity + "' is not a EnumType");
    return AS_PTR(found, EnumType);
}

ptr<Entity> fetch_entity(ptr<Scope> scope, string identity)
{
    auto found = fetch(scope, identity);
    if (!IS_PTR(found, Entity))
        // FIXME: Throw an appropriate error object
        throw "'" + identity + "' is not an Entity";
    return AS_PTR(found, Entity);
}
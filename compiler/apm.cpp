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

    if (IS_PTR(value, StateProperty))
        return AS_PTR(value, StateProperty)->identity;

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
    bool value_is_overloadable = IS_PTR(value, StateProperty);

    if (directly_declared_in_scope(scope, identity))
    {
        auto existing = fetch(scope, identity);
        if (IS_PTR(existing, Scope::OverloadedIdentity) && value_is_overloadable)
        {
            auto overloaded_identity = AS_PTR(existing, Scope::OverloadedIdentity);
            overloaded_identity->overloads.emplace_back(value);
        }
        else
        {
            // FIXME: Throw an appropriate error object
            throw runtime_error("Cannot declare " + identity + " in scope, as " + identity + " already exists.");
        }
    }
    else if (value_is_overloadable)
    {
        auto overloaded_identity = CREATE(Scope::OverloadedIdentity);
        overloaded_identity->identity = identity;
        overloaded_identity->overloads.emplace_back(value);
        scope->lookup.insert({identity, overloaded_identity});
    }
    else
    {
        scope->lookup.insert({identity, value});
    }
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
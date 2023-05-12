#include "apm.h"
#include "errors.h"
#include "intrinsic.h"

string identity_of(Scope::LookupValue value)
{
    if (IS_PTR(value, Variable))
        return AS_PTR(value, Variable)->identity;

    if (IS_PTR(value, IntrinsicType))
        return AS_PTR(value, IntrinsicType)->identity;

    if (IS_PTR(value, EnumType))
        return AS_PTR(value, EnumType)->identity;

    if (IS_PTR(value, Entity))
        return AS_PTR(value, Entity)->identity;

    if (IS_PTR(value, StateProperty))
        return AS_PTR(value, StateProperty)->identity;

    if (IS_PTR(value, FunctionProperty))
        return AS_PTR(value, FunctionProperty)->identity;

    throw CompilerError("Cannot get identity of Scope::LookupValue variant");
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

bool is_overloadable(Scope::LookupValue value)
{
    return IS_PTR(value, StateProperty) ||
           IS_PTR(value, FunctionProperty);
}

void declare(ptr<Scope> scope, Scope::LookupValue value)
{
    string identity = identity_of(value);

    if (directly_declared_in_scope(scope, identity))
    {
        auto existing = fetch(scope, identity);
        if (IS_PTR(existing, Scope::OverloadedIdentity) && is_overloadable(value))
        {
            auto overloaded_identity = AS_PTR(existing, Scope::OverloadedIdentity);
            overloaded_identity->overloads.emplace_back(value);
        }
        else
        {
            throw GambitError("Cannot declare " + identity + " in scope, as " + identity + " already exists.", Token()); // FIXME: Retrieve the correct token to put in the error message
        }
    }
    else if (is_overloadable(value))
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

    throw GambitError("'" + identity + "' does not exist.", Token()); // FIXME: Retrieve the correct token to put in the error message
}

vector<Scope::LookupValue> fetch_all_overloads(ptr<Scope> scope, string identity)
{
    vector<Scope::LookupValue> overloads;

    while (true)
    {
        if (directly_declared_in_scope(scope, identity))
        {
            auto fetched = scope->lookup.at(identity);
            if (IS_PTR(fetched, Scope::OverloadedIdentity))
            {
                auto overloaded_identity = AS_PTR(fetched, Scope::OverloadedIdentity);
                for (auto overload : overloaded_identity->overloads)
                    overloads.emplace_back(overload);
            }
        }

        if (scope->parent.expired())
            return overloads;

        scope = ptr<Scope>(scope->parent);
    }
}

Pattern determine_expression_pattern(Expression expression)
{
    // TODO: Complete implementation of all expression variants

    if (IS_PTR(expression, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
        throw CompilerError("Cannot determine pattern of expression before unresolved identities have been resolved.", unresolved_identity->token);
    }
    else if (IS_PTR(expression, Variable))
    {
        auto variable = AS_PTR(expression, Variable);
        return variable->pattern;
    }
    // else if (IS_PTR(expression, Literal))
    // else if (IS_PTR(expression, ListValue))
    // else if (IS_PTR(expression, InstanceList))
    // else if (IS_PTR(expression, EnumValue))
    // else if (IS_PTR(expression, Unary))
    // else if (IS_PTR(expression, Binary))
    else if (IS_PTR(expression, PropertyIndex))
    {
        auto property_index = AS_PTR(expression, PropertyIndex);
        auto property = property_index->property;
        if (IS_PTR(property, StateProperty))
            return AS_PTR(property, StateProperty)->pattern;
        if (IS_PTR(property, FunctionProperty))
            return AS_PTR(property, FunctionProperty)->pattern;
        if (IS_PTR(property, UnresolvedIdentity))
        {
            auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
            throw CompilerError("Cannot determine pattern of expression before unresolved identities have been resolved.", unresolved_identity->token);
        }

        throw CompilerError("Cannot determine pattern of Property variant in PropertyIndex expression.");
    }
    // else if (IS_PTR(expression, Match))

    throw CompilerError("Cannot determine pattern of Expression variant.");
}

bool is_pattern_subset_of_superset(Pattern subset, Pattern superset)
{
    // Cannot determine result if either pattern is invalid or unresolved
    if (
        IS_PTR(subset, UnresolvedIdentity) ||
        IS_PTR(superset, UnresolvedIdentity))
        throw CompilerError("Call to `is_pattern_subset_of_superset` has one or more unresolved identities in it's patterns");

    if (
        IS_PTR(subset, InvalidPattern) ||
        IS_PTR(superset, InvalidPattern))
        return false;

    // If patterns are the same, subset is confirmed
    if (subset == superset)
        return true;

    // Optional patterns
    // FIXME: I'm not certian that these comparisons will work correctly if multiple
    //        OptionalPattern nodes get nested inside each other. Once more complex
    //        patterns are possible, further testing and examination may be needed!

    bool subset_optional = IS_PTR(subset, OptionalPattern);
    bool superset_optional = IS_PTR(superset, OptionalPattern);

    if (subset_optional && superset_optional)
        return is_pattern_subset_of_superset(AS_PTR(subset, OptionalPattern)->pattern, AS_PTR(superset, OptionalPattern)->pattern);

    if (subset_optional && !superset_optional)
        return false;

    if (!subset_optional && superset_optional)
        return is_pattern_subset_of_superset(subset, AS_PTR(superset, OptionalPattern)->pattern);

    // Intrinsic sub types
    if (IS_PTR(subset, IntrinsicType) && IS_PTR(superset, IntrinsicType))
    {
        auto subtype = AS_PTR(subset, IntrinsicType);
        auto supertype = AS_PTR(superset, IntrinsicType);

        if (subtype == Intrinsic::type_amt && (supertype == Intrinsic::type_int ||
                                               supertype == Intrinsic::type_num))
            return true;

        if (subtype == Intrinsic::type_int && supertype == Intrinsic::type_num)
            return true;

        return false;
    }

    return false;
}

bool does_instance_list_match_parameters(ptr<InstanceList> instance_list, vector<ptr<Variable>> parameters)
{
    auto values = instance_list->values;

    if (values.size() > parameters.size())
        return false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        auto pattern = parameters[i]->pattern;

        // If there are more patterns than values, the patterns without corresponding values must be optional
        if (i >= values.size())
        {
            if (!IS_PTR(pattern, OptionalPattern))
                return false;
            continue;
        }

        auto value = values[i];
        auto value_pattern = determine_expression_pattern(value);
        if (!is_pattern_subset_of_superset(value_pattern, pattern))
            return false;
    }

    return true;
}
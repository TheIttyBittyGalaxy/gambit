#include "apm.h"
#include "errors.h"
#include "intrinsic.h"
#include "source.h"

string identity_of(Scope::LookupValue value)
{
    if (IS_PTR(value, Variable))
        return AS_PTR(value, Variable)->identity;
    if (IS_PTR(value, UnionPattern))
        return AS_PTR(value, UnionPattern)->identity;
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
    if (IS_PTR(value, Procedure))
        return AS_PTR(value, Procedure)->identity;
    if (IS_PTR(value, Scope::OverloadedIdentity))
        return AS_PTR(value, Scope::OverloadedIdentity)->identity;

    throw CompilerError("Cannot get identity of Scope::LookupValue variant", get_span(value));
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

Span get_span(Statement stmt)
{
    if (IS(stmt, Expression))
        return get_span(AS(stmt, Expression));
    if (IS_PTR(stmt, CodeBlock))
        return AS_PTR(stmt, CodeBlock)->span;
    if (IS_PTR(stmt, IfStatement))
        return AS_PTR(stmt, IfStatement)->span;
    if (IS_PTR(stmt, ForStatement))
        return AS_PTR(stmt, ForStatement)->span;
    if (IS_PTR(stmt, AssignmentStatement))
        return AS_PTR(stmt, AssignmentStatement)->span;
    if (IS_PTR(stmt, VariableDeclaration))
        return AS_PTR(stmt, VariableDeclaration)->span;

    throw CompilerError("Could not get span of Statement variant.");
}

Span get_span(Expression expr)
{
    if (IS_PTR(expr, UnresolvedIdentity))
        return AS_PTR(expr, UnresolvedIdentity)->span;
    if (IS_PTR(expr, Variable))
        return AS_PTR(expr, Variable)->span;
    if (IS_PTR(expr, EnumValue))
        return AS_PTR(expr, EnumValue)->span;
    if (IS_PTR(expr, IntrinsicValue))
        return AS_PTR(expr, IntrinsicValue)->span;
    if (IS_PTR(expr, ListValue))
        return AS_PTR(expr, ListValue)->span;
    if (IS_PTR(expr, InstanceList))
        return AS_PTR(expr, InstanceList)->span;
    if (IS_PTR(expr, Unary))
        return AS_PTR(expr, Unary)->span;
    if (IS_PTR(expr, Binary))
        return AS_PTR(expr, Binary)->span;
    if (IS_PTR(expr, ExpressionIndex))
        return AS_PTR(expr, ExpressionIndex)->span;
    if (IS_PTR(expr, PropertyIndex))
        return AS_PTR(expr, PropertyIndex)->span;
    if (IS_PTR(expr, Call))
        return AS_PTR(expr, Call)->span;
    if (IS_PTR(expr, Match))
        return AS_PTR(expr, Match)->span;
    if (IS_PTR(expr, InvalidValue))
        return AS_PTR(expr, InvalidValue)->span;
    if (IS_PTR(expr, InvalidExpression))
        return AS_PTR(expr, InvalidExpression)->span;

    throw CompilerError("Could not get span of Expression variant.");
}

Span get_span(Pattern pattern)
{
    if (IS_PTR(pattern, UnresolvedIdentity))
        return AS_PTR(pattern, UnresolvedIdentity)->span;
    if (IS_PTR(pattern, InvalidPattern))
        return AS_PTR(pattern, InvalidPattern)->span;
    if (IS_PTR(pattern, AnyPattern))
        return AS_PTR(pattern, AnyPattern)->span;
    if (IS_PTR(pattern, UnionPattern))
        return AS_PTR(pattern, UnionPattern)->span;
    if (IS_PTR(pattern, ListPattern))
        return AS_PTR(pattern, ListPattern)->span;
    if (IS_PTR(pattern, EnumType))
        return AS_PTR(pattern, EnumType)->span;
    if (IS_PTR(pattern, Entity))
        return AS_PTR(pattern, Entity)->span;
    if (IS_PTR(pattern, EnumValue))
        return AS_PTR(pattern, EnumValue)->span;
    if (IS_PTR(pattern, IntrinsicValue))
        return AS_PTR(pattern, IntrinsicValue)->span;

    if (IS_PTR(pattern, IntrinsicType))
    {
        auto intrinsic = AS_PTR(pattern, IntrinsicType);
        throw CompilerError("Attempt to get the span of an intrinsic type.");
    }

    throw CompilerError("Could not get span of Pattern variant.");
}

Span get_span(Scope::LookupValue value)
{
    if (IS_PTR(value, Variable))
        return AS_PTR(value, Variable)->span;
    if (IS_PTR(value, UnionPattern))
        return AS_PTR(value, UnionPattern)->span;
    if (IS_PTR(value, EnumType))
        return AS_PTR(value, EnumType)->span;
    if (IS_PTR(value, Entity))
        return AS_PTR(value, Entity)->span;
    if (IS_PTR(value, StateProperty))
        return AS_PTR(value, StateProperty)->span;
    if (IS_PTR(value, FunctionProperty))
        return AS_PTR(value, FunctionProperty)->span;
    if (IS_PTR(value, Procedure))
        return AS_PTR(value, Procedure)->span;

    if (IS_PTR(value, Scope::OverloadedIdentity))
        return get_span(AS_PTR(value, Scope::OverloadedIdentity)->overloads[0]); // FIXME: What span should we really use in this situation?

    if (IS_PTR(value, IntrinsicType))
        throw CompilerError("Attempt to get the span of an intrinsic type.");

    throw CompilerError("Could not get span of Scope::LookupValue variant.");
}

Scope::LookupValue fetch(ptr<Scope> scope, string identity)
{
    while (!directly_declared_in_scope(scope, identity) && !scope->parent.expired())
        scope = ptr<Scope>(scope->parent);

    if (directly_declared_in_scope(scope, identity))
        return scope->lookup.at(identity);

    throw CompilerError("Attempt to fetch LookupValue '" + identity + "' without confirming that it exists.");
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
        throw CompilerError("Cannot determine pattern of expression before unresolved identities have been resolved.", unresolved_identity->span);
    }
    else if (IS_PTR(expression, Variable))
    {
        auto variable = AS_PTR(expression, Variable);
        return variable->pattern;
    }
    else if (IS_PTR(expression, IntrinsicValue))
    {
        auto intrinsic_value = AS_PTR(expression, IntrinsicValue);
        return intrinsic_value;
    }
    // else if (IS_PTR(expression, ListValue))
    // else if (IS_PTR(expression, InstanceList))
    else if (IS_PTR(expression, EnumValue))
    {
        auto enum_value = AS_PTR(expression, EnumValue);
        return enum_value;
    }
    else if (IS_PTR(expression, Unary))
    {
        auto unary = AS_PTR(expression, Unary);
        auto op = unary->op;

        if (op == "not")
            return Intrinsic::type_bool;

        if (op == "+")
            return determine_expression_pattern(unary->value);

        // If the value is `int` or `amt`, the pattern should actually be `int`
        if (op == "-")
            return Intrinsic::type_num;
    }
    else if (IS_PTR(expression, Binary))
    {
        auto binary = AS_PTR(expression, Binary);
        auto op = binary->op;
        if (op == "==" || op == "!=" || op == "<=" || op == ">=" || op == ">" || op == "<" || op == "or" || op == "and")
            return Intrinsic::type_bool;

        // FIXME: Depending on the operation and arguments, sometimes the pattern can actually be `int` or `amt`
        if (op == "+" || op == "*" || op == "-" || op == "/")
            return Intrinsic::type_num;
    }
    else if (IS_PTR(expression, ExpressionIndex))
    {
        auto expression_index = AS_PTR(expression, ExpressionIndex);
        auto subject_pattern = determine_expression_pattern(expression_index->subject);

        if (IS_PTR(subject_pattern, ListPattern))
        {
            auto subject_list_pattern = AS_PTR(subject_pattern, ListPattern);
            return subject_list_pattern->list_of;
        }

        if (IS_PTR(subject_pattern, InvalidPattern))
        {
            return CREATE(InvalidPattern);
        }

        throw CompilerError("Cannot determine pattern of Expression Index as the subject's pattern is not a list pattern.");
    }
    else if (IS_PTR(expression, PropertyIndex))
    {
        auto property_index = AS_PTR(expression, PropertyIndex);
        auto property = property_index->property;
        if (IS_PTR(property, StateProperty))
            return AS_PTR(property, StateProperty)->pattern;
        if (IS_PTR(property, FunctionProperty))
            return AS_PTR(property, FunctionProperty)->pattern;
        if (IS_PTR(property, InvalidProperty))
            return CREATE(InvalidPattern);
        if (IS_PTR(property, UnresolvedIdentity))
        {
            auto unresolved_identity = AS_PTR(property, UnresolvedIdentity);
            throw CompilerError("Cannot determine pattern of expression before unresolved identities have been resolved.", unresolved_identity->span);
        }

        throw CompilerError("Cannot determine pattern of Property variant in PropertyIndex expression.", property_index->span);
    }
    else if (IS_PTR(expression, Call))
    {
        // TODO: Return the correct pattern
        return CREATE(AnyPattern);
    }
    else if (IS_PTR(expression, Match))
    {
        auto match = AS_PTR(expression, Match);

        // FIXME: This is a silly thing to check (given that a one rule match is
        //        useless). However, as of writing, `is_pattern_subset_of_superset`
        //        assumes that union patterns never have just one child.
        if (match->rules.size() == 1)
            return determine_expression_pattern(match->rules[0].result);

        auto union_pattern = CREATE(UnionPattern);

        for (auto rule : match->rules)
            union_pattern->patterns.push_back(determine_expression_pattern(rule.result));

        // FIXME: This union pattern is never resolved, which in turn means it is
        //        never simplified (as of writing, UnionPatterns are simplified
        //        when they are resolved)
        return union_pattern;
    }

    else if (IS_PTR(expression, InvalidExpression))
    {
        return CREATE(InvalidPattern);
    }
    else if (IS_PTR(expression, InvalidValue))
    {
        return CREATE(InvalidPattern);
    }

    throw CompilerError("Cannot determine pattern of Expression variant.", get_span(expression));
}

ptr<UnionPattern> create_union_pattern(Pattern a, Pattern b)
{
    // NOTE: This function does not simplify the resulting union
    //       e.g. remove patterns in the union that are subsets of other patterns.
    //       As of writing, this is completed when the pattern is resolved.

    auto union_pattern = CREATE(UnionPattern);

    if (IS_PTR(a, UnionPattern))
    {
        auto union_a = AS_PTR(a, UnionPattern);
        for (auto pattern : union_a->patterns)
            union_pattern->patterns.push_back(pattern);
    }
    else
    {
        union_pattern->patterns.push_back(a);
    }

    if (IS_PTR(b, UnionPattern))
    {
        auto union_b = AS_PTR(b, UnionPattern);
        for (auto pattern : union_b->patterns)
            union_pattern->patterns.push_back(pattern);
    }
    else
    {
        union_pattern->patterns.push_back(b);
    }

    return union_pattern;
}

bool is_pattern_subset_of_superset(Pattern subset, Pattern superset)
{
    // Cannot determine result if either pattern is invalid or unresolved
    // FIXME: Make clear in the error message which pattern is the UnresolvedIdentity
    if (
        IS_PTR(subset, UnresolvedIdentity) ||
        IS_PTR(superset, UnresolvedIdentity))
        throw CompilerError("Call to `is_pattern_subset_of_superset` has one or more unresolved identities in it's patterns", get_span(subset), get_span(superset));

    // TODO: For now, invalid patterns are considered to be subsets and supersets
    //       of every possible pattern. I'm not sure if this is the correct
    //       assumption. We're going to roll with it though while compiler matures.
    if (
        IS_PTR(subset, InvalidPattern) ||
        IS_PTR(superset, InvalidPattern))
        return true;

    // Any pattern
    if (IS_PTR(superset, AnyPattern))
        return true;

    if (IS_PTR(subset, AnyPattern))
        return false;

    // If patterns are the same, subset is confirmed
    if (subset == superset)
        return true;

    // List patterns
    if (IS_PTR(subset, ListPattern) && IS_PTR(superset, ListPattern))
        return is_pattern_subset_of_superset(AS_PTR(subset, ListPattern)->list_of, AS_PTR(superset, ListPattern)->list_of);

    // Union patterns
    bool subset_is_union = IS_PTR(subset, UnionPattern);
    bool superset_is_union = IS_PTR(superset, UnionPattern);

    if (!subset_is_union && superset_is_union)
    {
        auto super_union = AS_PTR(superset, UnionPattern);
        for (auto pattern : super_union->patterns)
        {
            if (is_pattern_subset_of_superset(subset, pattern))
                return true;
        }
        return false;
    }

    if (subset_is_union && !superset_is_union)
    {
        // NOTE: Technically, if the subset is a union with only one pattern in the union,
        //       and that one pattern is a subset of the superset, in that case we should
        //       return true. However, we are returning false by default under the
        //       assumption that the resolver will always simplify UnionPattern nodes with
        //       only one pattern to just the pattern itself.

        // FIXME: Check that this assumption about the resolver is correct.

        // FIXME: Is it possible that parts of the program may try to use this function on
        //        patterns that never get resolved? e.g. a caller could call
        //        `determine_expression_pattern` on an expression to do some pattern checking,
        //        meaning the result was never resolved as it was never part of the APM.
        //        Presumably that caller would then call `is_pattern_subset_of_superset` to
        //        do the check?

        return false;
    }

    if (subset_is_union && superset_is_union)
    {
        // In this case, every pattern in the sub union needs to be a subset
        // of at least one pattern in the super union.
        auto sub_union = AS_PTR(subset, UnionPattern);
        auto super_union = AS_PTR(superset, UnionPattern);

        for (auto sub_pattern : sub_union->patterns)
        {
            bool sub_pattern_is_subset = false;
            for (auto super_pattern : super_union->patterns)
            {
                if (is_pattern_subset_of_superset(sub_pattern, super_pattern))
                {
                    sub_pattern_is_subset = true;
                    break;
                }
            }

            if (!sub_pattern_is_subset)
                return false;
        }

        return true;
    }

    // Enums
    if (IS_PTR(subset, EnumValue) && IS_PTR(superset, EnumType))
    {
        auto enum_type = AS_PTR(superset, EnumType);
        auto enum_value = AS_PTR(subset, EnumValue);
        for (auto value : enum_type->values)
            if (value == enum_value)
                return true;
        return false;
    }

    // Intrinsic types
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

    // Intrinsic values
    if (IS_PTR(subset, IntrinsicValue) && IS_PTR(superset, IntrinsicType))
    {
        auto sub_value = AS_PTR(subset, IntrinsicValue);
        auto super_type = AS_PTR(superset, IntrinsicType);
        return is_pattern_subset_of_superset(sub_value->type, super_type);
    }

    if (IS_PTR(subset, IntrinsicValue) && IS_PTR(superset, IntrinsicValue))
    {
        auto sub_value = AS_PTR(subset, IntrinsicValue);
        auto super_value = AS_PTR(superset, IntrinsicValue);
        return is_pattern_subset_of_superset(sub_value->type, super_value->type) &&
               sub_value->value == super_value->value;
    }

    // None edge case
    if (IS_PTR(subset, IntrinsicType) && IS_PTR(superset, IntrinsicValue))
    {
        // NOTE: I don't we will ever _actually_ hit this code path, as the intrinsic type
        //       only really exists so that the intrinsic value has something to point to.
        //       If we do hit this code path, it might be worth exploring why the intrinsic
        //       value wasn't used instead.

        auto sub_type = AS_PTR(subset, IntrinsicType);
        auto super_value = AS_PTR(superset, IntrinsicValue);

        if (sub_type == Intrinsic::type_none && super_value == Intrinsic::none_val)
            return true;
    }

    return false;
}

// FIXME: This is a terribly inefficient way of doing this!
//        Figure out a sensible algorithm for this.
bool do_patterns_overlap(Pattern a, Pattern b)
{
    return is_pattern_subset_of_superset(a, b) || is_pattern_subset_of_superset(b, a);
}

bool is_pattern_optional(Pattern pattern)
{
    if (IS_PTR(pattern, IntrinsicValue))
    {
        auto intrinsic_value = AS_PTR(pattern, IntrinsicValue);
        return intrinsic_value == Intrinsic::none_val;
    }

    if (IS_PTR(pattern, IntrinsicType))
    {
        // NOTE: I don't we will ever _actually_ hit this code path, as the intrinsic type
        //       only really exists so that the intrinsic value has something to point to.
        //       If we do hit this code path, it might be worth exploring why the intrinsic
        //       value wasn't used instead.

        auto intrinsic_type = AS_PTR(pattern, IntrinsicType);
        return intrinsic_type == Intrinsic::type_none;
    }

    if (IS_PTR(pattern, UnionPattern))
    {
        auto union_pattern = AS_PTR(pattern, UnionPattern);
        for (auto sub_pattern : union_pattern->patterns)
            if (is_pattern_optional(sub_pattern))
                return true;
        return false;
    }

    if (IS_PTR(pattern, AnyPattern))
        return true;

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
            if (!is_pattern_optional(pattern))
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
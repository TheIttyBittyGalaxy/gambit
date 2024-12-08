#include "apm.h"
#include "errors.h"
#include "intrinsic.h"
#include "source.h"

// DECLARATION AND FETCHING

string identity_of(Scope::LookupValue value)
{
    if (IS_PTR(value, Scope::OverloadedIdentity))
        return AS_PTR(value, Scope::OverloadedIdentity)->identity;
    if (IS_PTR(value, Procedure))
        return AS_PTR(value, Procedure)->identity;
    if (IS_PTR(value, Variable))
        return AS_PTR(value, Variable)->identity;

    if (IS_PTR(value, StateProperty))
        return AS_PTR(value, StateProperty)->identity;
    if (IS_PTR(value, FunctionProperty))
        return AS_PTR(value, FunctionProperty)->identity;

    if (IS(value, Pattern))
    {
        auto pattern = AS(value, Pattern);

        if (IS_PTR(pattern, PrimitiveType))
            return AS_PTR(pattern, PrimitiveType)->identity;
        if (IS_PTR(pattern, EnumType))
            return AS_PTR(pattern, EnumType)->identity;
        if (IS_PTR(pattern, EntityType))
            return AS_PTR(pattern, EntityType)->identity;
        if (IS_PTR(pattern, UnionPattern))
            return AS_PTR(pattern, UnionPattern)->identity;

        throw CompilerError("Cannot get identity of Scope::LookupValue Pattern variant", get_span(value));
    }

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

// PATTERN ANALYSIS

Pattern determine_expression_pattern(Expression expression)
{

    // Literals
    if (IS(expression, UnresolvedLiteral))
    {
        auto unresolved_literal = AS(expression, UnresolvedLiteral);
        throw CompilerError("Cannot determine pattern of expression before identities have been resolved.", get_span(unresolved_literal));
    }

    if (IS_PTR(expression, ExpressionLiteral))
    {
        return determine_expression_pattern(AS_PTR(expression, ExpressionLiteral)->expr);
    }

    // Values
    if (IS_PTR(expression, PrimitiveValue))
    {
        return AS_PTR(expression, PrimitiveValue);
    }

    if (IS_PTR(expression, ListValue))
    {
        auto list_value AS_PTR(expression, ListValue);
        auto union_pattern = CREATE(UnionPattern);

        for (auto value : list_value->values)
            union_pattern->patterns.push_back(determine_expression_pattern(value));

        // FIXME: This union pattern is never resolved, which in turn means it is
        //        never simplified (as of writing, UnionPatterns are simplified
        //        when they are resolved)
        return union_pattern;
    }

    if (IS_PTR(expression, EnumValue))
    {
        return AS_PTR(expression, EnumValue);
    }

    if (IS_PTR(expression, Variable))
    {
        auto variable = AS_PTR(expression, Variable);
        return variable->pattern;
    }

    // Operations
    if (IS_PTR(expression, Unary))
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

    if (IS_PTR(expression, Binary))
    {
        auto binary = AS_PTR(expression, Binary);
        auto op = binary->op;
        if (op == "==" || op == "!=" || op == "<=" || op == ">=" || op == ">" || op == "<" || op == "or" || op == "and")
            return Intrinsic::type_bool;

        // FIXME: Depending on the operation and arguments, sometimes the pattern can actually be `int` or `amt`
        if (op == "+" || op == "*" || op == "-" || op == "/")
            return Intrinsic::type_num;
    }

    // Indexing
    if (IS_PTR(expression, ExpressionIndex))
    {
        auto expression_index = AS_PTR(expression, ExpressionIndex);
        auto subject_pattern = determine_expression_pattern(expression_index->subject);

        if (IS_PTR(subject_pattern, ListType))
        {
            auto subject_list_type = AS_PTR(subject_pattern, ListType);
            return subject_list_type->list_of;
        }

        if (IS_PTR(subject_pattern, InvalidPattern))
        {
            return CREATE(InvalidPattern);
        }

        // TODO: Correctly determine the pattern of other subject expressions

        return CREATE(AnyPattern);
        // throw CompilerError("Cannot determine pattern of Expression Index as the subject's pattern is not a list pattern.");
    }

    if (IS_PTR(expression, PropertyIndex))
    {
        auto property_index = AS_PTR(expression, PropertyIndex);
        auto property = property_index->property;
        if (IS_PTR(property, StateProperty))
            return AS_PTR(property, StateProperty)->pattern;
        if (IS_PTR(property, FunctionProperty))
            return AS_PTR(property, FunctionProperty)->pattern;
        if (IS_PTR(property, InvalidProperty))
            return CREATE(InvalidPattern);
        if (IS_PTR(property, IdentityLiteral))
        {
            auto identity_literal = AS_PTR(property, IdentityLiteral);
            throw CompilerError("Cannot determine pattern of expression before identities have been resolved.", identity_literal->span);
        }

        throw CompilerError("Cannot determine pattern of Property variant in PropertyIndex expression.", property_index->span);
    }

    // Calls
    if (IS_PTR(expression, Call))
    {
        // TODO: Return the correct pattern
        return CREATE(AnyPattern);
    }

    // "Statements style" expressions
    if (IS_PTR(expression, IfExpression))
    {
        auto if_expression = AS_PTR(expression, IfExpression);

        // FIXME: This is a silly thing to check (given that a one rule if is
        //        useless). However, as of writing, `is_pattern_subset_of_superset`
        //        assumes that union patterns never have just one child.
        if (if_expression->rules.size() == 1)
            return determine_expression_pattern(if_expression->rules[0].result);

        auto union_pattern = CREATE(UnionPattern);

        for (auto rule : if_expression->rules)
            union_pattern->patterns.push_back(determine_expression_pattern(rule.result));

        // FIXME: This union pattern is never resolved, which in turn means it is
        //        never simplified (as of writing, UnionPatterns are simplified
        //        when they are resolved)
        return union_pattern;
    }

    if (IS_PTR(expression, MatchExpression))
    {
        auto match = AS_PTR(expression, MatchExpression);

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

    // Invalid expression
    if (IS_PTR(expression, InvalidExpression))
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
    // Cannot determine result if either pattern is an unresolved literal
    // FIXME: Make clear in the error message which pattern is the UnresolvedLiteral
    if (
        IS(subset, UnresolvedLiteral) ||
        IS(superset, UnresolvedLiteral))
        throw CompilerError("Call to `is_pattern_subset_of_superset` has one or more unresolved literals in it's patterns", get_span(subset), get_span(superset));

    // Unwrap PatternLiteral nodes
    bool subset_is_literal = IS_PTR(subset, PatternLiteral);
    bool superset_is_literal = IS_PTR(superset, PatternLiteral);
    if (subset_is_literal && superset_is_literal)
        return is_pattern_subset_of_superset(AS_PTR(subset, PatternLiteral)->pattern, AS_PTR(superset, PatternLiteral)->pattern);

    if (subset_is_literal)
        return is_pattern_subset_of_superset(AS_PTR(subset, PatternLiteral)->pattern, superset);

    if (superset_is_literal)
        return is_pattern_subset_of_superset(subset, AS_PTR(superset, PatternLiteral)->pattern);

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

    // List types
    if (IS_PTR(subset, ListType) && IS_PTR(superset, ListType))
        return is_pattern_subset_of_superset(AS_PTR(subset, ListType)->list_of, AS_PTR(superset, ListType)->list_of);

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
    if (IS_PTR(subset, PrimitiveType) && IS_PTR(superset, PrimitiveType))
    {
        auto subtype = AS_PTR(subset, PrimitiveType);
        auto supertype = AS_PTR(superset, PrimitiveType);

        if (subtype == Intrinsic::type_amt && (supertype == Intrinsic::type_int ||
                                               supertype == Intrinsic::type_num))
            return true;

        if (subtype == Intrinsic::type_int && supertype == Intrinsic::type_num)
            return true;

        return false;
    }

    // Intrinsic values
    if (IS_PTR(subset, PrimitiveValue) && IS_PTR(superset, PrimitiveType))
    {
        auto sub_value = AS_PTR(subset, PrimitiveValue);
        auto super_type = AS_PTR(superset, PrimitiveType);
        return is_pattern_subset_of_superset(sub_value->type, super_type);
    }

    if (IS_PTR(subset, PrimitiveValue) && IS_PTR(superset, PrimitiveValue))
    {
        auto sub_value = AS_PTR(subset, PrimitiveValue);
        auto super_value = AS_PTR(superset, PrimitiveValue);
        return is_pattern_subset_of_superset(sub_value->type, super_value->type) &&
               sub_value->value == super_value->value;
    }

    // None edge case
    if (IS_PTR(subset, PrimitiveType) && IS_PTR(superset, PrimitiveValue))
    {
        // NOTE: I don't we will ever _actually_ hit this code path, as the intrinsic type
        //       only really exists so that the intrinsic value has something to point to.
        //       If we do hit this code path, it might be worth exploring why the intrinsic
        //       value wasn't used instead.

        auto sub_type = AS_PTR(subset, PrimitiveType);
        auto super_value = AS_PTR(superset, PrimitiveValue);

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
    if (IS_PTR(pattern, PrimitiveValue))
    {
        return AS_PTR(pattern, PrimitiveValue) == Intrinsic::none_val;
    }

    if (IS_PTR(pattern, PrimitiveType))
    {
        // NOTE: I don't we will ever _actually_ hit this code path, as the intrinsic type
        //       only really exists so that the intrinsic value has something to point to.
        //       If we do hit this code path, it might be worth exploring why the intrinsic
        //       value wasn't used instead.
        return AS_PTR(pattern, PrimitiveType) == Intrinsic::type_none;
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

// SPANS

Span get_span(UnresolvedLiteral stmt)
{
    if (IS_PTR(stmt, PrimitiveLiteral))
        return AS_PTR(stmt, PrimitiveLiteral)->span;
    if (IS_PTR(stmt, ListLiteral))
        return AS_PTR(stmt, ListLiteral)->span;
    if (IS_PTR(stmt, IdentityLiteral))
        return AS_PTR(stmt, IdentityLiteral)->span;
    if (IS_PTR(stmt, OptionLiteral))
        return AS_PTR(stmt, OptionLiteral)->span;

    throw CompilerError("Could not get span of UnresolvedLiteral variant.");
}

Span get_span(Pattern pattern)
{
    if (IS(pattern, UnresolvedLiteral))
        return get_span(AS(pattern, UnresolvedLiteral));
    if (IS_PTR(pattern, PatternLiteral))
        return AS_PTR(pattern, PatternLiteral)->span;

    // if (IS_PTR(pattern, AnyPattern))
    //     return AS_PTR(pattern, AnyPattern)->span;

    // if (IS_PTR(pattern, UnionPattern))
    //     return AS_PTR(pattern, UnionPattern)->span;

    // if (IS_PTR(pattern, PrimitiveValue))
    //     return AS_PTR(pattern, PrimitiveValue)->span;
    if (IS_PTR(pattern, EnumValue))
        return AS_PTR(pattern, EnumValue)->span;

    // if (IS_PTR(pattern, PrimitiveType))
    //     return AS_PTR(pattern, PrimitiveType)->span;
    // if (IS_PTR(pattern, ListType))
    //     return AS_PTR(pattern, ListType)->span;
    if (IS_PTR(pattern, EnumType))
        return AS_PTR(pattern, EnumType)->span;
    if (IS_PTR(pattern, EntityType))
        return AS_PTR(pattern, EntityType)->span;

    // if (IS_PTR(pattern, UninferredPattern))
    //     return AS_PTR(pattern, UninferredPattern)->span;

    // if (IS_PTR(pattern, InvalidPattern))
    //     return AS_PTR(pattern, InvalidPattern)->span;

    throw CompilerError("Could not get span of Pattern variant.");
}

Span get_span(Expression expr)
{
    if (IS(expr, UnresolvedLiteral))
        return get_span(AS(expr, UnresolvedLiteral));
    if (IS_PTR(expr, ExpressionLiteral))
        return AS_PTR(expr, ExpressionLiteral)->span;

    // if (IS_PTR(expr, PrimitiveValue))
    //     return AS_PTR(expr, PrimitiveValue)->span;
    // if (IS_PTR(expr, ListValue))
    //     return AS_PTR(expr, ListValue)->span;
    if (IS_PTR(expr, EnumValue))
        return AS_PTR(expr, EnumValue)->span;
    if (IS_PTR(expr, Variable))
        return AS_PTR(expr, Variable)->span;

    if (IS_PTR(expr, Unary))
        return AS_PTR(expr, Unary)->span;
    if (IS_PTR(expr, Binary))
        return AS_PTR(expr, Binary)->span;

    if (IS_PTR(expr, InstanceList))
        return AS_PTR(expr, InstanceList)->span;
    if (IS_PTR(expr, ExpressionIndex))
        return AS_PTR(expr, ExpressionIndex)->span;
    if (IS_PTR(expr, PropertyIndex))
        return AS_PTR(expr, PropertyIndex)->span;

    if (IS_PTR(expr, Call))
        return AS_PTR(expr, Call)->span;

    if (IS_PTR(expr, IfExpression))
        return AS_PTR(expr, IfExpression)->span;
    if (IS_PTR(expr, MatchExpression))
        return AS_PTR(expr, MatchExpression)->span;

    // if (IS_PTR(expr, InvalidExpression))
    //     return AS_PTR(expr, InvalidExpression)->span;

    throw CompilerError("Could not get span of Expression variant.");
}

Span get_span(Statement stmt)
{
    if (IS_PTR(stmt, IfStatement))
        return AS_PTR(stmt, IfStatement)->span;
    if (IS_PTR(stmt, ForStatement))
        return AS_PTR(stmt, ForStatement)->span;
    if (IS_PTR(stmt, LoopStatement))
        return AS_PTR(stmt, LoopStatement)->span;
    if (IS_PTR(stmt, ReturnStatement))
        return AS_PTR(stmt, ReturnStatement)->span;
    if (IS_PTR(stmt, AssignmentStatement))
        return AS_PTR(stmt, AssignmentStatement)->span;
    if (IS_PTR(stmt, VariableDeclaration))
        return AS_PTR(stmt, VariableDeclaration)->span;

    if (IS_PTR(stmt, CodeBlock))
        return AS_PTR(stmt, CodeBlock)->span;

    if (IS(stmt, Expression))
        return get_span(AS(stmt, Expression));

    throw CompilerError("Could not get span of Statement variant.");
}

Span get_span(Scope::LookupValue value)
{
    if (IS_PTR(value, Scope::OverloadedIdentity))
        return get_span(AS_PTR(value, Scope::OverloadedIdentity)->overloads[0]); // FIXME: What span should we really use in this situation?
    if (IS_PTR(value, Variable))
        return AS_PTR(value, Variable)->span;
    if (IS_PTR(value, Procedure))
        return AS_PTR(value, Procedure)->span;

    if (IS_PTR(value, StateProperty))
        return AS_PTR(value, StateProperty)->span;
    if (IS_PTR(value, FunctionProperty))
        return AS_PTR(value, FunctionProperty)->span;

    if (IS(value, Pattern))
        return get_span(AS(value, Pattern));

    throw CompilerError("Could not get span of Scope::LookupValue variant.");
}
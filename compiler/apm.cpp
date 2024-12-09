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
        auto list_value = AS_PTR(expression, ListValue);

        vector<Pattern> list_value_patterns;
        for (auto value : list_value->values)
            list_value_patterns.push_back(determine_expression_pattern(value));

        // FIXME: This union pattern is never resolved, which in turn means it is
        //        never simplified (as of writing, UnionPatterns are simplified
        //        when they are resolved)

        auto list_type = CREATE(ListType);
        list_type->list_of = create_union_pattern(list_value_patterns);

        auto fixed_size = CREATE(PrimitiveValue);
        fixed_size->type = Intrinsic::type_int;
        fixed_size->value = (int)list_value->values.size();
        list_type->fixed_size = fixed_size;

        return list_type;
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
    if (IS_PTR(expression, IndexWithExpression))
    {
        auto index_with_expression = AS_PTR(expression, IndexWithExpression);
        auto subject_pattern = determine_expression_pattern(index_with_expression->subject);
        return determine_pattern_of_contents_of(subject_pattern);
    }

    if (IS_PTR(expression, IndexWithIdentity))
    {
        // NOTE: As of writing, the resolver converts all IndexWithIdentity nodes into either a PropertyAccess or an enum value.
        //       This means an IndexWithIdentity should never be passed into this function. However, assuming that at some point
        //       it becomes possible to have indexes by identity which need to be resolved at run-time but are not property accesses,
        //       we will need to support this.
        auto index_with_expression = AS_PTR(expression, IndexWithIdentity);
        throw CompilerError("Cannot determine pattern of expression before identities have been resolved.", index_with_expression->span);
    }

    // Calls
    if (IS_PTR(expression, Call))
    {
        // TODO: Return the correct pattern
        return CREATE(AnyPattern);
    }

    if (IS_PTR(expression, PropertyAccess))
    {
        auto property_access = AS_PTR(expression, PropertyAccess);
        auto property = property_access->property;
        if (IS_PTR(property, StateProperty))
            return AS_PTR(property, StateProperty)->pattern;
        if (IS_PTR(property, FunctionProperty))
            return AS_PTR(property, FunctionProperty)->pattern;
        if (IS_PTR(property, InvalidProperty))
            return CREATE(InvalidPattern);
        if (IS_PTR(property, IdentityLiteral))
        {
            auto identity_literal = AS_PTR(property, IdentityLiteral);
            throw CompilerError("Cannot determine pattern of PropertyAccess expression before identities have been resolved.", identity_literal->span);
        }

        throw CompilerError("Cannot determine pattern of Property variant in PropertyAccess expression.", property_access->span);
    }

    // Choose expression
    if (IS_PTR(expression, ChooseExpression))
    {
        auto choose_expression = AS_PTR(expression, ChooseExpression);
        auto choices_pattern = determine_expression_pattern(choose_expression->choices);
        return determine_pattern_of_contents_of(choices_pattern);
    }

    // "Statements style" expressions
    if (IS_PTR(expression, IfExpression))
    {
        auto if_expression = AS_PTR(expression, IfExpression);

        vector<Pattern> rule_result_patterns;
        for (auto rule : if_expression->rules)
            rule_result_patterns.push_back(determine_expression_pattern(rule.result));

        return create_union_pattern(rule_result_patterns);
    }

    if (IS_PTR(expression, MatchExpression))
    {
        auto match = AS_PTR(expression, MatchExpression);

        vector<Pattern> rule_result_patterns;
        for (auto rule : match->rules)
            rule_result_patterns.push_back(determine_expression_pattern(rule.result));

        return create_union_pattern(rule_result_patterns);
    }

    // Invalid expression
    if (IS_PTR(expression, InvalidExpression))
    {
        return CREATE(InvalidPattern);
    }

    throw CompilerError("Cannot determine pattern of Expression variant.", get_span(expression));
}

Pattern determine_pattern_of_contents_of(Pattern pattern)
{
    // Literals
    if (IS(pattern, UnresolvedLiteral))
    {
        auto unresolved_literal = AS(pattern, UnresolvedLiteral);
        throw CompilerError("Cannot determine pattern of pattern's contents before said pattern has been resolved.", get_span(unresolved_literal));
    }

    if (IS_PTR(pattern, PatternLiteral))
    {
        return determine_pattern_of_contents_of(AS_PTR(pattern, PatternLiteral)->pattern);
    }

    // List types
    if (IS_PTR(pattern, ListType))
    {
        return AS_PTR(pattern, ListType)->list_of;
    }

    // Invalid pattern
    if (IS_PTR(pattern, InvalidPattern))
    {
        return CREATE(InvalidPattern);
    }

    throw CompilerError("Cannot determine pattern of pattern's contents as said pattern is not a list type.");
}

Pattern create_union_pattern(vector<Pattern> patterns)
{
    vector<Pattern> reduced_patterns;
    for (size_t i = 0; i < patterns.size(); i++)
    {
        auto pattern = patterns[i];
        bool pattern_is_superset = true;

        for (size_t j = i + 1; j < patterns.size(); j++)
        {
            auto other = patterns[j];
            if (is_pattern_subset_of_superset(pattern, other))
            {
                pattern_is_superset = false;
                break;
            }
        }

        if (pattern_is_superset)
            reduced_patterns.push_back(pattern);
    }

    if (reduced_patterns.size() == 1)
        return reduced_patterns[0];

    auto union_pattern = CREATE(UnionPattern);
    union_pattern->patterns = reduced_patterns;
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
        auto sub_union = AS_PTR(subset, UnionPattern);
        for (auto pattern : sub_union->patterns)
        {
            if (!is_pattern_subset_of_superset(pattern, superset))
                return false;
        }
        return true;
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
    if (IS_PTR(pattern, PatternLiteral))
    {
        return is_pattern_optional(AS_PTR(pattern, PatternLiteral)->pattern);
    }

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

Span get_span(UnresolvedLiteral literal)
{
    if (IS_PTR(literal, PrimitiveLiteral))
        return AS_PTR(literal, PrimitiveLiteral)->span;
    if (IS_PTR(literal, ListLiteral))
        return AS_PTR(literal, ListLiteral)->span;
    if (IS_PTR(literal, IdentityLiteral))
        return AS_PTR(literal, IdentityLiteral)->span;
    if (IS_PTR(literal, OptionLiteral))
        return AS_PTR(literal, OptionLiteral)->span;

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
    if (IS_PTR(expr, IndexWithExpression))
        return AS_PTR(expr, IndexWithExpression)->span;
    if (IS_PTR(expr, IndexWithIdentity))
        return AS_PTR(expr, IndexWithIdentity)->span;

    if (IS_PTR(expr, Call))
        return AS_PTR(expr, Call)->span;
    if (IS_PTR(expr, PropertyAccess))
        return AS_PTR(expr, PropertyAccess)->span;

    if (IS_PTR(expr, ChooseExpression))
        return AS_PTR(expr, ChooseExpression)->span;

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
    if (IS_PTR(stmt, WinsStatement))
        return AS_PTR(stmt, WinsStatement)->span;
    if (IS_PTR(stmt, DrawStatement))
        return AS_PTR(stmt, DrawStatement)->span;
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
#include "errors.h"
#include "intrinsic.h"
#include "resolver.h"
#include "source.h"
#include <optional>

void Resolver::resolve(Source &source, ptr<Program> program)
{
    this->source = &source;
    resolve_program(program);
}

// PROGRAM STRUCTURE //

void Resolver::resolve_program(ptr<Program> program)
{
    resolve_scope(program->global_scope);
}

void Resolver::resolve_scope(ptr<Scope> scope)
{
    for (auto index : scope->lookup)
    {
        auto value = index.second;
        if (IS(value, Pattern))
        {
            auto new_pattern = resolve_pattern(AS(value, Pattern), scope);
            scope->lookup.insert_or_assign(index.first, new_pattern);
        }
    }

    // Property signatures need to be resolved before property and procedure bodies so that
    // PropertyIndex nodes can correctly resolve which overload of the property they should use.
    for (auto index : scope->lookup)
        resolve_scope_lookup_value_property_signatures_pass(index.second, scope);

    for (auto index : scope->lookup)
        resolve_scope_lookup_value_final_pass(index.second, scope);
}

void Resolver::resolve_scope_lookup_value_property_signatures_pass(Scope::LookupValue value, ptr<Scope> scope)
{
    if (IS_PTR(value, StateProperty))
    {
        auto state = AS_PTR(value, StateProperty);
        state->pattern = resolve_pattern(state->pattern, scope);
        resolve_scope(state->scope); // This will resolve the parameters
    }

    else if (IS_PTR(value, FunctionProperty))
    {
        auto funct = AS_PTR(value, FunctionProperty);
        funct->pattern = resolve_pattern(funct->pattern, scope);
        resolve_scope(funct->scope); // This will resolve the parameters
    }

    else if (IS_PTR(value, Scope::OverloadedIdentity))
    {
        auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
        for (auto overload : overloaded_identity->overloads)
            resolve_scope_lookup_value_property_signatures_pass(overload, scope);
    }
}

void Resolver::resolve_scope_lookup_value_final_pass(Scope::LookupValue value, ptr<Scope> scope)
{
    if (IS_PTR(value, Variable))
    {
        auto variable = AS_PTR(value, Variable);
        if (!IS_PTR(variable->pattern, UninferredPattern))
            variable->pattern = resolve_pattern(variable->pattern, scope);
    }

    else if (IS_PTR(value, StateProperty))
    {
        auto state = AS_PTR(value, StateProperty);
        if (state->initial_value.has_value())
            state->initial_value = resolve_expression(state->initial_value.value(), state->scope, state->pattern);
    }

    else if (IS_PTR(value, FunctionProperty))
    {
        auto funct = AS_PTR(value, FunctionProperty);
        if (funct->body.has_value())
            resolve_code_block(funct->body.value(), funct->pattern);

        // FIXME: If the body is a singleton, resolve the statement as if it were a return expression
    }

    else if (IS_PTR(value, Procedure))
    {
        auto funct = AS_PTR(value, Procedure);
        resolve_code_block(funct->body);

        // FIXME: If the body is a singleton, resolve the statement as if it were a return expression
    }

    else if (IS_PTR(value, Scope::OverloadedIdentity))
    {
        auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
        for (auto overload : overloaded_identity->overloads)
            resolve_scope_lookup_value_final_pass(overload, scope);
    }
}

void Resolver::resolve_code_block(ptr<CodeBlock> code_block, optional<Pattern> pattern_hint)
{
    resolve_scope(code_block->scope);

    if (code_block->singleton_block)
    {
        // Unlike in regular code blocks, the statement of a singleton code block is given the pattern hint.
        // Codeblock may not contain a statements if an error occurred while parsing the statement.
        if (code_block->statements.size() > 0)
        {
            auto stmt = code_block->statements[0];
            code_block->statements[0] = resolve_statement(stmt, code_block->scope, pattern_hint);
        }
    }
    else
    {
        for (size_t i = 0; i < code_block->statements.size(); i++)
        {
            auto stmt = code_block->statements[i];
            code_block->statements[i] = resolve_statement(stmt, code_block->scope);
        }
    }
}

// STATEMENTS //

Statement Resolver::resolve_statement(Statement stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    if (IS(stmt, Expression))
        return resolve_expression(AS(stmt, Expression), scope, pattern_hint);

    else if (IS_PTR(stmt, CodeBlock))
        resolve_code_block(AS_PTR(stmt, CodeBlock), pattern_hint);

    else if (IS_PTR(stmt, IfStatement))
        resolve_if_statement(AS_PTR(stmt, IfStatement), scope, pattern_hint);

    else if (IS_PTR(stmt, ForStatement))
        resolve_for_statement(AS_PTR(stmt, ForStatement), scope, pattern_hint);

    else if (IS_PTR(stmt, AssignmentStatement))
        resolve_assignment_statement(AS_PTR(stmt, AssignmentStatement), scope);

    else if (IS_PTR(stmt, VariableDeclaration))
        resolve_variable_declaration(AS_PTR(stmt, VariableDeclaration), scope);

    else
        throw CompilerError("Cannot resolve Statement variant.", get_span(stmt));

    return stmt;
}

void Resolver::resolve_if_statement(ptr<IfStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (auto rule : stmt->rules)
    {
        rule.condition = resolve_expression(rule.condition, scope);
        resolve_code_block(rule.code_block, pattern_hint);
    }

    if (stmt->else_block.has_value())
        resolve_code_block(stmt->else_block.value(), pattern_hint);
}

void Resolver::resolve_for_statement(ptr<ForStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    stmt->range = resolve_expression(stmt->range, scope);
    stmt->variable->pattern = determine_expression_pattern(stmt->range);
    resolve_code_block(stmt->body);
}

void Resolver::resolve_assignment_statement(ptr<AssignmentStatement> stmt, ptr<Scope> scope)
{
    stmt->subject = resolve_expression(stmt->subject, scope);
    auto subject_pattern = determine_expression_pattern(stmt->subject);
    stmt->value = resolve_expression(stmt->value, scope, subject_pattern);
}

void Resolver::resolve_variable_declaration(ptr<VariableDeclaration> stmt, ptr<Scope> scope)
{
    if (stmt->value.has_value())
        stmt->value = resolve_expression(stmt->value.value(), scope, stmt->variable->pattern);

    if (IS_PTR(stmt->variable->pattern, UninferredPattern))
    {
        if (stmt->value.has_value())
            stmt->variable->pattern = determine_expression_pattern(stmt->value.value());
        else
            stmt->variable->pattern = CREATE(AnyPattern);
    }
}

// EXPRESSIONS //

Expression Resolver::resolve_expression(Expression expression, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    if (IS(expression, UnresolvedLiteral))
        return resolve_literal_as_expression(AS(expression, UnresolvedLiteral), scope, pattern_hint);

    else if (IS_PTR(expression, ListValue))
        resolve_list_value(AS_PTR(expression, ListValue), scope, pattern_hint);

    else if (IS_PTR(expression, InstanceList))
        resolve_instance_list(AS_PTR(expression, InstanceList), scope, pattern_hint);

    else if (IS_PTR(expression, Unary))
        resolve_unary(AS_PTR(expression, Unary), scope, pattern_hint);

    else if (IS_PTR(expression, Binary))
        resolve_binary(AS_PTR(expression, Binary), scope, pattern_hint);

    else if (IS_PTR(expression, ExpressionIndex))
        resolve_expression_index(AS_PTR(expression, ExpressionIndex), scope, pattern_hint);

    else if (IS_PTR(expression, PropertyIndex))
        resolve_property_index(AS_PTR(expression, PropertyIndex), scope, pattern_hint);

    else if (IS_PTR(expression, Call))
        resolve_call(AS_PTR(expression, Call), scope, pattern_hint);

    else if (IS_PTR(expression, IfExpression))
        resolve_if_expression(AS_PTR(expression, IfExpression), scope, pattern_hint);

    else if (IS_PTR(expression, MatchExpression))
        resolve_match(AS_PTR(expression, MatchExpression), scope, pattern_hint);

    return expression;
}

ptr<ExpressionLiteral> Resolver::resolve_literal_as_expression(UnresolvedLiteral unresolved_literal, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    auto expression_literal = CREATE(ExpressionLiteral);
    expression_literal->span = get_span(unresolved_literal);

    // Primitive literal
    if (IS_PTR(unresolved_literal, PrimitiveLiteral))
    {
        auto primitive_literal = AS_PTR(unresolved_literal, PrimitiveLiteral);

        // NOTE: We do not attempt to resolve `primitive_literal->value`, as this value
        //       will always be a PrimitiveValue, which has no need to be resolved.
        expression_literal->expr = primitive_literal->value;
    }

    // List literal
    else if (IS_PTR(unresolved_literal, ListLiteral))
    {
        auto list_literal = AS_PTR(unresolved_literal, ListLiteral);
        auto list_value = CREATE(ListValue);

        list_value->values.reserve(list_literal->values.size());
        for (size_t i = 0; i < list_literal->values.size(); i++)
            list_value->values.emplace_back(resolve_expression(list_literal->values[i], scope, pattern_hint));

        expression_literal->expr = list_value;
    }

    // Identity literals
    else if (IS_PTR(unresolved_literal, IdentityLiteral))
    {
        auto identity_literal = AS_PTR(unresolved_literal, IdentityLiteral);
        auto identity = identity_literal->identity;

        optional<Expression> expr;

        // Resolve identity by searching for it in the scope
        if (declared_in_scope(scope, identity))
        {
            auto resolved = fetch(scope, identity);

            if (IS_PTR(resolved, Variable))
            {
                expr = AS_PTR(resolved, Variable);
            }
            else
            {
                // FIXME: Make error more informative by saying _what_ the resolved object is (e.g. an entity, a type, etc)
                source->log_error("Expected value, got '" + identity + "'", identity_literal->span);
                expr = CREATE(InvalidExpression);
            }
        }

        // Resolve identity using the pattern hint
        else if (pattern_hint.has_value())
        {
            auto maybe_resolved = resolve_identity_from_pattern_hint(identity_literal, pattern_hint.value());
            if (maybe_resolved.has_value())
                expr = maybe_resolved.value();
        }

        // Identity could not be resolved
        if (!expr.has_value())
        {
            source->log_error("'" + identity + "' is not defined.", identity_literal->span);
            expr = CREATE(InvalidExpression);
        }

        expression_literal->expr = expr.value();
    }

    else
    {
        throw CompilerError("Could not resolve variant of UnresolvedLiteral as expression", get_span(unresolved_literal));
    }

    return expression_literal;
}

void Resolver::resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
    {
        auto value = list->values[i];
        list->values[i] = resolve_expression(value, scope, pattern_hint);
    }
}

void Resolver::resolve_instance_list(ptr<InstanceList> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
    {
        auto value = list->values[i];
        list->values[i] = resolve_expression(value, scope, pattern_hint);
    }
}

void Resolver::resolve_call(ptr<Call> call, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    // TODO: Resolve callee

    for (size_t i = 0; i < call->arguments.size(); i++)
    {
        auto argument = call->arguments[i];
        call->arguments[i].value = resolve_expression(argument.value, scope, pattern_hint);
    }
}

void Resolver::resolve_if_expression(ptr<IfExpression> if_expression, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (auto &rule : if_expression->rules)
    {
        rule.condition = resolve_expression(rule.condition, scope);
        rule.result = resolve_expression(rule.result, scope, pattern_hint);
    }
}

void Resolver::resolve_match(ptr<MatchExpression> match, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    match->subject = resolve_expression(match->subject, scope);
    auto subject_pattern = determine_expression_pattern(match->subject);

    for (auto &rule : match->rules)
    {
        rule.pattern = resolve_pattern(rule.pattern, scope, subject_pattern);
        rule.result = resolve_expression(rule.result, scope, pattern_hint);
    }
}

void Resolver::resolve_expression_index(ptr<ExpressionIndex> expression_index, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    expression_index->subject = resolve_expression(expression_index->subject, scope);
    expression_index->index = resolve_expression(expression_index->index, scope);
}

// NOTE: Currently, when resolving which property is being used in a property index, we look at
//       all overloads that could match, and throw an error unless there is exactly one match.
void Resolver::resolve_property_index(ptr<PropertyIndex> property_index, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    property_index->expr = resolve_expression(property_index->expr, scope);

    if (IS_PTR(property_index->property, IdentityLiteral))
    {
        auto identity_literal = AS_PTR(property_index->property, IdentityLiteral);
        auto identity = identity_literal->identity;
        auto instance_list = AS_PTR(property_index->expr, InstanceList);
        auto all_overloads = fetch_all_overloads(scope, identity);

        if (all_overloads.size() == 0)
        {
            // FIXME: If the identity is declared (just not as a property), give additional information about what it is.
            source->log_error("Property '" + identity + "' does not exist.", property_index->span);

            // FIXME: This code for generating the invalid span is duplicated three times in the function. Simplify?
            auto invalid_property = CREATE(InvalidProperty);
            invalid_property->span = identity_literal->span;
            property_index->property = invalid_property;
            return;
        }

        vector<Property> valid_overloads;
        for (auto overload : all_overloads)
        {
            if (IS_PTR(overload, StateProperty))
            {
                auto state_property = AS_PTR(overload, StateProperty);
                if (does_instance_list_match_parameters(instance_list, state_property->parameters))
                    valid_overloads.emplace_back(state_property);
            }

            else if (IS_PTR(overload, FunctionProperty))
            {
                auto function_property = AS_PTR(overload, FunctionProperty);
                if (does_instance_list_match_parameters(instance_list, function_property->parameters))
                    valid_overloads.emplace_back(function_property);
            }
        }

        if (valid_overloads.size() == 0)
        {
            source->log_error("No version of the property '" + identity + "' applies to these arguments.", property_index->span);

            // FIXME: This code for generating the invalid span is duplicated three times in the function. Simplify?
            auto invalid_property = CREATE(InvalidProperty);
            invalid_property->span = identity_literal->span;
            property_index->property = invalid_property;
            return;
        }
        else if (valid_overloads.size() > 1)
        {
            source->log_error("Which version of the property '" + identity + "' applies to these arguments is ambiguous.", property_index->span);

            // FIXME: This code for generating the invalid span is duplicated three times in the function. Simplify?
            auto invalid_property = CREATE(InvalidProperty);
            invalid_property->span = identity_literal->span;
            property_index->property = invalid_property;
            return;
        }

        property_index->property = valid_overloads[0];
    }
}

void Resolver::resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    unary->value = resolve_expression(unary->value, scope);
}

void Resolver::resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    if (binary->op == "==" || binary->op == "!=")
    {
        // FIXME: This allows the rhs to infer it's pattern from the lhs,
        //        but not vice versa. This means an enum on the lhs cannot
        //        be resolved using the pattern of the expression on the rhs.
        binary->lhs = resolve_expression(binary->lhs, scope);
        auto lhs_pattern = determine_expression_pattern(binary->lhs);
        binary->rhs = resolve_expression(binary->rhs, scope, lhs_pattern);
    }
    else if (binary->op == "insert")
    {
        binary->lhs = resolve_expression(binary->lhs, scope);
        auto lhs_pattern = determine_expression_pattern(binary->lhs);
        binary->rhs = resolve_expression(binary->rhs, scope, lhs_pattern);
    }
    else
    {
        binary->lhs = resolve_expression(binary->lhs, scope);
        binary->rhs = resolve_expression(binary->rhs, scope);
    }
}

// PATTERNS //

Pattern Resolver::resolve_pattern(Pattern pattern, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    // Literals
    if (IS(pattern, UnresolvedLiteral))
        return resolve_literal_as_pattern(AS(pattern, UnresolvedLiteral), scope, pattern_hint);

    if (IS_PTR(pattern, PatternLiteral))
    {
        // FIXME: If a node is a PatternLiteral, that should mean it has already been resolved.
        //        However, as of right now, the parser sometimes outputs PatternLiterals directly.

        auto pattern_literal = AS_PTR(pattern, PatternLiteral);
        pattern_literal->pattern = resolve_pattern(pattern_literal->pattern, scope, pattern_hint);
        return pattern_literal;
    }

    // Any pattern
    if (IS_PTR(pattern, AnyPattern))
        return pattern;

    // Union patterns
    if (IS_PTR(pattern, UnionPattern))
    {
        auto union_pattern = AS_PTR(pattern, UnionPattern);

        // Resolve all patterns in the union
        for (size_t i = 0; i < union_pattern->patterns.size(); i++)
        {
            auto pattern = union_pattern->patterns[i];
            union_pattern->patterns[i] = resolve_pattern(pattern, scope, pattern_hint);
        }

        // Remove any pattern in the union that is a subset of another pattern
        {
            size_t i = 0;
            while (i < union_pattern->patterns.size() - 1)
            {
                auto pattern = union_pattern->patterns[i];
                bool pattern_is_redundant = false;

                for (size_t j = union_pattern->patterns.size() - 1; j > i; j--)
                {
                    auto other = union_pattern->patterns[j];
                    if (is_pattern_subset_of_superset(pattern, other))
                    {
                        pattern_is_redundant = true;
                        break;
                    }

                    if (is_pattern_subset_of_superset(other, pattern))
                    {
                        union_pattern->patterns.erase(union_pattern->patterns.begin() + i);
                    }
                }

                if (pattern_is_redundant)
                    union_pattern->patterns.erase(union_pattern->patterns.begin() + i);
                else
                    i++;
            }
        }

        // If only one pattern is present in the union, the union is unecessary
        if (union_pattern->patterns.size() == 1)
            return union_pattern->patterns[0];

        return union_pattern;
    }

    // Values
    if (IS_PTR(pattern, PrimitiveValue))
        return pattern;

    if (IS_PTR(pattern, EnumValue))
        return pattern;

    // Types
    if (IS_PTR(pattern, PrimitiveType))
        return pattern;

    if (IS_PTR(pattern, ListType))
    {
        auto list_type = AS_PTR(pattern, ListType);
        list_type->list_of = resolve_pattern(list_type->list_of, scope);
        // TODO: Resolve the `fixed_size`, if present
        return list_type;
    }

    if (IS_PTR(pattern, EnumType))
        return pattern;

    if (IS_PTR(pattern, EntityType))
        return pattern;

    // Uninferred pattern
    if (IS_PTR(pattern, UninferredPattern))
        throw CompilerError("Attempt to resolve an UninferredPattern");

    // Invalid pattern
    if (IS_PTR(pattern, InvalidPattern))
        return pattern;

    throw CompilerError("Could not resolve Pattern variant ");
}

ptr<PatternLiteral> Resolver::resolve_literal_as_pattern(UnresolvedLiteral unresolved_literal, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    auto pattern_literal = CREATE(PatternLiteral);
    pattern_literal->span = get_span(unresolved_literal);

    // Primitive literal
    if (IS_PTR(unresolved_literal, PrimitiveLiteral))
    {
        auto primitive_literal = AS_PTR(unresolved_literal, PrimitiveLiteral);
        // NOTE: We do not attempt to resolve `primitive_literal->value`, as this value
        //       will always be a PrimitiveValue, which has no need to be resolved.
        pattern_literal->pattern = primitive_literal->value;
    }

    // List literal
    else if (IS_PTR(unresolved_literal, ListLiteral))
    {
        auto list_literal = AS_PTR(unresolved_literal, ListLiteral);

        if (list_literal->values.size() == 1 || list_literal->values.size() == 2)
        {
            auto list_type = CREATE(ListType);

            auto inner_expr = list_literal->values[0];
            if (!IS(inner_expr, UnresolvedLiteral))
            {
                // FIXME: We should return a user error if the `inner_expr` 'literal' cannot be recast as a pattern literal.
                throw CompilerError("Cannot resolve ListLiteral as a pattern as the inner expression is not a UnresolvedLiteral", list_literal->span);
            }
            auto inner_literal = AS(inner_expr, UnresolvedLiteral);
            list_type->list_of = resolve_literal_as_pattern(inner_literal, scope, pattern_hint);

            // TODO: Resolve the 'fixed size' (once the resolver is done, there should be no UnresolvedLiterals)
            if (list_literal->values.size() == 2)
                list_type->fixed_size = list_literal->values[1];

            pattern_literal->pattern = list_type;
        }
        else
        {
            throw CompilerError("Cannot resolve ListLiteral as pattern - Not yet implemented.", list_literal->span);
        }
    }

    // Identity literals
    else if (IS_PTR(unresolved_literal, IdentityLiteral))
    {
        auto identity_literal = AS_PTR(unresolved_literal, IdentityLiteral);
        auto identity = identity_literal->identity;

        optional<Pattern> pattern;

        // Resolve identity by searching for it in the scope
        if (declared_in_scope(scope, identity))
        {
            auto resolved = fetch(scope, identity);

            if (IS(resolved, Pattern))
            {
                pattern = AS(resolved, Pattern);
            }
            else
            {
                // FIXME: Provide information about what the node actually is.
                source->log_error("'" + identity_literal->identity + "' is not a type or pattern", identity_literal->span);
                pattern = CREATE(InvalidPattern);
            }
        }

        // Resolve identity using the pattern hint
        else if (pattern_hint.has_value())
        {
            auto maybe_resolved = resolve_identity_from_pattern_hint(identity_literal, pattern_hint.value());
            if (maybe_resolved.has_value())
                pattern = maybe_resolved.value();
        }

        // Identity could not be resolved
        if (!pattern.has_value())
        {
            source->log_error("'" + identity + "' is not defined.", identity_literal->span);
            pattern = CREATE(InvalidPattern);
        }

        pattern_literal->pattern = pattern.value();
    }

    else
    {
        throw CompilerError("Could not resolve variant of UnresolvedLiteral as pattern", get_span(unresolved_literal));
    }

    return pattern_literal;
}

optional<ptr<EnumValue>> Resolver::resolve_identity_from_pattern_hint(ptr<IdentityLiteral> identity_literal, Pattern hint)
{
    if (IS_PTR(hint, PatternLiteral))
    {
        auto pattern_literal = AS_PTR(hint, PatternLiteral);
        return resolve_identity_from_pattern_hint(identity_literal, pattern_literal->pattern);
    }

    auto identity = identity_literal->identity;

    if (IS_PTR(hint, EnumType))
    {
        auto enum_type = AS_PTR(hint, EnumType);
        for (const auto &value : enum_type->values)
            if (value->identity == identity)
                return value;
    }

    if (IS_PTR(hint, EnumValue))
    {
        // FIXME: Technically two different enums can have the same identity,
        //        meaning this is not correct.
        auto value = AS_PTR(hint, EnumValue);
        if (value->identity == identity)
            return value;
    }

    if (IS_PTR(hint, UnionPattern))
    {
        auto union_pattern = AS_PTR(hint, UnionPattern);
        vector<ptr<EnumValue>> potential_values;
        for (auto pattern : union_pattern->patterns)
        {
            if (IS_PTR(pattern, EnumType))
            {
                auto enum_type = AS_PTR(pattern, EnumType);
                for (const auto &value : enum_type->values)
                    if (value->identity == identity)
                        potential_values.push_back(value);
            }

            if (IS_PTR(pattern, EnumValue))
            {
                auto value = AS_PTR(pattern, EnumValue);
                if (value->identity == identity)
                    potential_values.push_back(value);
            }
        }

        if (potential_values.size() == 1)
        {
            return potential_values[0];
        }
        else if (potential_values.size() > 1)
        {
            string msg = "'" + identity + "' is ambiguous. It could refer to any of ";
            for (size_t i = 0; i < potential_values.size(); i++)
            {
                auto value = potential_values[i];
                if (i > 0)
                    msg += ", ";
                msg += value->type->identity + ":" + value->identity;
            }
            msg += ".";
            source->log_error(msg, identity_literal->span);
        }
    }

    return {};
}
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
        auto stmt = code_block->statements[0];
        code_block->statements[0] = resolve_statement(stmt, code_block->scope, pattern_hint);
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

    else if (IS_PTR(stmt, InvalidStatement))
        ; // pass

    else
        throw CompilerError("Cannot resolve Statement variant.", get_span(stmt));

    return stmt;
}

void Resolver::resolve_if_statement(ptr<IfStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (auto segment : stmt->segments)
    {
        segment.condition = resolve_expression(segment.condition, scope);
        resolve_code_block(segment.code_block, pattern_hint);
    }

    if (stmt->fallback.has_value())
        resolve_code_block(stmt->fallback.value(), pattern_hint);
}

void Resolver::resolve_for_statement(ptr<ForStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    stmt->range = resolve_pattern(stmt->range, scope);
    // TODO: Infer pattern for the variable
    stmt->variable->pattern = stmt->range;
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
    if (IS_PTR(expression, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
        auto identity = unresolved_identity->identity;

        // Resolve identity by searching for it in the scope
        if (declared_in_scope(scope, identity))
        {
            auto resolved = fetch(scope, identity);

            if (IS_PTR(resolved, Variable))
                return AS_PTR(resolved, Variable);

            // FIXME: Make error more informative by saying _what_ the resolved object is (e.g. an entity, a type, etc)
            source->log_error("Expected value, got '" + identity + "'", unresolved_identity->span);

            auto invalid_value = CREATE(InvalidValue);
            invalid_value->span = unresolved_identity->span;
            return invalid_value;
        }

        // Resolve identity using the pattern hint
        if (pattern_hint.has_value())
        {
            auto resolved = resolve_identity_from_pattern_hint(unresolved_identity, pattern_hint.value());
            if (IS_PTR(resolved, InvalidValue))
                ; // pass (proceed to throwing an error)
            else if (IS_PTR(resolved, EnumValue))
                return AS_PTR(resolved, EnumValue);
            else
                CompilerError("Cannot resolve variant of identity resolved from pattern hint (while resolving expression).", get_span(expression));
        }

        // Identity could not be resolved
        source->log_error("'" + identity + "' is not defined.", unresolved_identity->span);

        auto invalid_value = CREATE(InvalidValue);
        invalid_value->span = unresolved_identity->span;
        return invalid_value;
    }

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

    else if (IS_PTR(expression, Match))
        resolve_match(AS_PTR(expression, Match), scope, pattern_hint);

    return expression;
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

void Resolver::resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Pattern> pattern_hint)
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

    if (IS_PTR(property_index->property, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(property_index->property, UnresolvedIdentity);
        auto identity = unresolved_identity->identity;
        auto instance_list = AS_PTR(property_index->expr, InstanceList);
        auto all_overloads = fetch_all_overloads(scope, identity);

        if (all_overloads.size() == 0)
        {
            // FIXME: If the identity is declared (just not as a property), give additional information about what it is.
            source->log_error("Property '" + identity + "' does not exist.", property_index->span);

            // FIXME: This code for generating the invalid span is duplicated three times in the function. Simplify?
            auto invalid_property = CREATE(InvalidProperty);
            invalid_property->span = unresolved_identity->span;
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
            invalid_property->span = unresolved_identity->span;
            property_index->property = invalid_property;
            return;
        }
        else if (valid_overloads.size() > 1)
        {
            source->log_error("Which version of the property '" + identity + "' applies to these arguments is ambiguous.", property_index->span);

            // FIXME: This code for generating the invalid span is duplicated three times in the function. Simplify?
            auto invalid_property = CREATE(InvalidProperty);
            invalid_property->span = unresolved_identity->span;
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
    if (IS_PTR(pattern, UnresolvedIdentity))
    {
        auto unresolved_identity = AS_PTR(pattern, UnresolvedIdentity);
        auto identity = unresolved_identity->identity;

        // Resolve identity by searching for it in the scope
        if (declared_in_scope(scope, identity))
        {
            auto resolved = fetch(scope, identity);

            // NOTE: We presume that if a UnionPattern has been declared, then it must
            //       represent an enum comprised of both enum and intrinsic values.
            if (IS_PTR(resolved, UnionPattern))
                return AS_PTR(resolved, UnionPattern);
            if (IS_PTR(resolved, IntrinsicType))
                return AS_PTR(resolved, IntrinsicType);
            if (IS_PTR(resolved, EnumType))
                return AS_PTR(resolved, EnumType);
            if (IS_PTR(resolved, Entity))
                return AS_PTR(resolved, Entity);

            // FIXME: Provide information about what the node actually is.
            source->log_error("'" + identity_of(resolved) + "' is not a type", unresolved_identity->span);

            auto invalid_pattern = CREATE(InvalidPattern);
            invalid_pattern->span = unresolved_identity->span;
            return invalid_pattern;
        }

        // Resolve identity using the pattern hint
        if (pattern_hint.has_value())
        {
            auto resolved = resolve_identity_from_pattern_hint(unresolved_identity, pattern_hint.value());
            if (IS_PTR(resolved, InvalidValue))
                ; // pass (proceed to throwing an error)
            else if (IS_PTR(resolved, EnumValue))
                return AS_PTR(resolved, EnumValue);
            else
                CompilerError("Cannot resolve variant of identity resolved from pattern hint (while resolving pattern).", get_span(pattern));
        }

        // Identity could not be resolved
        source->log_error("'" + identity + "' is not defined.", unresolved_identity->span);

        auto invalid_pattern = CREATE(InvalidPattern);
        invalid_pattern->span = unresolved_identity->span;
        return invalid_pattern;
    }

    else if (IS_PTR(pattern, UnionPattern))
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
    }

    else if (IS_PTR(pattern, ListPattern))
    {
        auto list_pattern = AS_PTR(pattern, ListPattern);
        list_pattern->list_of = resolve_pattern(list_pattern->list_of, scope);
    }

    return pattern;
}

variant<ptr<EnumValue>, ptr<InvalidValue>> Resolver::resolve_identity_from_pattern_hint(ptr<UnresolvedIdentity> unresolved_identity, Pattern hint)
{
    auto identity = unresolved_identity->identity;

    if (IS_PTR(hint, EnumType))
    {
        auto enum_type = AS_PTR(hint, EnumType);
        for (const auto &value : enum_type->values)
            if (value->identity == identity)
                return value;
    }

    if (IS_PTR(hint, EnumValue))
    {
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
            source->log_error(msg, unresolved_identity->span);
        }
    }

    auto invalid_value = CREATE(InvalidValue);
    invalid_value->span = unresolved_identity->span;
    return invalid_value;
}
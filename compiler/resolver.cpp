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
}

void Resolver::resolve_code_block(ptr<CodeBlock> code_block, optional<Pattern> pattern_hint)
{
    resolve_scope(code_block->scope);

    if (code_block->singleton_block)
    {
        // The statement of a singleton code block is given the pattern hint,
        // whereas statements in a regular code block are not.
        auto stmt = code_block->statements[0];
        stmt = resolve_statement(stmt, code_block->scope, pattern_hint);
        code_block->statements[0] = stmt;
    }
    else
    {
        for (size_t i = 0; i < code_block->statements.size(); i++)
        {
            auto stmt = code_block->statements[i];
            stmt = resolve_statement(stmt, code_block->scope);
            code_block->statements[i] = stmt;
        }
    }
}

void Resolver::resolve_scope(ptr<Scope> scope)
{
    // Property signatures need to be resolved before property and procedure
    // bodies, so that PropertyIndex expressions can correctly resolve which
    // overload of the property they should use.
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
        {
            state->initial_value = resolve_expression(state->initial_value.value(), state->scope, state->pattern);
            // FIXME: Pattern check the default value
        }
    }

    else if (IS_PTR(value, FunctionProperty))
    {
        auto funct = AS_PTR(value, FunctionProperty);
        if (funct->body.has_value())
            resolve_code_block(funct->body.value(), funct->pattern);
    }

    else if (IS_PTR(value, Scope::OverloadedIdentity))
    {
        auto overloaded_identity = AS_PTR(value, Scope::OverloadedIdentity);
        for (auto overload : overloaded_identity->overloads)
            resolve_scope_lookup_value_final_pass(overload, scope);
    }
}

void Resolver::resolve_optional_pattern(ptr<OptionalPattern> optional_pattern, ptr<Scope> scope)
{
    optional_pattern->pattern = resolve_pattern(optional_pattern->pattern, scope);
}

Pattern Resolver::resolve_pattern(Pattern pattern, ptr<Scope> scope)
{
    try
    {
        if (IS_PTR(pattern, InvalidPattern))
            ; // pass

        else if (IS_PTR(pattern, NativeType) || IS_PTR(pattern, EnumType) || IS_PTR(pattern, Entity))
            ; // pass

        else if (IS_PTR(pattern, OptionalPattern))
            resolve_optional_pattern(AS_PTR(pattern, OptionalPattern), scope);

        else if (IS_PTR(pattern, UnresolvedIdentity))
        {
            auto unresolved_identity = AS_PTR(pattern, UnresolvedIdentity);
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

                auto unresolved_identity = AS_PTR(pattern, UnresolvedIdentity);
                throw GambitError("'" + identity_of(resolved) + "' is not a type", unresolved_identity->token);
            }
            else
            {
                throw GambitError("'" + id + "' is not defined.", unresolved_identity->token);
            }

            return CREATE(InvalidPattern);
        }

        else
            throw CompilerError("Cannot resolve Pattern variant.");
    }
    catch (GambitError err)
    {
        ; // pass
    }

    return pattern;
}

Expression Resolver::resolve_expression(Expression expression, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    try
    {
        if (IS_PTR(expression, UnresolvedIdentity))
        {
            auto unresolved_identity = AS_PTR(expression, UnresolvedIdentity);
            string id = unresolved_identity->identity;

            if (declared_in_scope(scope, id)) // Resolve identity from scope
            {
                auto resolved = fetch(scope, id);

                if (IS_PTR(resolved, Variable))
                    return AS_PTR(resolved, Variable);

                // FIXME: Make error more informative by saying _what_ the resolved object is (e.g. an entity, a type, etc)
                throw GambitError("Expected value, got '" + id + "'", unresolved_identity->token);

                return CREATE(InvalidValue);
            }

            if (pattern_hint.has_value()) // Resolve identity from pattern hint
            {
                auto the_pattern_hint = pattern_hint.value();
                optional<ptr<EnumType>> enum_type = {};

                if (IS_PTR(the_pattern_hint, EnumType)) // Pattern hint is enum type
                {
                    enum_type = AS_PTR(the_pattern_hint, EnumType);
                }
                else if (IS_PTR(the_pattern_hint, OptionalPattern)) // Pattern hint is optional enum type
                {
                    auto optional_pattern = AS_PTR(the_pattern_hint, OptionalPattern);
                    if (IS_PTR(optional_pattern->pattern, EnumType))
                        enum_type = AS_PTR(optional_pattern->pattern, EnumType);
                }

                if (enum_type.has_value())
                    for (const auto &value : enum_type.value()->values)
                        if (value->identity == unresolved_identity->identity)
                            return value;
            }

            throw GambitError("'" + id + "' is not defined.", unresolved_identity->token);
            return CREATE(InvalidValue);
        }
        else if (IS_PTR(expression, Variable))
            ; // pass
        else if (IS_PTR(expression, Literal))
            ; // pass
        else if (IS_PTR(expression, ListValue))
            resolve_list_value(AS_PTR(expression, ListValue), scope, pattern_hint);
        else if (IS_PTR(expression, InstanceList))
            resolve_instance_list(AS_PTR(expression, InstanceList), scope, pattern_hint);
        else if (IS_PTR(expression, EnumValue))
            ; // pass
        else if (IS_PTR(expression, Unary))
            resolve_unary(AS_PTR(expression, Unary), scope, pattern_hint);
        else if (IS_PTR(expression, Binary))
            resolve_binary(AS_PTR(expression, Binary), scope, pattern_hint);
        else if (IS_PTR(expression, PropertyIndex))
            resolve_property_index(AS_PTR(expression, PropertyIndex), scope, pattern_hint);
        else if (IS_PTR(expression, Match))
            resolve_match(AS_PTR(expression, Match), scope, pattern_hint);
        else
            throw CompilerError("Cannot resolve Expression variant.");
    }
    catch (GambitError err)
    {
        ; // pass
    }

    return expression;
}

void Resolver::resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
        list->values[i] = resolve_expression(list->values[i], scope, pattern_hint);
}

void Resolver::resolve_instance_list(ptr<InstanceList> list, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    for (size_t i = 0; i < list->values.size(); i++)
        list->values[i] = resolve_expression(list->values[i], scope, pattern_hint);
}

void Resolver::resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    match->subject = resolve_expression(match->subject, scope);
    // FIXME: Implement pattern checking on the subject
    // auto subject_type = determine_type(match->subject);

    for (auto &rule : match->rules)
    {
        rule.pattern = resolve_expression(rule.pattern, scope);
        // FIXME: Implement pattern checking and inference on the pattern
        rule.result = resolve_expression(rule.result, scope, pattern_hint);
    }
}

// NOTE: Currently when resolving which property is being used in a property index
//       we look at all overloads that could match, and throw an error unless there
//       is exactly one match. That is to say, we ignore the specifity of the match.
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
            throw GambitError("Property '" + identity + "' does not exist.", unresolved_identity->token);

        vector<variant<ptr<UnresolvedIdentity>, ptr<StateProperty>, ptr<FunctionProperty>>> valid_overloads;
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
            throw GambitError("No version of the property '" + identity + "' applies to these arguments.", unresolved_identity->token);
        else if (valid_overloads.size() > 1)
            throw GambitError("Which version of the property '" + identity + "' applies to these arguments is ambiguous.", unresolved_identity->token);

        property_index->property = valid_overloads[0];
    }
}

void Resolver::resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    unary->value = resolve_expression(unary->value, scope);
    // FIXME: Implement pattern checking on the operands
}

void Resolver::resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    binary->lhs = resolve_expression(binary->lhs, scope);
    binary->rhs = resolve_expression(binary->rhs, scope);
    // FIXME: Implement pattern checking on the operands
}

Statement Resolver::resolve_statement(Statement stmt, ptr<Scope> scope, optional<Pattern> pattern_hint)
{
    try
    {
        if (IS_PTR(stmt, CodeBlock))
            resolve_code_block(AS_PTR(stmt, CodeBlock), pattern_hint);
        else if (IS(stmt, Expression))
            return resolve_expression(AS(stmt, Expression), scope, pattern_hint);
        else
            throw CompilerError("Cannot resolve Statement variant.");
    }
    catch (GambitError err)
    {
        ; // pass
    }

    return stmt;
}
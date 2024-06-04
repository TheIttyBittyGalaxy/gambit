#pragma once
#ifndef RESOLVER_H
#define RESOLVER_H

#include "apm.h"
#include "source.h"
#include "utilty.h"
#include <optional>
using namespace std;

class Resolver
{
public:
    void resolve(Source &source, ptr<Program> program);

private:
    ptr<Program> program = nullptr;
    Source *source = nullptr;

    // PROGRAM STRUCTURE //
    void resolve_program(ptr<Program> program);
    void resolve_scope(ptr<Scope> scope);
    void resolve_scope_lookup_value_property_signatures_pass(Scope::LookupValue value, ptr<Scope> scope);
    void resolve_scope_lookup_value_final_pass(Scope::LookupValue value, ptr<Scope> scope);
    void resolve_code_block(ptr<CodeBlock> code_block, optional<Pattern> pattern_hint = {});

    // STATEMENTS //
    [[nodiscard]] Statement resolve_statement(Statement statement, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_if_statement(ptr<IfStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_for_statement(ptr<ForStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_assignment_statement(ptr<AssignmentStatement> stmt, ptr<Scope> scope);
    void resolve_variable_declaration(ptr<VariableDeclaration> stmt, ptr<Scope> scope);

    // EXPRESSIONS //
    [[nodiscard]] Expression resolve_expression(Expression expression, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_instance_list(ptr<InstanceList> list, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_call(ptr<Call> call, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_expression_index(ptr<ExpressionIndex> expression_index, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_property_index(ptr<PropertyIndex> property_index, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Pattern> pattern_hint = {});

    // PATTERNS //
    [[nodiscard]] Pattern resolve_pattern(Pattern pattern, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    [[nodiscard]] variant<ptr<EnumValue>, ptr<InvalidValue>> resolve_identity_from_pattern_hint(ptr<UnresolvedIdentity> unresolved_identity, Pattern hint);
};

#endif
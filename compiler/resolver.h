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
    void resolve_loop_statement(ptr<LoopStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_return_statement(ptr<ReturnStatement> stmt, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_wins_statement(ptr<WinsStatement> stmt, ptr<Scope> scope);
    void resolve_assignment_statement(ptr<AssignmentStatement> stmt, ptr<Scope> scope);
    void resolve_variable_declaration(ptr<VariableDeclaration> stmt, ptr<Scope> scope);

    // EXPRESSIONS //
    [[nodiscard]] Expression resolve_expression(Expression expression, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    [[nodiscard]] ptr<ExpressionLiteral> resolve_literal_as_expression(UnresolvedLiteral unresolved_literal, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_instance_list(ptr<InstanceList> list, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_call(ptr<Call> call, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_choose_expression(ptr<ChooseExpression> choose_expression, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_if_expression(ptr<IfExpression> if_expression, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_match(ptr<MatchExpression> match, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_index_with_expression(ptr<IndexWithExpression> index_with_expression, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    Expression resolve_index_with_identity(ptr<IndexWithIdentity> index_with_identity, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    void resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Pattern> pattern_hint = {});

    // PATTERNS //
    [[nodiscard]] Pattern resolve_pattern(Pattern pattern, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    [[nodiscard]] ptr<PatternLiteral> resolve_literal_as_pattern(UnresolvedLiteral unresolved_literal, ptr<Scope> scope, optional<Pattern> pattern_hint = {});
    [[nodiscard]] optional<ptr<EnumValue>> resolve_identity_from_pattern_hint(ptr<IdentityLiteral> identity_literal, Pattern hint);
};

#endif
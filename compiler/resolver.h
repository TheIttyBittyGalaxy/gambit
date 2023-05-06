#ifndef RESOLVER_H
#define RESOLVER_H

#include "apm.h"
#include "utilty.h"
#include <optional>
using namespace std;

class Resolver
{
public:
    void resolve(ptr<Program> program);

private:
    ptr<Program> program = nullptr;

    void resolve_program(ptr<Program> program);
    void resolve_code_block(ptr<CodeBlock> code_block, optional<Type> type_hint = {});
    void resolve_scope(ptr<Scope> scope);
    void resolve_scope_lookup_value(Scope::LookupValue value, ptr<Scope> scope);

    void resolve_state_property(ptr<StateProperty> state, ptr<Scope> scope);
    void resolve_function_property(ptr<FunctionProperty> function, ptr<Scope> scope);

    void resolve_pattern(ptr<Pattern> pattern, ptr<Scope> scope);
    void resolve_pattern_list(ptr<PatternList> pattern_list, ptr<Scope> scope);

    Type resolve_optional_type(ptr<OptionalType> type, ptr<Scope> scope);
    optional<Type> resolve_type(Type type, ptr<Scope> scope);

    Expression resolve_expression(Expression expression, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_list_value(ptr<ListValue> list, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_property_index(ptr<PropertyIndex> property_index, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Type> type_hint = {});

    Statement resolve_statement(Statement statement, ptr<Scope> scope, optional<Type> type_hint = {});
};

#endif
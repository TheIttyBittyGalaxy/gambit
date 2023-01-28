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
    void resolve_scope(ptr<Scope> scope);

    Type resolve_optional_type(ptr<OptionalType> type, ptr<Scope> scope);
    optional<Type> resolve_type(Type type, ptr<Scope> scope);

    void resolve_entity_definition(ptr<Entity> definition, ptr<Scope> scope);
    void resolve_entity_field(ptr<EntityField> field, ptr<Entity> entity, ptr<Scope> scope);

    Expression resolve_expression(Expression expression, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_match(ptr<Match> match, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_unary(ptr<Unary> unary, ptr<Scope> scope, optional<Type> type_hint = {});
    void resolve_binary(ptr<Binary> binary, ptr<Scope> scope, optional<Type> type_hint = {});
};

#endif
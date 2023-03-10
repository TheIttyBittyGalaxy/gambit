#ifndef APM_H
#define APM_H

#include "token.h"
#include "utilty.h"
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
using namespace std;

// Forward declarations

struct Program;
struct Scope;

struct UnresolvedIdentity;

struct EnumType;
struct EnumValue;

struct Entity;
struct EntityField;

struct NativeType;
struct OptionalType;
struct InvalidType;
using Type = variant<
    ptr<UnresolvedIdentity>,
    ptr<EnumType>,
    ptr<Entity>,
    ptr<NativeType>,
    ptr<OptionalType>,
    ptr<InvalidType>>;

struct Literal;
struct ListValue;
struct Unary;
struct Binary;
struct Match;
struct InvalidValue;
using Expression = variant<
    ptr<UnresolvedIdentity>,
    ptr<EnumValue>,
    ptr<Literal>,
    ptr<ListValue>,
    ptr<Unary>,
    ptr<Binary>,
    ptr<Match>,
    ptr<InvalidValue>>;

// Program

struct Program
{
    ptr<Scope> global_scope;
};

struct Scope
{
    using LookupValue = variant<ptr<NativeType>, ptr<EnumType>, ptr<Entity>>;
    wptr<Scope> parent;
    map<string, LookupValue> lookup;
};

// Unresolved Identity

struct UnresolvedIdentity
{
    string identity;
    Token token;
};

// Enums

struct EnumType
{
    string identity;
    vector<ptr<EnumValue>> values;
};

struct EnumValue
{
    string identity;
};

// Entities

struct Entity
{
    string identity;
    map<string, ptr<EntityField>> fields;
    vector<string> signature;
    bool base_definition_found = false;
};

struct EntityField
{
    string identity;
    Type type;
    bool is_static = false;
    bool is_property = false;
    optional<Expression> initializer;
};

// Types

struct NativeType
{
    string identity;
    string cpp_identity;
};

struct OptionalType
{
    Type type;
};

struct InvalidType
{
    // FIXME: Include the token of node that eventually became this invalid type
};

// Expressions

struct Literal
{
    variant<double, int, bool, string> value;
};

struct ListValue
{
    vector<Expression> values;
};

struct Unary
{
    string op;
    Expression value;
};

struct Binary
{
    string op;
    Expression lhs;
    Expression rhs;
};

struct Match
{
    struct Rule
    {
        Expression pattern;
        Expression result;
    };
    Expression subject;
    vector<Rule> rules;
};

struct InvalidValue
{
    // FIXME: Include the token of node that eventually became this invalid value
};

// Methods

string identity_of(Scope::LookupValue value);
bool directly_declared_in_scope(ptr<Scope> scope, string identity);
bool declared_in_scope(ptr<Scope> scope, string identity);
void declare(ptr<Scope> scope, Scope::LookupValue value);
Scope::LookupValue fetch(ptr<Scope> scope, string identity);
ptr<NativeType> fetch_native_type(ptr<Scope> scope, string identity);
ptr<EnumType> fetch_enum_type(ptr<Scope> scope, string identity);
ptr<Entity> fetch_entity(ptr<Scope> scope, string identity);

// JSON Serialisation

string to_json(const ptr<Program> &program, const size_t &depth = 0);
string to_json(const Scope::LookupValue &lookup_value, const size_t &depth = 0);
string to_json(const ptr<Scope> &scope, const size_t &depth = 0);
string to_json(const ptr<UnresolvedIdentity> &unresolved_identity, const size_t &depth = 0);
string to_json(const ptr<EnumType> &enum_type, const size_t &depth = 0);
string to_json(const ptr<EnumValue> &enum_value, const size_t &depth = 0);
string to_json(const ptr<Entity> &entity, const size_t &depth = 0);
string to_json(const ptr<EntityField> &entity_field, const size_t &depth = 0);
string to_json(const ptr<NativeType> &native_type, const size_t &depth = 0);
string to_json(const ptr<OptionalType> &optional_type, const size_t &depth = 0);
string to_json(const ptr<InvalidType> &invalid_type, const size_t &depth = 0);
string to_json(const Type &type, const size_t &depth = 0);
string to_json(const ptr<Unary> &unary, const size_t &depth = 0);
string to_json(const ptr<Binary> &binary, const size_t &depth = 0);
string to_json(const Match::Rule &rule, const size_t &depth = 0);
string to_json(const ptr<Match> &match, const size_t &depth = 0);
string to_json(const ptr<InvalidValue> &invalid_value, const size_t &depth = 0);
string to_json(const ptr<Literal> &literal, const size_t &depth = 0);
string to_json(const ptr<ListValue> &list_value, const size_t &depth = 0);
string to_json(const Expression &expression, const size_t &depth = 0);

#endif
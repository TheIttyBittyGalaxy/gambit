#ifndef APM_H
#define APM_H

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

struct NativeType;

struct EnumType;
struct EnumValue;

struct Entity;
struct EntityField;

using Type = variant<ptr<UnresolvedIdentity>, ptr<NativeType>, ptr<EnumType>, ptr<Entity>>;

struct Literal;
using Expression = variant<ptr<Literal>, ptr<EnumValue>>;

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
};

// Native type

struct NativeType
{
    string identity;
    string cpp_identity;
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
    bool base_definition_found = false;
};

struct EntityField
{
    string identity;
    Type type;
    bool is_static = false;
    bool is_property = false;
    optional<Expression> default_value;
};

// Expressions

struct Literal
{
    variant<double, int, bool, string> value;
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

#endif
/*
apm.h

Defines the nodes of the Abstract Program Model as well as a range of utility methods.
The utility methods should not have side-effects, and should not be responsible for
handling language errors.

Different parts of the compiler handle different scenarios in different ways, and so
side-effects be managed by the 'actual' compiler.
*/

#pragma once
#ifndef APM_H
#define APM_H

#include "span.h"
#include "utilty.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
using namespace std;

// Forward declarations

struct Program;
struct CodeBlock;
struct Scope;

struct InvalidStatement;

struct UnresolvedIdentity;

struct Variable;

struct AnyPattern;
struct UnionPattern;
struct InvalidPattern;

struct EnumType;
struct EnumValue;

struct Entity;

struct StateProperty;
struct FunctionProperty;
struct InvalidProperty;
using Property = variant<
    ptr<UnresolvedIdentity>,
    ptr<StateProperty>,
    ptr<FunctionProperty>,
    ptr<InvalidProperty>>;

struct IntrinsicType;
struct IntrinsicValue;

using Pattern = variant<
    ptr<UnresolvedIdentity>,
    ptr<InvalidPattern>,
    ptr<AnyPattern>,
    ptr<UnionPattern>,
    ptr<IntrinsicType>,
    ptr<EnumType>,
    ptr<Entity>,
    ptr<IntrinsicValue>,
    ptr<EnumValue>>;

struct ListValue;
struct InstanceList;
struct Unary;
struct Binary;
struct PropertyIndex;
struct Match;
struct InvalidValue;
struct InvalidExpression;
using Expression = variant<
    ptr<UnresolvedIdentity>,
    ptr<Variable>,
    ptr<EnumValue>,
    ptr<IntrinsicValue>,
    ptr<ListValue>,
    ptr<InstanceList>,
    ptr<Unary>,
    ptr<Binary>,
    ptr<PropertyIndex>,
    ptr<Match>,
    ptr<InvalidValue>,
    ptr<InvalidExpression>>;

struct IfStatement;

using Statement = variant<
    Expression,
    ptr<CodeBlock>,
    ptr<IfStatement>,
    ptr<InvalidStatement>>;

// Program

struct Program
{
    ptr<Scope> global_scope;
};

struct CodeBlock
{
    Span span;
    bool singleton_block = false;
    ptr<Scope> scope;
    vector<Statement> statements;
};

struct Scope
{
    struct OverloadedIdentity;

    using LookupValue = variant<
        ptr<Variable>,
        ptr<UnionPattern>,
        ptr<IntrinsicType>,
        ptr<EnumType>,
        ptr<Entity>,
        ptr<StateProperty>,
        ptr<FunctionProperty>,
        ptr<OverloadedIdentity>>;

    struct OverloadedIdentity
    {
        string identity;
        vector<LookupValue> overloads;
    };

    wptr<Scope> parent;
    unordered_map<string, LookupValue> lookup;
};

struct InvalidStatement
{
    Span span;
};

// Unresolved Identity

struct UnresolvedIdentity
{
    Span span;
    string identity;
};

// Variables

struct Variable
{
    Span span;
    string identity;
    Pattern pattern;
};

// Patterns

// TODO: If the only serious use of the any pattern is in match rules, perhaps it could be
//       replaced with an 'else' syntax (meant to mirror the syntax of else/ifs statements)

// TODO: A semantics question to figure out is if the "Any Pattern" should match against `none`.
//       The compiler currently says yes, so that match 'any rules' can still cover none,
//       but it might be worth giving some thought as to what should apply in other cases.
struct AnyPattern
{
    Span span;
};

// TODO: Current UnionPattern has an identity field and is part of the Scope::LookupValue variant
//       as when an enum is declared with both enums and intrinsic values, we represent that as
//       a UnionPattern declared in the scope. However, most UnionPatterns are anonymous.
//       I'm not sure if this is the best solution for this?
struct UnionPattern
{
    Span span;
    string identity;
    vector<Pattern> patterns;
};

struct InvalidPattern
{
    Span span;
};

// Enums

struct EnumType
{
    Span span;
    string identity;
    vector<ptr<EnumValue>> values;
};

struct EnumValue
{
    Span span;
    string identity;
    ptr<EnumType> type;
};

// Entities

struct Entity
{
    Span span;
    string identity;
};

// Properties

struct StateProperty
{
    Span span;
    string identity;
    Pattern pattern;
    ptr<Scope> scope;
    vector<ptr<Variable>> parameters;
    optional<Expression> initial_value;
};

struct FunctionProperty
{
    Span span;
    string identity;
    Pattern pattern;
    ptr<Scope> scope;
    vector<ptr<Variable>> parameters;
    optional<ptr<CodeBlock>> body;
};

struct InvalidProperty
{
    Span span;
};

// Types

struct IntrinsicType
{
    string identity;
    string cpp_identity;
};

// Expressions

struct IntrinsicValue
{
    Span span;
    variant<double, int, bool, string> value;
    ptr<IntrinsicType> type;
};

struct ListValue
{
    Span span;
    vector<Expression> values;
};

// FIXME: For now I've named value lists (lists of values that correspond to patterns)
//        `InstanceLists` to avoid confusion with 'list' values. Go in and fix the
//        terminology at some point. (perhaps 'lists' need to be called arrays?)
struct InstanceList
{
    Span span;
    vector<Expression> values;
};

struct Unary
{
    Span span;
    string op;
    Expression value;
};

struct Binary
{
    Span span;
    string op;
    Expression lhs;
    Expression rhs;
};

struct PropertyIndex
{
    Span span;
    Expression expr;
    Property property;
};

struct Match
{
    Span span;
    struct Rule
    {
        Span span;
        Pattern pattern;
        Expression result;
    };
    Expression subject;
    vector<Rule> rules;
    bool has_fallback_rule = false;
};

struct InvalidValue
{
    Span span;
};

struct InvalidExpression
{
    Span span;
};

// Statements

struct IfStatement
{
    Span span;
    struct Segment
    {
        Span span;
        Expression condition;
        ptr<CodeBlock> code_block;
    };
    vector<Segment> segments;
    optional<ptr<CodeBlock>> fallback;
};

// Methods

string identity_of(Scope::LookupValue value);
bool directly_declared_in_scope(ptr<Scope> scope, string identity);
bool declared_in_scope(ptr<Scope> scope, string identity);
bool is_overloadable(Scope::LookupValue value);
Span get_span(Statement stmt);
Span get_span(Expression expr);
Span get_span(Pattern pattern);
Span get_span(Scope::LookupValue value);

Scope::LookupValue fetch(ptr<Scope> scope, string identity);
vector<Scope::LookupValue> fetch_all_overloads(ptr<Scope> scope, string identity);

[[nodiscard]] Pattern determine_expression_pattern(Expression expr);
[[nodiscard]] ptr<UnionPattern> create_union_pattern(Pattern a, Pattern b);
bool is_pattern_subset_of_superset(Pattern subset, Pattern superset);
bool is_pattern_optional(Pattern pattern);
bool does_instance_list_match_parameters(ptr<InstanceList> instance_list, vector<ptr<Variable>> parameters);

// JSON Serialisation

string to_json(const ptr<Program> &program, const size_t &depth = 0);
string to_json(const ptr<CodeBlock> &code_block, const size_t &depth = 0);
string to_json(const Scope::LookupValue &lookup_value, const size_t &depth = 0);
string to_json(const ptr<Scope::OverloadedIdentity> &lookup_index, const size_t &depth = 0);
string to_json(const ptr<Scope> &scope, const size_t &depth = 0);
string to_json(const ptr<InvalidStatement> &invalid_statement, const size_t &depth = 0);
string to_json(const ptr<UnresolvedIdentity> &unresolved_identity, const size_t &depth = 0);
string to_json(const ptr<Variable> &unresolved_identity, const size_t &depth = 0);
string to_json(const ptr<AnyPattern> &any_pattern, const size_t &depth = 0);
string to_json(const ptr<UnionPattern> &union_pattern, const size_t &depth = 0);
string to_json(const ptr<InvalidPattern> &invalid_type, const size_t &depth = 0);
string to_json(const ptr<EnumType> &enum_type, const size_t &depth = 0);
string to_json(const ptr<EnumValue> &enum_value, const size_t &depth = 0);
string to_json(const ptr<Entity> &entity, const size_t &depth = 0);
string to_json(const ptr<StateProperty> &state, const size_t &depth = 0);
string to_json(const ptr<FunctionProperty> &state, const size_t &depth = 0);
string to_json(const ptr<InvalidProperty> &invalid_property, const size_t &depth = 0);
string to_json(const Property &property, const size_t &depth = 0);
string to_json(const ptr<IntrinsicType> &intrinsic_type, const size_t &depth = 0);
string to_json(const Pattern &pattern, const size_t &depth = 0);
string to_json(const ptr<Unary> &unary, const size_t &depth = 0);
string to_json(const ptr<Binary> &binary, const size_t &depth = 0);
string to_json(const Match::Rule &rule, const size_t &depth = 0);
string to_json(const ptr<PropertyIndex> &property_index, const size_t &depth = 0);
string to_json(const ptr<Match> &match, const size_t &depth = 0);
string to_json(const ptr<InvalidValue> &invalid_value, const size_t &depth = 0);
string to_json(const ptr<InvalidExpression> &invalid_expression, const size_t &depth = 0);
string to_json(const ptr<IntrinsicValue> &intrinsic_value, const size_t &depth = 0);
string to_json(const ptr<InstanceList> &list_value, const size_t &depth = 0);
string to_json(const ptr<ListValue> &list_value, const size_t &depth = 0);
string to_json(const Expression &expression, const size_t &depth = 0);
string to_json(const IfStatement::Segment &segment, const size_t &depth = 0);
string to_json(const ptr<IfStatement> &if_statement, const size_t &depth = 0);
string to_json(const Statement &statement, const size_t &depth = 0);

#endif
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

// FORWARD DECLARATIONS

// Program
struct Program;
struct CodeBlock;
struct Scope;

struct Procedure;
struct Variable;

// Literals
// NOTE: The Parser produces UnresolvedLiterals in place of expressions and patterns.
//
//       The Resolver turns these into either a PatternLiteral or a ExpressionLiteral.
//       The new node takes the span from the UnresolvedLiteral so we can track where
//       the pattern or expression was originally written in the source. The child node
//       of the new node is then a Pattern/Expression.
//
//       Because of this, these literals should cease to exist after the Resolver stage.

struct PrimitiveLiteral;
struct ListLiteral;
struct IdentityLiteral;
struct OptionLiteral;
using UnresolvedLiteral = variant<
    ptr<PrimitiveLiteral>,
    ptr<ListLiteral>,
    ptr<IdentityLiteral>,
    ptr<OptionLiteral>>;

// Values
struct PrimitiveValue;
struct ListValue;
struct EnumValue;

// Types
struct PrimitiveType;
struct ListType;
struct EnumType;
struct EntityType;

// Properties
struct StateProperty;
struct FunctionProperty;
struct InvalidProperty;
using Property = variant<
    ptr<IdentityLiteral>,
    ptr<StateProperty>,
    ptr<FunctionProperty>,
    ptr<InvalidProperty>>;

// Patterns
struct PatternLiteral;
struct AnyPattern;
struct UnionPattern;

struct UninferredPattern;
struct InvalidPattern;

using Pattern = variant<
    // Literals
    UnresolvedLiteral,
    ptr<PatternLiteral>,

    // The "any pattern" - matches all values
    ptr<AnyPattern>,

    // Union pattern - matches only if at least one sub-pattern matches
    ptr<UnionPattern>,

    // Value pattern - matches a specific value
    ptr<PrimitiveValue>,
    ptr<EnumValue>,

    // Type pattern - matches any value of a specific type
    ptr<PrimitiveType>,
    ptr<ListType>,
    ptr<EnumType>,
    ptr<EntityType>,

    // Uninfered pattern - the pattern is yet to be inferred by the compiler
    ptr<UninferredPattern>,

    // Invalid pattern - the pattern has been deemed invalid at some point during compilation
    ptr<InvalidPattern> //
    >;

// Expressions
struct ExpressionLiteral;

struct Unary;
struct Binary;

struct InstanceList;
struct ExpressionIndex;
struct PropertyIndex;

struct Call;

struct IfExpression;
struct MatchExpression;

struct InvalidExpression;

using Expression = variant<
    // Literals
    UnresolvedLiteral,
    ptr<ExpressionLiteral>,

    // Values
    ptr<PrimitiveValue>,
    ptr<ListValue>,
    ptr<EnumValue>,
    ptr<Variable>,

    // Operations
    ptr<Unary>,
    ptr<Binary>,

    // Indexing
    ptr<InstanceList>,
    ptr<ExpressionIndex>,
    ptr<PropertyIndex>,

    // Calls
    ptr<Call>,

    // "Statement style" expressions
    ptr<IfExpression>,
    ptr<MatchExpression>,

    // Invalid expression - the expression has been deemed invalid at some point during compilation
    ptr<InvalidExpression> //
    >;

// Statements
struct IfStatement;
struct ForStatement;
struct LoopStatement;
struct ReturnStatement;
struct AssignmentStatement;
struct VariableDeclaration;

using Statement = variant<
    ptr<IfStatement>,
    ptr<ForStatement>,
    ptr<LoopStatement>,
    ptr<ReturnStatement>,
    ptr<AssignmentStatement>,
    ptr<VariableDeclaration>,

    ptr<CodeBlock>,
    Expression>;

// PROGRAM

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
        ptr<OverloadedIdentity>,
        ptr<Procedure>,
        ptr<Variable>,

        ptr<StateProperty>,
        ptr<FunctionProperty>,

        Pattern>;

    struct OverloadedIdentity
    {
        string identity;
        vector<LookupValue> overloads;
    };

    wptr<Scope> parent;
    unordered_map<string, LookupValue> lookup;
};

struct Procedure
{
    Span span;
    string identity;
    ptr<Scope> scope;
    vector<ptr<Variable>> parameters;
    ptr<CodeBlock> body;
};

struct Variable
{
    Span span;
    string identity;
    Pattern pattern;
    bool is_constant;
};

// LITERALS

struct PrimitiveLiteral
{
    Span span;
    ptr<PrimitiveValue> value;
};

struct ListLiteral
{
    Span span;
    vector<Expression> values;
};

struct IdentityLiteral
{
    Span span;
    string identity;
};

struct OptionLiteral
{
    Span span;
    UnresolvedLiteral literal;
};

// VALUES

// TODO: Through the program, we maintain pointers to PrimitiveValues that have
//       been dynamically allocated to the heap. This is a bit naff. It would be
//       better if PrimitiveValue was treated like a regular value-struct, rather
//       than a APM node.
struct PrimitiveValue
{
    variant<double, int, bool, string> value;
    ptr<PrimitiveType> type;
};

struct ListValue
{
    vector<Expression> values;
};

struct EnumValue
{
    Span span; // The span where the enum value was declared
    string identity;
    ptr<EnumType> type;
};

// TYPES

struct PrimitiveType
{
    string identity;
    string cpp_identity;
};

struct ListType
{
    Pattern list_of;
    optional<Expression> fixed_size;
};

struct EnumType
{
    Span span;
    string identity;
    vector<ptr<EnumValue>> values;
};

struct EntityType
{
    Span span;
    string identity;
};

// PROPERTIES

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

// PATTERNS

struct PatternLiteral
{
    Span span;
    Pattern pattern;
};

// FIXME: Is there a better way of representing this?
// FIXME: Should "any" include "none"?
struct AnyPattern
{
};

// TODO: Current UnionPattern has an identity field and is part of the Scope::LookupValue variant
//       as when an enum is declared with both enums and intrinsic values, we represent that as
//       a UnionPattern declared in the scope. However, most UnionPatterns are anonymous.
//       I'm not sure if this is the best solution for this?
struct UnionPattern
{
    string identity;
    vector<Pattern> patterns;
};

struct UninferredPattern
{
};

struct InvalidPattern
{
};

// EXPRESSIONS

struct ExpressionLiteral
{
    Span span;
    Expression expr;
};

struct Unary
{
    Span span;
    string op; // FIXME: Make this an enum instead of a string
    Expression value;
};

struct Binary
{
    Span span;
    string op; // FIXME: Make this an enum instead of a string
    Expression lhs;
    Expression rhs;
};

// FIXME: I'm not sure if this node is really required? I think
//        it's just used temporarily when parsing property indexes
struct InstanceList
{
    Span span;
    vector<Expression> values;
};

struct ExpressionIndex
{
    Span span;
    Expression subject;
    Expression index;
};

struct PropertyIndex
{
    Span span;
    Expression expr;
    Property property;
};

struct Call
{
    Span span;
    struct Argument
    {
        Span span;
        bool named;
        string name;
        Expression value;
    };
    Expression callee;
    vector<Argument> arguments;
};

struct IfExpression
{
    Span span;
    struct Rule
    {
        Span span;
        Expression condition;
        Expression result;
    };
    vector<Rule> rules;
    bool has_else = false;
};

struct MatchExpression
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
    bool has_else = false;
};

struct InvalidExpression
{
};

// STATEMENTS

struct IfStatement
{
    Span span;
    struct Rule
    {
        Span span;
        Expression condition;
        ptr<CodeBlock> code_block;
    };
    vector<Rule> rules;
    optional<ptr<CodeBlock>> else_block;
};

struct ForStatement
{
    Span span;
    ptr<Variable> variable;
    Expression range;
    ptr<Scope> scope;
    ptr<CodeBlock> body;
};

struct LoopStatement
{
    Span span;
    ptr<Scope> scope;
    ptr<CodeBlock> body;
};

struct ReturnStatement
{
    Span span;
    Expression value;
};

struct AssignmentStatement
{
    Span span;
    Expression subject;
    Expression value;
};

struct VariableDeclaration
{
    Span span;
    ptr<Variable> variable;
    optional<Expression> value;
};

// APM METHODS

// Declaration and fetching
[[nodiscard]] string identity_of(Scope::LookupValue value);
[[nodiscard]] bool directly_declared_in_scope(ptr<Scope> scope, string identity);
[[nodiscard]] bool declared_in_scope(ptr<Scope> scope, string identity);
[[nodiscard]] bool is_overloadable(Scope::LookupValue value);

[[nodiscard]] Scope::LookupValue fetch(ptr<Scope> scope, string identity);
[[nodiscard]] vector<Scope::LookupValue> fetch_all_overloads(ptr<Scope> scope, string identity);

// Pattern analysis
[[nodiscard]] Pattern determine_expression_pattern(Expression expr);
[[nodiscard]] ptr<UnionPattern> create_union_pattern(Pattern a, Pattern b);
[[nodiscard]] bool is_pattern_subset_of_superset(Pattern subset, Pattern superset);
[[nodiscard]] bool do_patterns_overlap(Pattern a, Pattern b);
[[nodiscard]] bool is_pattern_optional(Pattern pattern);
[[nodiscard]] bool does_instance_list_match_parameters(ptr<InstanceList> instance_list, vector<ptr<Variable>> parameters);

// Spans
[[nodiscard]] Span get_span(UnresolvedLiteral stmt);
[[nodiscard]] Span get_span(Pattern pattern);
[[nodiscard]] Span get_span(Expression expr);
[[nodiscard]] Span get_span(Statement stmt);

[[nodiscard]] Span get_span(Scope::LookupValue value);

// JSON SERIALISATION

// Program
string to_json(const ptr<Program> &node, const size_t &depth = 0);
string to_json(const ptr<CodeBlock> &node, const size_t &depth = 0);
string to_json(const ptr<Scope> &node, const size_t &depth = 0);
string to_json(const Scope::LookupValue &node, const size_t &depth = 0);
string to_json(const ptr<Scope::OverloadedIdentity> &node, const size_t &depth = 0);

string to_json(const ptr<Procedure> &node, const size_t &depth = 0);
string to_json(const ptr<Variable> &node, const size_t &depth = 0);

// Literals
string to_json(const UnresolvedLiteral &node, const size_t &depth = 0);

string to_json(const ptr<PrimitiveLiteral> &node, const size_t &depth = 0);
string to_json(const ptr<ListLiteral> &node, const size_t &depth = 0);
string to_json(const ptr<IdentityLiteral> &node, const size_t &depth = 0);
string to_json(const ptr<OptionLiteral> &node, const size_t &depth = 0);

// Values
string to_json(const ptr<PrimitiveValue> &node, const size_t &depth = 0);
string to_json(const ptr<ListValue> &node, const size_t &depth = 0);
string to_json(const ptr<EnumValue> &node, const size_t &depth = 0);

// Types
string to_json(const ptr<PrimitiveType> &node, const size_t &depth = 0);
string to_json(const ptr<ListType> &node, const size_t &depth = 0);
string to_json(const ptr<EnumType> &node, const size_t &depth = 0);
string to_json(const ptr<EntityType> &node, const size_t &depth = 0);

// Properties
string to_json(const Property &node, const size_t &depth = 0);

string to_json(const ptr<StateProperty> &node, const size_t &depth = 0);
string to_json(const ptr<FunctionProperty> &node, const size_t &depth = 0);
string to_json(const ptr<InvalidProperty> &node, const size_t &depth = 0);

// Patterns
string to_json(const Pattern &node, const size_t &depth = 0);
string to_json(const ptr<PatternLiteral> &node, const size_t &depth = 0);

string to_json(const ptr<AnyPattern> &node, const size_t &depth = 0);
string to_json(const ptr<UnionPattern> &node, const size_t &depth = 0);

string to_json(const ptr<UninferredPattern> &node, const size_t &depth = 0);
string to_json(const ptr<InvalidPattern> &node, const size_t &depth = 0);

// Expressions
string to_json(const Expression &node, const size_t &depth = 0);
string to_json(const ptr<ExpressionLiteral> &node, const size_t &depth = 0);

string to_json(const ptr<Unary> &node, const size_t &depth = 0);
string to_json(const ptr<Binary> &node, const size_t &depth = 0);

string to_json(const ptr<InstanceList> &node, const size_t &depth = 0);
string to_json(const ptr<ExpressionIndex> &node, const size_t &depth = 0);
string to_json(const ptr<PropertyIndex> &node, const size_t &depth = 0);

string to_json(const ptr<Call> &node, const size_t &depth = 0);
string to_json(const Call::Argument &node, const size_t &depth = 0);

string to_json(const ptr<IfExpression> &node, const size_t &depth = 0);
string to_json(const IfExpression::Rule &node, const size_t &depth = 0);
string to_json(const ptr<MatchExpression> &node, const size_t &depth = 0);
string to_json(const MatchExpression::Rule &node, const size_t &depth = 0);

string to_json(const ptr<InvalidExpression> &node, const size_t &depth = 0);

// Statements
string to_json(const Statement &node, const size_t &depth = 0);

string to_json(const ptr<IfStatement> &node, const size_t &depth = 0);
string to_json(const IfStatement::Rule &node, const size_t &depth = 0);
string to_json(const ptr<ForStatement> &node, const size_t &depth = 0);
string to_json(const ptr<LoopStatement> &node, const size_t &depth = 0);
string to_json(const ptr<ReturnStatement> &node, const size_t &depth = 0);
string to_json(const ptr<AssignmentStatement> &node, const size_t &depth = 0);
string to_json(const ptr<VariableDeclaration> &node, const size_t &depth = 0);

#endif
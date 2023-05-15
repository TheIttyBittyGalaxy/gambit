#pragma once
#ifndef PARSER_H
#define PARSER_H

#include "apm.h"
#include "expression.h"
#include "span.h"
#include "token.h"
#include "utilty.h"
#include <vector>
using namespace std;

class Parser
{
public:
    ptr<Program> parse(Source &source);

private:
    ptr<Program> program = nullptr;
    Source *source;

    size_t current_token_index;
    size_t current_block_nesting;
    bool panic_mode = false;

    // UTILITY //
    Token current_token();
    Token previous_token();
    bool peek(Token::Kind kind);
    Token eat(Token::Kind kind);
    void skip();
    bool match(Token::Kind kind);
    Span to_span(Token token);

    bool end_of_file();
    void skip_whitespace();
    void skip_to_end_of_line();
    void skip_to_block_nesting(size_t target_nesting);
    void skip_to_end_of_current_block();

    void declare(ptr<Scope> scope, Scope::LookupValue value);

    // ERROR HANDLING //
    void gambit_error(string msg, size_t line, size_t column, initializer_list<Span> spans = {});
    void gambit_error(string msg, Token token);
    void gambit_error(string msg, Span span);
    void gambit_error(string msg, initializer_list<Span> spans);

    // PROGRAM STRUCTURE //
    void parse_program();

    bool peek_code_block(bool singleton_allowed = true);
    [[nodiscard]] ptr<CodeBlock> parse_code_block(ptr<Scope> scope);

    bool peek_enum_definition();
    void parse_enum_definition(ptr<Scope> scope);

    bool peek_entity_definition();
    void parse_entity_definition(ptr<Scope> scope);

    bool peek_state_property_definition();
    void parse_state_property_definition(ptr<Scope> scope);

    bool peek_function_property_definition();
    void parse_function_property_definition(ptr<Scope> scope);

    // STATEMENTS //
    bool peek_statement();
    [[nodiscard]] Statement parse_statement(ptr<Scope> scope);

    // EXPRESSIONS //
    bool operator_should_bind(Precedence operator_precedence, Precedence caller_precedence, bool left_associative = true);
    [[nodiscard]] ptr<UnresolvedIdentity> parse_unresolved_identity();

    bool peek_expression();
    Expression parse_expression(Precedence precedence = Precedence::None);

    bool peek_paren_expr();
    Expression parse_paren_expr();
    bool peek_match();
    [[nodiscard]] ptr<Match> parse_match();
    bool peek_unary();
    [[nodiscard]] ptr<Unary> parse_unary();
    bool peek_literal();
    [[nodiscard]] Expression parse_literal();
    bool peek_list_value();
    [[nodiscard]] ptr<ListValue> parse_list_value();

    bool peek_infix_logical_or();
    [[nodiscard]] ptr<Binary> parse_infix_logical_or(Expression lhs);
    bool peek_infix_logical_and();
    [[nodiscard]] ptr<Binary> parse_infix_logical_and(Expression lhs);
    bool peek_infix_term();
    [[nodiscard]] ptr<Binary> parse_infix_term(Expression lhs);
    bool peek_infix_factor();
    [[nodiscard]] ptr<Binary> parse_infix_factor(Expression lhs);
    bool peek_infix_property_index();
    [[nodiscard]] ptr<PropertyIndex> parse_infix_property_index(Expression lhs);

    // PATTERNS //
    bool peek_pattern();
    Pattern parse_pattern(ptr<Scope> scope);
};

#endif
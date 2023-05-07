#ifndef PARSER_H
#define PARSER_H

#include "apm.h"
#include "expression.h"
#include "token.h"
#include "utilty.h"
#include <vector>
using namespace std;

class Parser
{
public:
    ptr<Program> parse(vector<Token> new_tokens);

private:
    vector<Token> tokens;
    ptr<Program> program = nullptr;
    size_t current_token_index;
    size_t current_block_nesting;

    Token current_token();
    bool peek(Token::Kind kind);
    Token eat(Token::Kind kind);
    void skip();
    bool match(Token::Kind kind);

    bool end_of_file();
    void skip_whitespace();
    void skip_to_end_of_line();
    void skip_to_block_nesting(size_t target_nesting);
    void skip_to_end_of_current_block();

    void parse_program();

    bool peek_code_block();
    ptr<CodeBlock> parse_code_block(ptr<Scope> scope);

    ptr<UnresolvedIdentity> parse_unresolved_identity();

    bool peek_enum_definition();
    void parse_enum_definition(ptr<Scope> scope);

    bool peek_entity_definition();
    void parse_entity_definition(ptr<Scope> scope);

    bool peek_state_property_definition();
    ptr<StateProperty> parse_state_property_definition(ptr<Scope> scope);
    bool peek_function_property_definition();
    ptr<FunctionProperty> parse_function_property_definition(ptr<Scope> scope);

    bool peek_pattern();
    ptr<Pattern> parse_pattern(ptr<Scope> scope);
    bool peek_pattern_list();
    ptr<PatternList> parse_pattern_list(ptr<Scope> scope);

    bool peek_type();
    Type parse_type(ptr<Scope> scope);

    bool operator_should_bind(Precedence operator_precedence, Precedence caller_precedence, bool left_associative = true);
    bool peek_expression();
    Expression parse_expression(Precedence precedence = Precedence::None);
    bool peek_paren_expr();
    Expression parse_paren_expr();
    bool peek_match();
    ptr<Match> parse_match();
    bool peek_unary();
    ptr<Unary> parse_unary();
    bool peek_literal();
    ptr<Literal> parse_literal();
    bool peek_list_value();
    ptr<ListValue> parse_list_value();

    bool peek_infix_logical_or();
    ptr<Binary> parse_infix_logical_or(Expression lhs);
    bool peek_infix_logical_and();
    ptr<Binary> parse_infix_logical_and(Expression lhs);
    bool peek_infix_term();
    ptr<Binary> parse_infix_term(Expression lhs);
    bool peek_infix_factor();
    ptr<Binary> parse_infix_factor(Expression lhs);
    bool peek_infix_property_index();
    ptr<PropertyIndex> parse_infix_property_index(Expression lhs);

    bool peek_statement();
    Statement parse_statement(ptr<Scope> scope);
};

#endif
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
    size_t current_token;
    size_t current_depth;

    bool end_of_file();
    bool peek(Token::Kind kind);
    Token eat(Token::Kind kind);
    bool match(Token::Kind kind);

    void skip_whitespace();
    void skip_to_end_of_line();
    void skip_to_depth(size_t target_depth);
    void skip_to_end_of_current_depth();

    void parse_program();

    ptr<UnresolvedIdentity> parse_unresolved_identity();

    bool peek_enum_definition();
    void parse_enum_definition(ptr<Scope> scope);

    bool peek_entity_definition();
    void parse_entity_definition(ptr<Scope> scope);

    bool peek_entity_field();
    void parse_entity_field(ptr<Scope> scope, ptr<Entity> entity);

    bool peek_type();
    Type parse_type(ptr<Scope> scope);

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

    bool peek_term();
    ptr<Binary> parse_term(Expression lhs);
    bool peek_factor();
    ptr<Binary> parse_factor(Expression lhs);
    ptr<Binary> parse_binary(Expression lhs, Precedence precedence);
};

#endif
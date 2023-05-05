#include "errors.h"
#include "parser.h"

class Error : public std::exception
{
public:
    string msg;
    Token token;

    Error(string msg, Token token) : msg(msg), token(token){};

    string what()
    {
        return msg;
    }
};

ptr<Program> Parser::parse(vector<Token> new_tokens)
{
    tokens = new_tokens;
    current_token = 0;
    current_depth = 0;

    parse_program();
    return program;
}

bool Parser::end_of_file()
{
    return current_token >= tokens.size();
}

bool Parser::peek(Token::Kind kind)
{
    if (end_of_file())
        return false;

    if (kind == Token::Line)
        return tokens.at(current_token).kind == kind;

    size_t i = current_token;
    while (tokens.at(i).kind == Token::Line)
        i++;
    return tokens.at(i).kind == kind;
}

Token Parser::eat(Token::Kind kind)
{
    if (end_of_file())
    {
        Token token = tokens.at(tokens.size() - 1);
        throw Error("Expected " + token_name.at(kind) + ", got end of file", token);
    }

    Token token = tokens.at(current_token);

    if (!peek(kind))
    {
        Token::Kind other = token.kind;
        throw Error("Expected " + token_name.at(kind) + ", got " + token_name.at(other), token);
    }

    if (kind != Token::Line)
    {
        while (token.kind == Token::Line)
        {
            current_token++;
            token = tokens.at(current_token);
        }
    }

    if (kind == Token::CurlyL)
        current_depth++;
    else if (kind == Token::CurlyR && current_depth > 0)
        current_depth--;

    current_token++;
    return token;
}

bool Parser::match(Token::Kind kind)
{
    if (peek(kind))
    {
        eat(kind);
        return true;
    }
    return false;
}

void Parser::skip_whitespace()
{
    while (match(Token::Line))
        ;
}

void Parser::skip_to_end_of_line()
{
    while (!end_of_file() && !match(Token::Line))
        eat(tokens.at(current_token).kind);
}

void Parser::skip_to_depth(size_t target_depth)
{
    while (!end_of_file() && current_depth != target_depth)
        eat(tokens.at(current_token).kind);
}

void Parser::skip_to_end_of_current_depth()
{
    skip_to_depth(current_depth - 1);
}

void Parser::parse_program()
{
    program = CREATE(Program);
    program->global_scope = CREATE(Scope);

    {
        auto gambit_bool = CREATE(NativeType);
        gambit_bool->identity = "bool";
        gambit_bool->cpp_identity = "bool";
        declare(program->global_scope, gambit_bool);

        auto gambit_int = CREATE(NativeType);
        gambit_int->identity = "int";
        gambit_int->cpp_identity = "int";
        declare(program->global_scope, gambit_int);

        auto gambit_num = CREATE(NativeType);
        gambit_num->identity = "num";
        gambit_num->cpp_identity = "num";
        declare(program->global_scope, gambit_num);

        auto gambit_string = CREATE(NativeType);
        gambit_string->identity = "string";
        gambit_string->cpp_identity = "string";
        declare(program->global_scope, gambit_string);
    }

    while (!end_of_file())
    {
        try
        {
            skip_whitespace();
            if (end_of_file())
                break;

            if (peek_entity_definition())
                parse_entity_definition(program->global_scope);
            else if (peek_enum_definition())
                parse_enum_definition(program->global_scope);
            else
                throw Error("Unexpected '" + tokens.at(current_token).str + "' in global scope.", tokens.at(current_token));
        }
        catch (Error err)
        {
            emit_error(err.msg, err.token);
            skip_to_end_of_line();
            skip_to_depth(0);
        }
    }
}

ptr<UnresolvedIdentity> Parser::parse_unresolved_identity()
{
    auto identity = CREATE(UnresolvedIdentity);
    Token token = eat(Token::Identity);
    identity->token = token;
    identity->identity = token.str;

    return identity;
}

bool Parser::peek_enum_definition()
{
    return peek(Token::KeyEnum);
}

void Parser::parse_enum_definition(ptr<Scope> scope)
{
    auto enum_type = CREATE(EnumType);

    eat(Token::KeyEnum);
    enum_type->identity = eat(Token::Identity).str;
    declare(scope, enum_type);

    eat(Token::CurlyL);
    do
    {
        auto enum_value = CREATE(EnumValue);
        enum_value->identity = eat(Token::Identity).str;
        enum_type->values.emplace_back(enum_value);
    } while (match(Token::Comma));

    eat(Token::CurlyR);
}

bool Parser::peek_entity_definition()
{
    return peek(Token::KeyEntity);
}

void Parser::parse_entity_definition(ptr<Scope> scope)
{
    auto entity = CREATE(Entity);

    eat(Token::KeyEntity);
    entity->identity = eat(Token::Identity).str;
    declare(scope, entity);

    eat(Token::Line);
}

bool Parser::peek_type()
{
    return peek(Token::Identity);
}

Type Parser::parse_type(ptr<Scope> scope)
{
    Type type = parse_unresolved_identity();

    if (match(Token::Question))
    {
        auto opt = CREATE(OptionalType);
        opt->type = type;
        type = opt;
    }

    return type;
}

bool Parser::peek_expression()
{
    return peek_paren_expr() || peek_unary() || peek_literal();
}

// Parses any expression of the given precedence or higher
// FIXME: This does not correctly handle associativity.
//        e.g. 1/2/3 is interpreted as 1/(2/3), not (1/2)/3
Expression Parser::parse_expression(Precedence precedence)
{
    // Prefix expressions
    Expression expr;
    if (peek_paren_expr())
        expr = parse_paren_expr();
    else if (peek_match())
        expr = parse_match();
    else if (peek_unary())
        expr = parse_unary();
    else if (peek(Token::Identity))
        expr = parse_unresolved_identity();
    else if (peek_literal())
        expr = parse_literal();
    else if (peek_list_value())
        expr = parse_list_value();
    else
        throw Error("Expected expression", tokens.at(current_token));

    while (true)
    {
        if (peek_factor() && precedence <= Precedence::Factor)
            expr = parse_factor(expr);
        else if (peek_term() && precedence <= Precedence::Term)
            expr = parse_term(expr);
        else
            break;
    }

    return expr;
}

bool Parser::peek_paren_expr()
{
    return peek(Token::ParenL);
}

Expression Parser::parse_paren_expr()
{
    eat(Token::ParenL);
    auto expr = parse_expression();
    eat(Token::ParenR);
    return expr;
}

bool Parser::peek_match()
{
    return peek(Token::KeyMatch);
}

ptr<Match> Parser::parse_match()
{
    auto match = CREATE(Match);

    eat(Token::KeyMatch);
    match->subject = parse_expression();

    eat(Token::CurlyL);
    while (peek_expression())
    {
        auto pattern = parse_expression();
        eat(Token::Colon);
        auto result = parse_expression();
        match->rules.emplace_back(Match::Rule{pattern, result});
    }
    eat(Token::CurlyR);

    return match;
}

bool Parser::peek_unary()
{
    return peek(Token::Add) ||
           peek(Token::Sub) ||
           peek(Token::KeyNot);
}

ptr<Unary> Parser::parse_unary()
{
    auto expr = CREATE(Unary);

    if (peek(Token::Add))
        expr->op = eat(Token::Add).str;
    else if (peek(Token::Sub))
        expr->op = eat(Token::Sub).str;
    else if (peek(Token::KeyNot))
        expr->op = eat(Token::KeyNot).str;
    else
        throw Error("Expected unary expression", tokens.at(current_token)); // FIXME: Should this be a compiler error rather than a language error?

    expr->value = parse_expression(Precedence::Unary);

    return expr;
}

bool Parser::peek_literal()
{
    return peek(Token::Number) ||
           peek(Token::String) ||
           peek(Token::Boolean);
}

ptr<Literal> Parser::parse_literal()
{
    auto literal = CREATE(Literal);
    if (peek(Token::Number))
    {
        // FIXME: Treat num and int literals differently
        Token t = eat(Token::Number);
        literal->value = stod(t.str);
    }
    else if (peek(Token::String))
    {
        Token t = eat(Token::String);
        literal->value = t.str;
    }
    else if (peek(Token::Boolean))
    {
        Token t = eat(Token::Boolean);
        literal->value = t.str == "true";
    }
    else
    {
        throw Error("Expected literal", tokens.at(current_token));
    }

    return literal;
}

bool Parser::peek_list_value()
{
    return peek(Token::SquareL);
}

ptr<ListValue> Parser::parse_list_value()
{
    auto list = CREATE(ListValue);
    eat(Token::SquareL);
    if (peek_expression())
    {
        do
        {
            list->values.push_back(parse_expression());
        } while (match(Token::Comma));
    }
    eat(Token::SquareR);
    return list;
}

bool Parser::peek_term()
{
    return peek(Token::Add) ||
           peek(Token::Sub);
}

ptr<Binary> Parser::parse_term(Expression lhs)
{
    return parse_binary(lhs, Precedence::Term);
}

bool Parser::peek_factor()
{
    return peek(Token::Mul) ||
           peek(Token::Div);
}

ptr<Binary> Parser::parse_factor(Expression lhs)
{
    return parse_binary(lhs, Precedence::Factor);
}

ptr<Binary> Parser::parse_binary(Expression lhs, Precedence precedence)
{
    auto expr = CREATE(Binary);
    expr->lhs = lhs;

    if (peek(Token::Add) && precedence <= Precedence::Term)
        expr->op = eat(Token::Add).str;
    else if (peek(Token::Sub) && precedence <= Precedence::Term)
        expr->op = eat(Token::Sub).str;
    else if (peek(Token::Mul) && precedence <= Precedence::Factor)
        expr->op = eat(Token::Mul).str;
    else if (peek(Token::Div) && precedence <= Precedence::Factor)
        expr->op = eat(Token::Div).str;
    else
        throw Error("Expected binary expression", tokens.at(current_token)); // FIXME: Should this be a compiler error rather than a language error?

    expr->rhs = parse_expression(precedence);

    return expr;
}

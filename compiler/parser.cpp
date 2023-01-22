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
};

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
};

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
};

bool Parser::match(Token::Kind kind)
{
    if (peek(kind))
    {
        eat(kind);
        return true;
    }
    return false;
};

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
                throw Error("Expected Entity definition", tokens.at(current_token));
        }
        catch (Error err)
        {
            emit_error(err.msg, err.token);
            skip_to_end_of_line();
            skip_to_depth(0);
        }
    }
};

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
    return peek(Token::KeyEntity) || peek(Token::KeyExtend);
}

void Parser::parse_entity_definition(ptr<Scope> scope)
{
    bool is_base_definition = !match(Token::KeyExtend);
    if (is_base_definition)
        eat(Token::KeyEntity);

    Token identity = eat(Token::Identity);

    ptr<Entity> entity;

    if (is_base_definition || !declared_in_scope(scope, identity.str))
    {
        entity = CREATE(Entity);
        entity->identity = identity.str;
        declare(scope, entity);
    }
    else
    {
        entity = fetch_entity(scope, identity.str);
    }

    entity->base_definition_found = is_base_definition;

    if (peek(Token::SquareL))
    {
        if (!is_base_definition)
            throw Error("Cannot define an Entity's signature on an additive definition.", tokens.at(current_token));

        eat(Token::SquareL);
        do
        {
            auto identity = eat(Token::Identity).str;
            entity->signature.emplace_back(identity);
        } while (match(Token::Comma));
        eat(Token::SquareR);
    }

    eat(Token::CurlyL);
    while (peek_entity_field())
    {
        try
        {
            parse_entity_field(scope, entity);
            eat(Token::Line);
        }
        catch (Error err)
        {
            emit_error(err.msg, err.token);
            skip_to_end_of_line();
        }
    }
    eat(Token::CurlyR);
};

bool Parser::peek_entity_field()
{
    return peek(Token::KeyStatic) || peek(Token::KeyState) || peek(Token::KeyProperty);
}

void Parser::parse_entity_field(ptr<Scope> scope, ptr<Entity> entity)
{
    auto field = CREATE(EntityField);
    if (match(Token::KeyStatic))
        field->is_static = true;
    else if (match(Token::KeyProperty))
        field->is_property = true;
    else
        eat(Token::KeyState);

    field->type = parse_type(scope);
    field->identity = eat(Token::Identity).str;

    entity->fields.insert({field->identity, field});

    if (match(Token::Assign))
        field->default_value = parse_expression();
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
    return peek_literal();
}

Expression Parser::parse_expression()
{
    if (peek_literal())
    {
        return parse_literal();
    }

    throw Error("Expected expression", tokens.at(current_token));
}

bool Parser::peek_literal()
{
    return peek(Token::Number) || peek(Token::String) || peek(Token::Boolean);
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

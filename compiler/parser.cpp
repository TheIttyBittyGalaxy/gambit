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
    current_token_index = 0;
    current_block_nesting = 0;

    parse_program();
    return program;
}

Token Parser::current_token()
{
    if (current_token_index >= tokens.size())
        return tokens.back();
    return tokens.at(current_token_index);
}

bool Parser::peek(Token::Kind kind)
{
    Token token = current_token();

    if (token.kind == kind)
        return true;

    // Look ahead of line tokens if we are attempting to peek a non-line token
    if (kind != Token::Line && token.kind != Token::EndOfFile)
    {
        size_t i = current_token_index;
        while (tokens.at(i).kind == Token::Line)
            i++;
        return tokens.at(i).kind == kind;
    }

    return false;
}

Token Parser::eat(Token::Kind kind)
{
    if (!peek(kind))
    {
        Token token = current_token();
        throw Error("Expected " + token_name.at(kind) + ", got " + token_name.at(token.kind), token);
    }

    // Skip line tokens if we are attempting to eat a non-line token
    if (kind != Token::Line)
        while (current_token().kind == Token::Line)
            current_token_index++;

    Token token = current_token();

    if (kind == Token::CurlyL)
        current_block_nesting++;
    else if (kind == Token::CurlyR && current_block_nesting > 0)
        current_block_nesting--;

    current_token_index++;
    return token;
}

void Parser::skip()
{
    Token::Kind kind = current_token().kind;

    if (kind == Token::CurlyL)
        current_block_nesting++;
    else if (kind == Token::CurlyR && current_block_nesting > 0)
        current_block_nesting--;

    current_token_index++;
}

bool Parser::match(Token::Kind kind)
{
    if (!peek(kind))
        return false;

    eat(kind);
    return true;
}

bool Parser::end_of_file()
{
    return peek(Token::EndOfFile);
}

void Parser::skip_whitespace()
{
    while (match(Token::Line))
        ;
}

void Parser::skip_to_end_of_line()
{
    while (!end_of_file() && !match(Token::Line))
        skip();
}

void Parser::skip_to_block_nesting(size_t target_nesting)
{
    while (!end_of_file() && current_block_nesting != target_nesting)
        skip();
}

void Parser::skip_to_end_of_current_block()
{
    skip_to_block_nesting(current_block_nesting - 1);
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

    while (true)
    {
        if (match(Token::EndOfFile))
            break;

        try
        {
            if (peek_entity_definition())
                parse_entity_definition(program->global_scope);
            else if (peek_enum_definition())
                parse_enum_definition(program->global_scope);
            else if (peek_state_property_definition())
                parse_state_property_definition(program->global_scope);
            else if (peek_function_property_definition())
                parse_function_property_definition(program->global_scope);
            else
            {
                skip_whitespace();
                throw Error("Unexpected '" + current_token().str + "' in global scope.", current_token());
            }
        }
        catch (Error err)
        {
            emit_error(err.msg, err.token);
            skip_to_end_of_line();
            skip_to_block_nesting(0);
        }
    }
}

bool Parser::peek_code_block()
{
    return peek(Token::CurlyL) ||
           peek(Token::Colon);
}

ptr<CodeBlock> Parser::parse_code_block(ptr<Scope> scope)
{
    auto code_block = CREATE(CodeBlock);
    code_block->scope = CREATE(Scope);
    code_block->scope->parent = scope;

    if (match(Token::Colon))
    {
        // FIXME: Do not allow the statement of a singleton code block to another code block
        auto statement = parse_statement(code_block->scope);
        code_block->statements.emplace_back(statement);
        code_block->singleton_block = true;
    }
    else
    {
        eat(Token::CurlyL);
        eat(Token::Line);
        while (!match(Token::CurlyR))
        {
            auto statement = parse_statement(code_block->scope);
            code_block->statements.emplace_back(statement);
        }
    }

    return code_block;
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

bool Parser::peek_state_property_definition()
{
    return peek(Token::KeyState);
}

ptr<StateProperty> Parser::parse_state_property_definition(ptr<Scope> scope)
{
    auto state = CREATE(StateProperty);

    eat(Token::KeyState);
    state->pattern = parse_pattern(scope);
    state->pattern_list = parse_pattern_list(scope);
    eat(Token::Dot);
    state->identity = eat(Token::Identity).str;

    declare(scope, state);

    if (match(Token::Assign))
        state->initial_value = parse_expression();

    return state;
}

bool Parser::peek_function_property_definition()
{
    return peek(Token::KeyFn);
}

ptr<FunctionProperty> Parser::parse_function_property_definition(ptr<Scope> scope)
{
    auto funct = CREATE(FunctionProperty);

    eat(Token::KeyFn);
    funct->pattern = parse_pattern(scope);
    funct->pattern_list = parse_pattern_list(scope);
    eat(Token::Dot);
    funct->identity = eat(Token::Identity).str;

    declare(scope, funct);

    if (peek_code_block())
        funct->body = parse_code_block(scope);

    return funct;
}

bool Parser::peek_named_pattern()
{
    return peek_pattern();
}

ptr<NamedPattern> Parser::parse_named_pattern(ptr<Scope> scope)
{
    auto named_pattern = CREATE(NamedPattern);

    named_pattern->pattern = parse_pattern(scope);
    named_pattern->name = eat(Token::Identity).str;

    return named_pattern;
}

bool Parser::peek_pattern_list()
{
    return peek(Token::ParenL);
}

ptr<PatternList> Parser::parse_pattern_list(ptr<Scope> scope)
{
    auto pattern_list = CREATE(PatternList);

    eat(Token::ParenL);
    do
    {
        auto pattern = parse_named_pattern(scope);
        pattern_list->patterns.emplace_back(pattern);
    } while (match(Token::Comma));
    eat(Token::ParenR);

    return pattern_list;
}

bool Parser::peek_pattern()
{
    return peek(Token::Identity);
}

Pattern Parser::parse_pattern(ptr<Scope> scope)
{
    Pattern pattern = parse_unresolved_identity();

    if (match(Token::Question))
    {
        auto optional_pattern = CREATE(OptionalPattern);
        optional_pattern->pattern = pattern;
        pattern = optional_pattern;
    }

    return pattern;
}

bool Parser::peek_expression()
{
    return peek_paren_expr() ||
           peek_match() ||
           peek_unary() ||
           peek(Token::Identity) ||
           peek_literal() ||
           peek_list_value();
}

bool Parser::operator_should_bind(Precedence operator_precedence, Precedence caller_precedence, bool left_associative)
{
    if (left_associative)
        return operator_precedence > caller_precedence;
    else
        return operator_precedence >= caller_precedence;
}

Expression Parser::parse_expression(Precedence caller_precedence)
{
    // Prefix expressions
    Expression lhs;

    // FIXME: Are `operator_should_bind` checks required for any of these 'nud' nodes?
    //
    //        I've added it to the unary check, as this way it is made left-associative,
    //        making expressions such as `--1` illegal. I think this is a good way to go?
    //
    //        I think most of the remaining nuds can be left without a check, as they all
    //        represent values, and so defacto have the highest precedence. Only one I
    //        have no idea about is `match`.
    if (peek_unary() && operator_should_bind(Precedence::Unary, caller_precedence))
        lhs = parse_unary();
    else if (peek_match())
        lhs = parse_match();

    else if (peek(Token::Identity))
        lhs = parse_unresolved_identity();
    else if (peek_paren_expr())
        lhs = parse_paren_expr();
    else if (peek_literal())
        lhs = parse_literal();
    else if (peek_list_value())
        lhs = parse_list_value();

    else
        throw Error("Expected expression", current_token());

    while (true)
    {
        if (peek_infix_factor() && operator_should_bind(Precedence::Factor, caller_precedence))
            lhs = parse_infix_factor(lhs);
        else if (peek_infix_term() && operator_should_bind(Precedence::Term, caller_precedence))
            lhs = parse_infix_term(lhs);
        else if (peek_infix_property_index() && operator_should_bind(Precedence::Index, caller_precedence))
            lhs = parse_infix_property_index(lhs);
        if (peek_infix_logical_and() && operator_should_bind(Precedence::LogicalAnd, caller_precedence))
            lhs = parse_infix_logical_and(lhs);
        else if (peek_infix_logical_or() && operator_should_bind(Precedence::LogicalOr, caller_precedence))
            lhs = parse_infix_logical_or(lhs);
        else
            break;
    }

    return lhs;
}

bool Parser::peek_paren_expr()
{
    return peek(Token::ParenL);
}

Expression Parser::parse_paren_expr()
{
    eat(Token::ParenL);
    auto expr = parse_expression();
    if (!peek(Token::Comma))
    {
        eat(Token::ParenR);
        return expr;
    }

    auto instance_list = CREATE(InstanceList);
    instance_list->values.emplace_back(expr);

    while (match(Token::Comma))
        instance_list->values.emplace_back(parse_expression());
    eat(Token::ParenR);

    auto property_index = parse_infix_property_index(instance_list);
    return property_index;
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
        throw Error("Expected unary expression", current_token()); // FIXME: Should this be a compiler error rather than a language error?

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
        throw Error("Expected literal", current_token());
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

bool Parser::peek_infix_logical_or()
{
    return peek(Token::KeyOr);
}

ptr<Binary> Parser::parse_infix_logical_or(Expression lhs)
{
    auto expr = CREATE(Binary);
    expr->lhs = lhs;
    expr->op = eat(Token::KeyOr).str;
    expr->rhs = parse_expression(Precedence::LogicalOr);
    return expr;
}

bool Parser::peek_infix_logical_and()
{
    return peek(Token::KeyAnd);
}

ptr<Binary> Parser::parse_infix_logical_and(Expression lhs)
{
    auto expr = CREATE(Binary);
    expr->lhs = lhs;
    expr->op = eat(Token::KeyAnd).str;
    expr->rhs = parse_expression(Precedence::LogicalAnd);
    return expr;
}

bool Parser::peek_infix_term()
{
    return peek(Token::Add) ||
           peek(Token::Sub);
}

ptr<Binary> Parser::parse_infix_term(Expression lhs)
{
    auto expr = CREATE(Binary);
    expr->lhs = lhs;

    if (peek(Token::Add))
        expr->op = eat(Token::Add).str;
    else if (peek(Token::Sub))
        expr->op = eat(Token::Sub).str;
    else
        throw Error("Expected infix term expression", current_token()); // FIXME: Should this be a compiler error rather than a language error?

    expr->rhs = parse_expression(Precedence::Term);

    return expr;
}

bool Parser::peek_infix_factor()
{
    return peek(Token::Mul) ||
           peek(Token::Div);
}

ptr<Binary> Parser::parse_infix_factor(Expression lhs)
{
    auto expr = CREATE(Binary);
    expr->lhs = lhs;

    if (peek(Token::Mul))
        expr->op = eat(Token::Mul).str;
    else if (peek(Token::Div))
        expr->op = eat(Token::Div).str;
    else
        throw Error("Expected infix factor expression", current_token()); // FIXME: Should this be a compiler error rather than a language error?

    expr->rhs = parse_expression(Precedence::Factor);

    return expr;
}

bool Parser::peek_infix_property_index()
{
    return peek(Token::Dot);
}

ptr<PropertyIndex> Parser::parse_infix_property_index(Expression lhs)
{
    // TODO: As of writing, InstanceLists are only used to collect values that will become the
    //       lhs of a PropertyIndex. If that remains the case, maybe it would be worth (at this
    //       point in the code) transfering all of the elements of instance_list->values into a
    //       'lhs' ('subjects'?) vector<Expression> on the PropertyList? This way there isn't a
    //       reduntant InstanceList node just hanging around in the program model on each index?

    if (!IS_PTR(lhs, InstanceList))
    {
        auto instance_list = CREATE(InstanceList);
        instance_list->values.emplace_back(lhs);
        lhs = instance_list;
    }

    auto property_index = CREATE(PropertyIndex);
    property_index->expr = lhs;
    eat(Token::Dot);
    property_index->property = parse_unresolved_identity();
    return property_index;
}

bool Parser::peek_statement()
{
    return peek_code_block() ||
           peek_expression();
}

Statement Parser::parse_statement(ptr<Scope> scope)
{
    // FIXME: Do not allow singleton code blocks to also be statements

    Statement stmt;
    if (peek_code_block())
        stmt = parse_code_block(scope);
    else if (peek_expression())
        stmt = parse_expression();
    else
        throw Error("Expected statement", current_token());

    eat(Token::Line);
    return stmt;
}

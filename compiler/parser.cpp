#include "errors.h"
#include "intrinsic.h"
#include "parser.h"

ptr<Program> Parser::parse(vector<Token> new_tokens, Source *new_source)
{
    tokens = new_tokens;
    source = new_source;
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

// UTILITY //

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
        throw GambitError("Expected " + token_name.at(kind) + ", got " + token_name.at(token.kind), token);
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

    if (start_span_on_next_eat)
    {
        span_stack.emplace_back(Span(token.line, token.column, token.position, 0, source));
        start_span_on_next_eat = false;
    }

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

void Parser::start_span()
{
    // Starting the span is differed, as the current token at this point may be a newline,
    // that could potentially be skipped over by eat. If this token were part of the span,
    // it and any subsequent new lines and comments would be included in the span.
    start_span_on_next_eat = true;
}

void Parser::start_span(Span start)
{
    span_stack.emplace_back(Span(start.line, start.column, start.position, 0, source));
}

Span Parser::end_span()
{
    Span span = span_stack.back();
    span_stack.pop_back();
    span.length = current_token().position - span.position;
    return span;
}

// PROGRAM STRUCTURE //

void Parser::parse_program()
{
    program = CREATE(Program);
    program->global_scope = CREATE(Scope);

    declare(program->global_scope, Intrinsic::type_str);
    declare(program->global_scope, Intrinsic::type_num);
    declare(program->global_scope, Intrinsic::type_int);
    declare(program->global_scope, Intrinsic::type_amt);
    declare(program->global_scope, Intrinsic::type_bool);

    declare(program->global_scope, Intrinsic::entity_player);
    declare(program->global_scope, Intrinsic::state_player_number);

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
                throw GambitError("Unexpected '" + current_token().str + "' in global scope.", current_token());
            }
        }
        catch (GambitError err)
        {
            skip_to_end_of_line();
            skip_to_block_nesting(0);
        }
    }
}

bool Parser::peek_code_block(bool singleton_allowed)
{
    return peek(Token::CurlyL) ||
           peek(Token::Colon) && singleton_allowed;
}

ptr<CodeBlock> Parser::parse_code_block(ptr<Scope> scope)
{
    start_span();
    auto code_block = CREATE(CodeBlock);
    code_block->scope = CREATE(Scope);
    code_block->scope->parent = scope;

    if (match(Token::Colon))
    {
        auto statement = parse_statement(code_block->scope);
        code_block->statements.emplace_back(statement);
        code_block->singleton_block = true;

        if (IS_PTR(statement, CodeBlock))
        {
            // FIXME: Include a relevant span instead of an invalid token
            auto code_block_statement = AS_PTR(statement, CodeBlock);
            if (code_block_statement->singleton_block)
                throw GambitError("Too many colons.", Token());
            else
                throw GambitError("Syntax `: { ... }` is invalid. Either use `: ... ` for a single statement, or `{ ... }` for multiple statements.", Token());
        }
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

    code_block->span = end_span();
    return code_block;
}

bool Parser::peek_enum_definition()
{
    return peek(Token::KeyEnum);
}

void Parser::parse_enum_definition(ptr<Scope> scope)
{
    start_span();
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
    enum_type->span = end_span();
}

bool Parser::peek_entity_definition()
{
    return peek(Token::KeyEntity);
}

void Parser::parse_entity_definition(ptr<Scope> scope)
{
    start_span();
    auto entity = CREATE(Entity);

    eat(Token::KeyEntity);
    entity->identity = eat(Token::Identity).str;
    declare(scope, entity);

    entity->span = end_span();
    eat(Token::Line);
}

bool Parser::peek_state_property_definition()
{
    return peek(Token::KeyState);
}

void Parser::parse_state_property_definition(ptr<Scope> scope)
{
    start_span();
    auto state = CREATE(StateProperty);
    state->scope = CREATE(Scope);
    state->scope->parent = scope;

    eat(Token::KeyState);
    state->pattern = parse_pattern(scope);

    eat(Token::ParenL);
    do
    {
        start_span();
        auto parameter = CREATE(Variable);
        parameter->pattern = parse_pattern(scope);
        parameter->identity = eat(Token::Identity).str;
        parameter->span = end_span();

        state->parameters.emplace_back(parameter);
        declare(state->scope, parameter);
    } while (match(Token::Comma));
    eat(Token::ParenR);

    eat(Token::Dot);
    state->identity = eat(Token::Identity).str;

    declare(scope, state);

    state->span = end_span();

    if (match(Token::Colon))
        state->initial_value = parse_expression();
}

bool Parser::peek_function_property_definition()
{
    return peek(Token::KeyFn);
}

void Parser::parse_function_property_definition(ptr<Scope> scope)
{
    start_span();
    auto funct = CREATE(FunctionProperty);
    funct->scope = CREATE(Scope);
    funct->scope->parent = scope;

    eat(Token::KeyFn);
    funct->pattern = parse_pattern(scope);

    eat(Token::ParenL);
    do
    {
        start_span();
        auto parameter = CREATE(Variable);
        parameter->pattern = parse_pattern(scope);
        parameter->identity = eat(Token::Identity).str;
        parameter->span = end_span();

        funct->parameters.emplace_back(parameter);
        declare(funct->scope, parameter);
    } while (match(Token::Comma));
    eat(Token::ParenR);

    eat(Token::Dot);
    funct->identity = eat(Token::Identity).str;

    declare(scope, funct);

    funct->span = end_span();

    if (peek_code_block())
        funct->body = parse_code_block(funct->scope);
}

// STATEMENTS //

bool Parser::peek_statement()
{
    return peek_code_block() ||
           peek_expression();
}

Statement Parser::parse_statement(ptr<Scope> scope)
{
    Statement stmt;
    if (peek_code_block(false))
        stmt = parse_code_block(scope);
    else if (peek_expression())
        stmt = parse_expression();
    else
        throw GambitError("Expected statement", current_token());

    if (!match(Token::EndOfFile))
        eat(Token::Line);

    return stmt;
}

// EXPRESSIONS //

bool Parser::operator_should_bind(Precedence operator_precedence, Precedence caller_precedence, bool left_associative)
{
    if (left_associative)
        return operator_precedence > caller_precedence;
    else
        return operator_precedence >= caller_precedence;
}

ptr<UnresolvedIdentity> Parser::parse_unresolved_identity()
{
    auto identity = CREATE(UnresolvedIdentity);
    Token token = eat(Token::Identity);
    identity->span = Span(token);
    identity->identity = token.str;

    return identity;
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
        throw GambitError("Expected expression", current_token());

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
    Token start_token = eat(Token::ParenL);
    auto expr = parse_expression();
    if (!peek(Token::Comma))
    {
        eat(Token::ParenR);
        return expr;
    }

    start_span(Span(start_token));
    auto instance_list = CREATE(InstanceList);
    instance_list->values.emplace_back(expr);

    while (match(Token::Comma))
        instance_list->values.emplace_back(parse_expression());

    eat(Token::ParenR);
    instance_list->span = end_span();

    auto property_index = parse_infix_property_index(instance_list);
    return property_index;
}

bool Parser::peek_match()
{
    return peek(Token::KeyMatch);
}

ptr<Match> Parser::parse_match()
{
    start_span();
    auto match = CREATE(Match);

    eat(Token::KeyMatch);
    match->subject = parse_expression();

    eat(Token::CurlyL);
    while (peek_expression())
    {
        start_span();

        auto pattern = parse_expression();
        eat(Token::Colon);
        auto result = parse_expression();

        auto span = end_span();

        match->rules.emplace_back(Match::Rule{span, pattern, result});
    }
    eat(Token::CurlyR);

    match->span = end_span();
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
    start_span();
    auto expr = CREATE(Unary);

    if (peek(Token::Add))
        expr->op = eat(Token::Add).str;
    else if (peek(Token::Sub))
        expr->op = eat(Token::Sub).str;
    else if (peek(Token::KeyNot))
        expr->op = eat(Token::KeyNot).str;
    else
        throw CompilerError("Expected unary expression", current_token());

    expr->value = parse_expression(Precedence::Unary);

    expr->span = end_span();
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
    start_span();
    auto literal = CREATE(Literal);
    if (peek(Token::Number))
    {
        // FIXME: Treat num and int literals differently
        Token t = eat(Token::Number);
        if (t.str.find(".") != std::string::npos)
        {
            literal->value = stod(t.str);
            literal->pattern = Intrinsic::type_num;
        }
        else
        {
            literal->value = stoi(t.str);
            literal->pattern = Intrinsic::type_amt; // We can use `amt` and not `int` as number literals cannot be negative
        }
    }
    else if (peek(Token::String))
    {
        Token t = eat(Token::String);
        literal->value = t.str;
        literal->pattern = Intrinsic::type_str;
    }
    else if (peek(Token::Boolean))
    {
        Token t = eat(Token::Boolean);
        literal->value = t.str == "true";
        literal->pattern = Intrinsic::type_bool;
    }
    else
    {
        throw GambitError("Expected literal", current_token());
    }

    literal->span = end_span();
    return literal;
}

bool Parser::peek_list_value()
{
    return peek(Token::SquareL);
}

ptr<ListValue> Parser::parse_list_value()
{
    start_span();
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

    list->span = end_span();
    return list;
}

bool Parser::peek_infix_logical_or()
{
    return peek(Token::KeyOr);
}

ptr<Binary> Parser::parse_infix_logical_or(Expression lhs)
{
    start_span(get_span(lhs));
    auto expr = CREATE(Binary);

    expr->lhs = lhs;
    expr->op = eat(Token::KeyOr).str;
    expr->rhs = parse_expression(Precedence::LogicalOr);

    expr->span = end_span();
    return expr;
}

bool Parser::peek_infix_logical_and()
{
    return peek(Token::KeyAnd);
}

ptr<Binary> Parser::parse_infix_logical_and(Expression lhs)
{
    start_span(get_span(lhs));
    auto expr = CREATE(Binary);

    expr->lhs = lhs;
    expr->op = eat(Token::KeyAnd).str;
    expr->rhs = parse_expression(Precedence::LogicalAnd);

    expr->span = end_span();
    return expr;
}

bool Parser::peek_infix_term()
{
    return peek(Token::Add) ||
           peek(Token::Sub);
}

ptr<Binary> Parser::parse_infix_term(Expression lhs)
{
    start_span(get_span(lhs));
    auto expr = CREATE(Binary);
    expr->lhs = lhs;

    if (peek(Token::Add))
        expr->op = eat(Token::Add).str;
    else if (peek(Token::Sub))
        expr->op = eat(Token::Sub).str;
    else
        throw CompilerError("Expected infix term expression", current_token());

    expr->rhs = parse_expression(Precedence::Term);

    expr->span = end_span();
    return expr;
}

bool Parser::peek_infix_factor()
{
    return peek(Token::Mul) ||
           peek(Token::Div);
}

ptr<Binary> Parser::parse_infix_factor(Expression lhs)
{
    start_span(get_span(lhs));
    auto expr = CREATE(Binary);
    expr->lhs = lhs;

    if (peek(Token::Mul))
        expr->op = eat(Token::Mul).str;
    else if (peek(Token::Div))
        expr->op = eat(Token::Div).str;
    else
        throw CompilerError("Expected infix factor expression", current_token());

    expr->rhs = parse_expression(Precedence::Factor);

    expr->span = end_span();
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
        instance_list->span = get_span(lhs);
        lhs = instance_list;
    }

    start_span(get_span(lhs));
    auto property_index = CREATE(PropertyIndex);

    property_index->expr = lhs;
    eat(Token::Dot);
    property_index->property = parse_unresolved_identity();

    property_index->span = end_span();
    return property_index;
}

// PATTERNS

bool Parser::peek_pattern()
{
    return peek(Token::Identity);
}

Pattern Parser::parse_pattern(ptr<Scope> scope)
{
    Pattern pattern = parse_unresolved_identity();

    if (match(Token::Question))
    {
        start_span(get_span(pattern));
        auto optional_pattern = CREATE(OptionalPattern);
        optional_pattern->pattern = pattern;
        optional_pattern->span = end_span();
        pattern = optional_pattern;
    }

    return pattern;
}

#include "errors.h"
#include "source.h"
#include "intrinsic.h"
#include "parser.h"

ptr<Program> Parser::parse(Source &source)
{
    this->source = &source;
    current_token_index = 0;
    current_block_nesting = 0;
    panic_mode = false;

    parse_program();
    return program;
}

// TOKENS //

Token Parser::current_token()
{
    if (current_token_index >= source->tokens.size())
        return source->tokens.back();

    return source->tokens.at(current_token_index);
}

Token Parser::previous_token()
{
    if (current_token_index == 0)
        throw CompilerError("Attempt to get the token before the first token.");

    if (current_token_index >= source->tokens.size())
        return source->tokens.back();

    return source->tokens.at(current_token_index - 1);
}

// TOKEN PARSING //

// | METHOD              | OPTIONAL | CONSUMES | THROWS   |
// | ------              | -------- | -------- | ------   |
// | peek                | YES      | NO       | NEVER    |
// | confirm             | NO       | NO       | GAMBIT   |
// | comsume             | NO       | YES      | COMPILER |
// | peek_and_consume    | YES      | YES      | NEVER    |
// | confirm_and_consume | NO       | YES      | GAMBIT   |

bool Parser::peek(Token::Kind kind)
{
    if (current_token().kind == kind)
        return true;

    // Peek at first token that isn't a line token
    size_t i = current_token_index;
    while (source->tokens.at(i).kind == Token::Line && i < source->tokens.size())
        i++;
    return source->tokens.at(i).kind == kind;
}

bool Parser::confirm(Token::Kind kind)
{
    if (peek(kind))
        return true;

    Token token = current_token();
    gambit_error("Expected " + token_name.at(kind) + ", got " + token_name.at(token.kind), token);
    return false;
}

Token Parser::consume(Token::Kind kind)
{
    if (!peek(kind))
    {
        Token token = current_token();
        throw CompilerError("Attempt to eat " + token_name.at(kind) + ", got " + token_name.at(token.kind) + " " + to_string(token));
    }

    // Skip ahead to first token that isn't a line token (unless we are attempting to eat one)
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

Token Parser::consume()
{
    Token token = current_token();

    if (token.kind == Token::CurlyL)
        current_block_nesting++;
    else if (token.kind == Token::CurlyR && current_block_nesting > 0)
        current_block_nesting--;

    current_token_index++;
    return token;
}

bool Parser::peek_and_consume(Token::Kind kind)
{
    if (peek(kind))
    {
        consume(kind);
        return true;
    }

    return false;
}

bool Parser::confirm_and_consume(Token::Kind kind)
{
    if (confirm(kind))
    {
        consume(kind);
        return true;
    }

    return false;
}

bool Parser::peek_next(Token::Kind kind)
{
    // Skip lines before the current token
    size_t i = current_token_index;
    while (source->tokens.at(i).kind == Token::Line && i < source->tokens.size())
        i++;

    // Skip the current token
    i++;

    // Peek next
    if (source->tokens.at(i).kind == kind)
        return true;

    // Skip lines before the "next" token
    while (source->tokens.at(i).kind == Token::Line && i < source->tokens.size())
        i++;

    // Peek next
    return source->tokens.at(i).kind == kind;
}

// TOKEN PARSING UTILITY //

bool Parser::end_of_file()
{
    return peek(Token::EndOfFile);
}

void Parser::skip_whitespace()
{
    while (peek_and_consume(Token::Line))
        ;
}

void Parser::skip_line()
{
    while (!end_of_file() && !peek_and_consume(Token::Line))
        consume();
}

void Parser::skip_to_block_nesting(size_t target_nesting)
{
    while (!end_of_file() && current_block_nesting != target_nesting)
        consume();
}

void Parser::skip_to_end_of_current_block()
{
    skip_to_block_nesting(current_block_nesting - 1);
}

// SPANS //

Span Parser::to_span(Token token)
{
    return Span(
        token.line,
        token.column,
        token.position,
        token.str.length(),
        token.kind == Token::Line,
        source);
}

void Parser::start_span()
{
    Token token = current_token();
    span_stack.push_back({token.line,
                          token.column,
                          token.position,
                          0,     // A correct length will be generated when the span is finished
                          false, // If a span is multiline will be determined when the span is finished
                          source});
}

Span Parser::finish_span()
{
    Span span = span_stack.back();
    span_stack.pop_back();

    Token token = current_token();
    span.length = token.position + token.str.length() - span.position;
    span.multiline = span.line != token.line;

    return span;
}

void Parser::discard_span()
{
    span_stack.pop_back();
}

// SCOPES //

void Parser::declare(ptr<Scope> scope, Scope::LookupValue value)
{
    auto identity = identity_of(value);

    if (directly_declared_in_scope(scope, identity))
    {
        auto existing = fetch(scope, identity);

        if (IS_PTR(existing, Scope::OverloadedIdentity) && is_overloadable(value))
        {
            auto overloaded_identity = AS_PTR(existing, Scope::OverloadedIdentity);
            overloaded_identity->overloads.emplace_back(value);
        }
        else
        {
            gambit_error("Cannot declare " + identity + " in scope, as " + identity + " already exists.", {get_span(value), get_span(existing)});

            // As this is not a syntax error, we do not need to enter panic mode

            // FIXME: Currently, this function is included in the parser and not the APM utility,
            //        under the assumption that the APM util shouldn't have side-effects, because
            //        each part of the compiler will want to handle those side effects differently.
            //        Once the compiler has matured more, reassess if that is still true?

            panic_mode = false;
        }
    }
    else if (is_overloadable(value))
    {
        auto overloaded_identity = CREATE(Scope::OverloadedIdentity);
        overloaded_identity->identity = identity;
        overloaded_identity->overloads.emplace_back(value);
        scope->lookup.insert({identity, overloaded_identity});
    }
    else
    {
        scope->lookup.insert({identity, value});
    }
}

// ERRORS //

void Parser::gambit_error(string msg, size_t line, size_t column, initializer_list<Span> spans)
{
    if (panic_mode)
        return;
    source->log_error(msg, line, column, spans);
    panic_mode = true;
}

void Parser::gambit_error(string msg, Token token)
{
    if (panic_mode)
        return;
    source->log_error(msg, token);
    panic_mode = true;
}

void Parser::gambit_error(string msg, Span span)
{
    if (panic_mode)
        return;
    source->log_error(msg, span);
    panic_mode = true;
}

void Parser::gambit_error(string msg, initializer_list<Span> spans)
{
    if (panic_mode)
        return;
    source->log_error(msg, spans);
    panic_mode = true;
}

// FIXME: APM nodes that are generated during panic mode should be annotated as such.
//        This way, later stages of the compiler can identity parts of the APM that
//        are malformed, and that it should therefore ignore. (this same flag can
//        probably be shared by the different stages).
//
//        Though, think this through before you implement it, as I haven't actually
//        thought it through all the way. There may be a better solution?

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
    // NOTE: We intentionally do not declare the none type, as users cannot access this directly.
    //       Instead they should use the `none` keyword.
    // FIXME: Implement the `none` keyword.

    declare(program->global_scope, Intrinsic::entity_player);
    declare(program->global_scope, Intrinsic::state_player_number);

    declare(program->global_scope, Intrinsic::entity_game);
    declare(program->global_scope, Intrinsic::variable_game);
    declare(program->global_scope, Intrinsic::state_game_players);

    while (!peek_and_consume(Token::EndOfFile))
    {
        if (peek_entity_definition())
            parse_entity_definition(program->global_scope);
        else if (peek_enum_definition())
            parse_enum_definition(program->global_scope);
        else if (peek_state_property_definition())
            parse_state_property_definition(program->global_scope);
        else if (peek_function_property_definition())
            parse_function_property_definition(program->global_scope);
        else if (peek_procedure_definition())
            parse_procedure_definition(program->global_scope);
        else
        {
            skip_whitespace();
            gambit_error("Unexpected '" + current_token().str + "' in global scope.", current_token());
        }

        if (panic_mode)
        {
            skip_line();
            skip_to_block_nesting(0);
            panic_mode = false;
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
    auto code_block = CREATE(CodeBlock);
    code_block->scope = CREATE(Scope);
    code_block->scope->parent = scope;

    start_span();

    // SINGLETON CODE BLOCKS
    if (peek_and_consume(Token::Colon))
    {
        code_block->singleton_block = true;

        auto maybe_statement = parse_statement(code_block->scope, false);
        if (maybe_statement.has_value())
        {
            auto statement = maybe_statement.value();
            code_block->statements.emplace_back(statement);

            // Code block statements are not allowed inside of singleton blocks
            if (IS_PTR(statement, CodeBlock))
            {
                auto code_block_statement = AS_PTR(statement, CodeBlock);
                if (code_block_statement->singleton_block)
                    gambit_error("Too many colons.", code_block->span);
                else
                    gambit_error("Syntax `: { ... }` is invalid. Either use `: ... ` for a single statement, or `{ ... }` for multiple statements.", code_block->span);
            }
        }
    }

    // REGULAR CODE BLOCKS
    else if (confirm_and_consume(Token::CurlyL))
    {
        while (peek_statement())
        {
            auto statement = parse_statement(code_block->scope);
            if (statement.has_value())
                code_block->statements.emplace_back(statement.value());
        }
        confirm_and_consume(Token::CurlyR);
    }

    code_block->span = finish_span();
    return code_block;
}

bool Parser::peek_enum_definition()
{
    return peek(Token::KeyEnum);
}

void Parser::parse_enum_definition(ptr<Scope> scope)
{
    auto enum_type = CREATE(EnumType);
    auto union_pattern = CREATE(UnionPattern);

    start_span();
    confirm_and_consume(Token::KeyEnum);
    if (!confirm(Token::Identity))
    {
        discard_span();
        return;
    }
    enum_type->identity = consume(Token::Identity).str;

    // Enum values
    if (confirm_and_consume(Token::CurlyL))
    {
        do
        {
            auto literal = parse_literal(true);

            if (IS_PTR(literal, IdentityLiteral))
            {
                auto identity_literal = AS_PTR(literal, IdentityLiteral);

                auto enum_value = CREATE(EnumValue);
                enum_value->identity = identity_literal->identity;
                enum_value->span = identity_literal->span;
                enum_value->type = enum_type;

                enum_type->values.emplace_back(enum_value);
            }
            else
            {
                union_pattern->patterns.push_back(literal);
            }
        } while (peek_and_consume(Token::Comma));

        confirm_and_consume(Token::CurlyR);
    }

    confirm_and_consume(Token::Line);

    // Declare enum
    enum_type->span = finish_span();
    if (union_pattern->patterns.size() == 0)
    {
        declare(scope, enum_type);
    }
    else
    {
        union_pattern->patterns.push_back(enum_type);
        union_pattern->identity = enum_type->identity;
        // union_pattern->span = enum_type->span;
        declare(scope, union_pattern);
    }
}

bool Parser::peek_entity_definition()
{
    return peek(Token::KeyEntity);
}

void Parser::parse_entity_definition(ptr<Scope> scope)
{
    auto entity = CREATE(EntityType);

    start_span();
    confirm_and_consume(Token::KeyEntity);

    if (!confirm(Token::Identity))
    {
        discard_span();
        return;
    }
    entity->identity = consume(Token::Identity).str;

    confirm_and_consume(Token::Line);

    entity->span = finish_span();
    declare(scope, entity);
}

bool Parser::peek_state_property_definition()
{
    return peek(Token::KeyState);
}

void Parser::parse_state_property_definition(ptr<Scope> scope)
{
    auto state = CREATE(StateProperty);
    state->scope = CREATE(Scope);
    state->scope->parent = scope;

    start_span();
    confirm_and_consume(Token::KeyState);

    state->pattern = parse_literal(false);

    if (confirm_and_consume(Token::ParenL))
    {
        do
        {
            start_span();
            auto parameter = CREATE(Variable);
            parameter->pattern = parse_literal(false);

            if (!confirm(Token::Identity))
            {
                discard_span();
                continue;
            }
            parameter->identity = consume(Token::Identity).str;

            parameter->span = finish_span();
            state->parameters.emplace_back(parameter);
            declare(state->scope, parameter);
        } while (peek_and_consume(Token::Comma));

        confirm_and_consume(Token::ParenR);
    }

    confirm_and_consume(Token::Dot);

    if (!confirm(Token::Identity))
    {
        discard_span();
        return;
    }
    state->identity = consume(Token::Identity).str;

    state->span = finish_span();
    declare(scope, state);

    if (peek_and_consume(Token::Colon))
        state->initial_value = parse_expression();
}

bool Parser::peek_function_property_definition()
{
    return peek(Token::KeyFn);
}

void Parser::parse_function_property_definition(ptr<Scope> scope)
{
    auto funct = CREATE(FunctionProperty);
    funct->scope = CREATE(Scope);
    funct->scope->parent = scope;

    start_span();
    confirm_and_consume(Token::KeyFn);

    funct->pattern = parse_literal(false);

    if (confirm_and_consume(Token::ParenL))
    {
        do
        {
            start_span();
            auto parameter = CREATE(Variable);
            parameter->pattern = parse_literal(false);

            if (!confirm(Token::Identity))
            {
                discard_span();
                continue;
            }
            parameter->identity = consume(Token::Identity).str;

            parameter->span = finish_span();
            funct->parameters.emplace_back(parameter);
            declare(funct->scope, parameter);
        } while (peek_and_consume(Token::Comma));

        confirm_and_consume(Token::ParenR);
    }

    confirm_and_consume(Token::Dot);

    if (!confirm(Token::Identity))
    {
        discard_span();
        return;
    }
    funct->identity = consume(Token::Identity).str;

    funct->span = finish_span();
    declare(scope, funct);

    if (peek_code_block(true))
        funct->body = parse_code_block(funct->scope);
}

bool Parser::peek_procedure_definition()
{
    return peek(Token::Identity);
}

void Parser::parse_procedure_definition(ptr<Scope> scope)
{
    if (!confirm(Token::Identity))
        return;

    auto proc = CREATE(Procedure);
    proc->scope = CREATE(Scope);
    proc->scope->parent = scope;

    start_span();
    proc->identity = consume(Token::Identity).str;

    if (confirm_and_consume(Token::ParenL))
    {
        if (peek_literal(false))
        {
            do
            {
                start_span();
                auto parameter = CREATE(Variable);
                parameter->pattern = parse_literal(false);

                if (!confirm(Token::Identity))
                {
                    discard_span();
                    continue;
                }
                parameter->identity = consume(Token::Identity).str;

                parameter->span = finish_span();
                proc->parameters.emplace_back(parameter);
                declare(proc->scope, parameter);
            } while (peek_and_consume(Token::Comma));
        }

        confirm_and_consume(Token::ParenR);
    }

    proc->span = finish_span();
    declare(scope, proc);

    proc->body = parse_code_block(proc->scope);
}

// STATEMENTS //

bool Parser::peek_statement()
{
    return peek_expression() ||
           peek(Token::CurlyL) ||
           peek(Token::KeyIf) ||
           peek(Token::KeyFor) ||
           peek(Token::KeyLoop) ||
           peek(Token::KeyReturn);
}

optional<Statement> Parser::parse_statement(ptr<Scope> scope, bool require_newline)
{
    Statement stmt;
    start_span();

    // CODE BLOCK
    if (peek_code_block(false))
    {
        discard_span();
        stmt = parse_code_block(scope);
    }

    // IF STATEMENT (Ignore if expressions)
    else if (peek(Token::KeyIf) && !peek_next(Token::CurlyL))
    {
        auto if_statement = CREATE(IfStatement);

        start_span();
        consume(Token::KeyIf);

        IfStatement::Rule rule;
        rule.condition = parse_expression();
        rule.code_block = parse_code_block(scope);
        rule.span = finish_span();
        if_statement->rules.push_back(rule);

        while (peek(Token::KeyElse))
        {
            start_span();
            consume(Token::KeyElse);

            if (peek_and_consume(Token::KeyIf))
            {
                IfStatement::Rule rule;
                rule.condition = parse_expression();
                rule.code_block = parse_code_block(scope);
                rule.span = finish_span();
                if_statement->rules.push_back(rule);
            }
            else
            {
                if_statement->else_block = parse_code_block(scope);
                if_statement->span = finish_span();
                break;
            }
        }

        if_statement->span = finish_span();
        stmt = if_statement;
    }

    // FOR LOOP
    else if (peek_and_consume(Token::KeyFor))
    {
        auto for_statement = CREATE(ForStatement);
        for_statement->scope = CREATE(Scope);
        for_statement->scope->parent = scope;

        start_span();
        auto variable = CREATE(Variable);
        variable->identity = consume(Token::Identity).str;
        variable->is_constant = true;
        variable->pattern = CREATE(UninferredPattern);
        variable->span = finish_span();

        for_statement->variable = variable;
        declare(for_statement->scope, variable);

        confirm_and_consume(Token::KeyIn);

        for_statement->range = parse_expression();

        for_statement->body = parse_code_block(for_statement->scope);

        for_statement->span = finish_span();
        stmt = for_statement;
    }

    // LOOP STATEMENT
    else if (peek_and_consume(Token::KeyLoop))
    {
        auto loop_statement = CREATE(LoopStatement);
        loop_statement->scope = CREATE(Scope);
        loop_statement->scope->parent = scope;
        loop_statement->body = parse_code_block(loop_statement->scope);
        loop_statement->span = finish_span();
        stmt = loop_statement;
    }

    // RETURN STATEMENT
    else if (peek_and_consume(Token::KeyReturn))
    {
        auto return_statement = CREATE(ReturnStatement);
        return_statement->value = parse_expression();
        return_statement->span = finish_span();
        stmt = return_statement;
    }

    // EXPRESSION STATEMENT / DECLARATION / ASSIGNMENT / INSERT
    else if (peek_expression())
    {
        auto expr = parse_expression();
        if (peek(Token::Line))
        {
            discard_span();
            stmt = expr;
        }
        else
        {
            stmt = parse_infix_expression_statement(expr, scope);
        }
    }

    // INVALID SYNTAX
    else
    {
        skip_line();
        gambit_error("Expected statement", finish_span());

        return {};
    }

    if (require_newline)
    {
        // NOTE: Order of these functions is important, otherwise the user will get a
        //       "expected new line" error, even if the end of file follows the statement
        if (!end_of_file() && !confirm_and_consume(Token::Line))
            skip_line();
    }

    return stmt;
}

Statement Parser::parse_infix_expression_statement(Expression lhs, ptr<Scope> scope)
{
    // NOTE: Assumes the caller started a span before starting to parse the lhs

    // DECLARATION (starting with pattern)
    if (peek(Token::Identity)) // FIXME: Should we try parsing an entire expression, and then check to see if it is an identity?
    {
        // FIXME: This code is a bit sloppy! Fix the partial implementation.
        if (!IS(lhs, UnresolvedLiteral))
            throw CompilerError("Error due to partial implementation. Attempt to turn expresion into pattern of a variable declaration failed.", get_span(lhs));

        // Variable
        Token identity_token = consume(Token::Identity);

        auto variable = CREATE(Variable);
        variable->identity = identity_token.str;
        variable->pattern = AS(lhs, UnresolvedLiteral);
        variable->is_constant = false;
        variable->span = to_span(identity_token);
        declare(scope, variable);

        // Declaration statement
        auto variable_declaration = CREATE(VariableDeclaration);
        variable_declaration->variable = variable;

        if (peek_and_consume(Token::Assign))
        {
            variable_declaration->value = parse_expression();
        }
        else if (peek_and_consume(Token::AssignConstant))
        {
            variable_declaration->value = parse_expression();
            variable->is_constant = true;
        }

        variable_declaration->span = finish_span();
        return variable_declaration;
    }

    // ASSIGNMENT
    if (peek_and_consume(Token::Assign))
    {
        auto assignment_stmt = CREATE(AssignmentStatement);
        assignment_stmt->subject = lhs;
        assignment_stmt->value = parse_expression();

        assignment_stmt->span = finish_span();
        return assignment_stmt;
    }

    // CONSTANT DECLARATION (not beginning with pattern)
    if (peek_and_consume(Token::AssignConstant))
    {
        // FIXME: This code is a bit sloppy! Fix the partial implementation.
        if (!IS(lhs, UnresolvedLiteral))
            throw CompilerError("Error due to partial implementation. Attempt to turn expresion into identity of constant declaration failed. #1", get_span(lhs));

        auto lhs_literal = AS(lhs, UnresolvedLiteral);

        // FIXME: This code is a bit sloppy! Fix the partial implementation.
        if (!IS_PTR(lhs_literal, IdentityLiteral))
            throw CompilerError("Error due to partial implementation. Attempt to turn expresion into identity of constant declaration failed. #2", get_span(lhs));

        // Variable
        auto identity_literal = AS_PTR(lhs_literal, IdentityLiteral);

        auto variable = CREATE(Variable);
        variable->identity = identity_literal->identity;
        variable->pattern = CREATE(UninferredPattern);
        variable->is_constant = true;
        variable->span = identity_literal->span;
        declare(scope, variable);

        // Declaration
        auto variable_declaration = CREATE(VariableDeclaration);
        variable_declaration->variable = variable;
        variable_declaration->value = parse_expression();

        variable_declaration->span = finish_span();
        return variable_declaration;
    }

    // INSERT
    if (peek_and_consume(Token::KeyInsert))
    {
        auto insert_oper = CREATE(Binary);
        insert_oper->op = "insert";
        insert_oper->lhs = lhs;
        insert_oper->rhs = parse_expression();

        insert_oper->span = finish_span();
        return insert_oper;
    }

    discard_span();
    return lhs;
}

// EXPRESSIONS //

// FIXME: Move into utility code unit
bool Parser::operator_should_bind(Precedence operator_precedence, Precedence caller_precedence, bool left_associative)
{
    if (left_associative)
        return operator_precedence > caller_precedence;
    else
        return operator_precedence >= caller_precedence;
}

bool Parser::peek_expression()
{
    return peek_paren_expr() ||
           peek_literal(true) ||
           peek_unary() ||
           peek_if_expression() ||
           peek_match();
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
    //        represent values, and so defacto have the highest precedence. Only ones I
    //        have no idea about are `match` and `if_expression`.
    if (peek_paren_expr())
        lhs = parse_paren_expr();

    else if (peek_literal(true))
        lhs = parse_literal(true);

    else if (peek_unary() && operator_should_bind(Precedence::Unary, caller_precedence))
        lhs = parse_unary();

    else if (peek_if_expression())
        lhs = parse_if_expression();

    else if (peek_match())
        lhs = parse_match();

    else
    {
        // FIXME: Currently, only the current token is turned into an invalid expression,
        //        should some larger portion of the code become invalid? (e.g. until the)
        //        end of the current line? maybe the current brackets?)
        Token token = consume();
        gambit_error("Expected expression", token);

        auto expr = CREATE(InvalidExpression);
        // expr->span = to_span(token);
        return expr;
    }

    while (true)
    {
        if (peek_infix_call() && operator_should_bind(Precedence::Call, caller_precedence))
            lhs = parse_infix_call(lhs);
        else if (peek_infix_factor() && operator_should_bind(Precedence::Factor, caller_precedence))
            lhs = parse_infix_factor(lhs);
        else if (peek_infix_term() && operator_should_bind(Precedence::Term, caller_precedence))
            lhs = parse_infix_term(lhs);
        else if (!peek(Token::Line) && peek_infix_expression_index() && operator_should_bind(Precedence::Index, caller_precedence))
            lhs = parse_infix_expression_index(lhs);
        else if (peek_infix_property_index() && operator_should_bind(Precedence::Index, caller_precedence))
            lhs = parse_infix_property_index(lhs);
        else if (peek_infix_compare_relative() && operator_should_bind(Precedence::CompareRelative, caller_precedence))
            lhs = parse_infix_compare_relative(lhs);
        else if (peek_infix_compare_equal() && operator_should_bind(Precedence::CompareEqual, caller_precedence))
            lhs = parse_infix_compare_equal(lhs);
        else if (peek_infix_logical_and() && operator_should_bind(Precedence::LogicalAnd, caller_precedence))
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
    start_span();
    confirm_and_consume(Token::ParenL);

    auto expr = parse_expression();

    // Bracketed expression
    if (!peek(Token::Comma))
    {
        confirm_and_consume(Token::ParenR);
        // FIXME: Update the expression's span to include the parentheses
        // expr->span = finish_span();
        discard_span();
        return expr;
    }

    // Instance list
    auto instance_list = CREATE(InstanceList);
    instance_list->values.emplace_back(expr);

    while (peek_and_consume(Token::Comma))
        instance_list->values.emplace_back(parse_expression());

    confirm_and_consume(Token::ParenR);
    instance_list->span = finish_span();

    auto property_index = parse_infix_property_index(instance_list);
    return property_index;
}

bool Parser::peek_if_expression()
{
    return peek(Token::KeyIf);
}

ptr<IfExpression> Parser::parse_if_expression()
{
    auto if_expression = CREATE(IfExpression);

    start_span();
    confirm_and_consume(Token::KeyIf);

    if (confirm_and_consume(Token::CurlyL))
    {
        while ((peek_expression() || peek(Token::KeyElse)) && !if_expression->has_else)
        {
            IfExpression::Rule rule;

            start_span();
            if (peek_and_consume(Token::KeyElse))
            {
                if_expression->has_else = true;
                // FIXME: Should we really be creating a whole new intrinsic value like this?
                auto true_value = CREATE(PrimitiveValue);
                true_value->value = true;
                true_value->type = Intrinsic::type_bool;
                rule.condition = true_value;
            }
            else
            {
                rule.condition = parse_expression();
            }

            confirm_and_consume(Token::Colon);
            rule.result = parse_expression();
            rule.span = finish_span();
            confirm_and_consume(Token::Line);

            if_expression->rules.emplace_back(rule);
        }

        confirm_and_consume(Token::CurlyR);
    }

    if_expression->span = finish_span();
    return if_expression;
}

bool Parser::peek_match()
{
    return peek(Token::KeyMatch);
}

ptr<MatchExpression> Parser::parse_match()
{
    auto match = CREATE(MatchExpression);

    start_span();
    confirm_and_consume(Token::KeyMatch);

    match->subject = parse_expression();

    if (confirm_and_consume(Token::CurlyL))
    {
        while (peek_literal(true) || peek(Token::KeyElse))
        {
            MatchExpression::Rule rule;

            start_span();
            if (peek_and_consume(Token::KeyElse))
            {
                match->has_else = true;
                rule.pattern = CREATE(AnyPattern);
            }
            else
            {
                rule.pattern = parse_literal(true);
            }

            confirm_and_consume(Token::Colon);
            rule.result = parse_expression();
            rule.span = finish_span();
            confirm_and_consume(Token::Line);

            match->rules.emplace_back(rule);

            if (match->has_else)
                break;
        }

        confirm_and_consume(Token::CurlyR);
    }

    match->span = finish_span();
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
    Token op_token;

    start_span();

    if (peek(Token::Add))
        op_token = consume(Token::Add);
    else if (peek(Token::Sub))
        op_token = consume(Token::Sub);
    else if (peek(Token::KeyNot))
        op_token = consume(Token::KeyNot);
    else
        throw CompilerError("Expected unary expression, got " + to_string(current_token()) + " token");

    expr->op = op_token.str;
    expr->value = parse_expression(Precedence::Unary);
    expr->span = finish_span();

    return expr;
}

bool Parser::peek_infix_logical_or()
{
    return peek(Token::KeyOr);
}

ptr<Binary> Parser::parse_infix_logical_or(Expression lhs)
{
    auto expr = CREATE(Binary);
    confirm_and_consume(Token::KeyOr);

    expr->lhs = lhs;
    expr->op = "or";
    expr->rhs = parse_expression(Precedence::LogicalOr);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
    return expr;
}

bool Parser::peek_infix_logical_and()
{
    return peek(Token::KeyAnd);
}

ptr<Binary> Parser::parse_infix_logical_and(Expression lhs)
{
    auto expr = CREATE(Binary);
    confirm_and_consume(Token::KeyAnd);

    expr->lhs = lhs;
    expr->op = "and";
    expr->rhs = parse_expression(Precedence::LogicalAnd);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
    return expr;
}

bool Parser::peek_infix_compare_equal()
{
    return peek(Token::Equal) ||
           peek(Token::NotEqual);
}

ptr<Binary> Parser::parse_infix_compare_equal(Expression lhs)
{
    auto expr = CREATE(Binary);

    expr->lhs = lhs;
    if (peek_and_consume(Token::Equal))
        expr->op = "==";
    else if (peek_and_consume(Token::NotEqual))
        expr->op = "!=";
    else
        throw CompilerError("Expected infix compare equal expression, got " + to_string(current_token()) + " token");
    expr->rhs = parse_expression(Precedence::CompareEqual);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
    return expr;
}

bool Parser::peek_infix_compare_relative()
{
    return peek(Token::TrigL) ||
           peek(Token::LessThanEqual) ||
           peek(Token::TrigR) ||
           peek(Token::GreaterThanEqual);
}

ptr<Binary> Parser::parse_infix_compare_relative(Expression lhs)
{
    auto expr = CREATE(Binary);

    expr->lhs = lhs;
    if (peek_and_consume(Token::TrigL))
        expr->op = "<";
    else if (peek_and_consume(Token::LessThanEqual))
        expr->op = "<=";
    else if (peek_and_consume(Token::TrigR))
        expr->op = ">";
    else if (peek_and_consume(Token::GreaterThanEqual))
        expr->op = ">=";
    else
        throw CompilerError("Expected infix compare relative expression, got " + to_string(current_token()) + " token");
    expr->rhs = parse_expression(Precedence::CompareRelative);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
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
    if (peek_and_consume(Token::Add))
        expr->op = "+";
    else if (peek_and_consume(Token::Sub))
        expr->op = "-";
    else
        throw CompilerError("Expected infix term expression, got " + to_string(current_token()) + " token");
    expr->rhs = parse_expression(Precedence::Term);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
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
    if (peek_and_consume(Token::Mul))
        expr->op = "*";
    else if (peek_and_consume(Token::Div))
        expr->op = "/";
    else
        throw CompilerError("Expected infix factor expression, got " + to_string(current_token()) + " token");
    expr->rhs = parse_expression(Precedence::Factor);

    expr->span = merge(get_span(expr->lhs), get_span(expr->rhs));
    return expr;
}

bool Parser::peek_infix_expression_index()
{
    return peek(Token::SquareL);
}

ptr<ExpressionIndex> Parser::parse_infix_expression_index(Expression lhs)
{
    auto expression_index = CREATE(ExpressionIndex);
    expression_index->subject = lhs;

    if (confirm_and_consume(Token::SquareL))
    {
        expression_index->index = parse_expression();
        confirm_and_consume(Token::SquareR);
    }
    else
    {
        // FIXME: This error exists solely to avoid having a `expression_index` with ain in valid `index`
        //        What should the proper solution to this be?
        throw CompilerError("Attempt to `parse_infix_expression_index`, however a `[` token could not be consumed.");
    }

    expression_index->span = merge(get_span(expression_index->subject), get_span(expression_index->index));
    return expression_index;
}

bool Parser::peek_infix_property_index()
{
    return peek(Token::Dot);
}

ptr<PropertyIndex> Parser::parse_infix_property_index(Expression lhs)
{
    // TODO: As of writing, InstanceLists are only used to collect values that will become the
    //       lhs of a PropertyIndex. If that remains the case, maybe it would be worth (at this
    //       point in the code) transferring all of the elements of instance_list->values into a
    //       'lhs' ('subjects'?) vector<Expression> on the PropertyList? This way there isn't a
    //       redundant InstanceList node just hanging around in the program model on each index?

    // Convert lhs to an instance list if it isn't one already
    // This occurs when the syntax foo.bar or (foo).bar is used.
    if (!IS_PTR(lhs, InstanceList))
    {
        auto instance_list = CREATE(InstanceList);
        instance_list->values.emplace_back(lhs);
        instance_list->span = get_span(lhs);
        lhs = instance_list;
    }

    // Property index
    auto property_index = CREATE(PropertyIndex);

    property_index->expr = lhs;
    confirm_and_consume(Token::Dot);

    // FIXME: Confirm the identity is present and, if not, then gracefully and provide a user error
    Token token = consume(Token::Identity);
    auto unresolved_identity = CREATE(IdentityLiteral);
    unresolved_identity->identity = token.str;
    unresolved_identity->span = to_span(token);
    property_index->property = unresolved_identity;

    property_index->span = merge(get_span(property_index->expr), unresolved_identity->span);
    return property_index;
}

bool Parser::peek_infix_call()
{
    return peek(Token::ParenL);
}

ptr<Call> Parser::parse_infix_call(Expression lhs)
{
    auto call = CREATE(Call);
    call->callee = lhs;

    start_span();
    confirm_and_consume(Token::ParenL);

    if (peek_expression())
    {
        do
        {
            Call::Argument argument;

            start_span();

            if (peek(Token::Identity) && peek_next(Token::Colon))
            {
                argument.named = true;
                argument.name = consume(Token::Identity).str;
                consume(Token::Colon);
                argument.value = parse_expression();
            }
            else
            {
                argument.named = false;
                argument.value = parse_expression();
            }

            argument.span = finish_span();
            call->arguments.emplace_back(argument);
        } while (peek_and_consume(Token::Comma));
    }

    confirm_and_consume(Token::ParenR);

    auto span = finish_span();
    call->span = merge(get_span(lhs), span);
    return call;
}

// PATTERNS

bool Parser::peek_literal(bool allow_primitive_values)
{
    return peek(Token::Identity) ||
           peek(Token::KeyAny) ||
           peek(Token::KeyNone) ||
           (allow_primitive_values && peek(Token::Number)) ||
           (allow_primitive_values && peek(Token::String)) ||
           (allow_primitive_values && peek(Token::Boolean)) ||
           peek(Token::SquareL);
}

UnresolvedLiteral Parser::parse_literal(bool allow_primitive_values)
{
    UnresolvedLiteral unresolved_literal;
    start_span();

    // Identity Literals
    if (peek(Token::Identity))
    {
        auto identity = CREATE(IdentityLiteral);
        identity->identity = consume(Token::Identity).str;

        identity->span = finish_span();
        unresolved_literal = identity;
    }

    // "Any"
    else if (peek_and_consume(Token::KeyAny))
    {
        auto identity_literal = CREATE(IdentityLiteral);
        identity_literal->identity = "any";

        identity_literal->span = finish_span();
        unresolved_literal = identity_literal;
    }

    // "none"
    else if (peek_and_consume(Token::KeyNone))
    {
        auto primitive_literal = CREATE(PrimitiveLiteral);
        primitive_literal->value = Intrinsic::none_val;
        primitive_literal->span = finish_span();
        unresolved_literal = primitive_literal;
    }

    // Primitive value
    else if (allow_primitive_values && (peek(Token::Number) || peek(Token::String) || peek(Token::Boolean)))
    {
        auto primitive_literal = CREATE(PrimitiveLiteral);
        auto primitive_value = CREATE(PrimitiveValue);
        primitive_literal->value = primitive_value;

        skip_whitespace();
        auto token = consume();

        if (token.kind == Token::Number)
        {
            if (token.str.find(".") != std::string::npos)
            {
                primitive_value->value = stod(token.str);
                primitive_value->type = Intrinsic::type_num;
            }
            else
            {
                primitive_value->value = stoi(token.str);
                primitive_value->type = Intrinsic::type_amt; // We use `amt` instead of `int` as number literals cannot be negative
            }
        }

        else if (token.kind == Token::String)
        {
            primitive_value->value = token.str;
            primitive_value->type = Intrinsic::type_str;
        }

        else if (token.kind == Token::Boolean)
        {
            primitive_value->value = token.str == "true";
            primitive_value->type = Intrinsic::type_bool;
        }

        else
        {
            throw CompilerError("Could not match token type while parsing PrimitiveLiteral");
        }

        primitive_literal->span = finish_span();
        unresolved_literal = primitive_literal;
    }

    // Lists
    else if (peek_and_consume(Token::SquareL))
    {
        auto list_literal = CREATE(ListLiteral);

        if (peek_expression())
        {
            do
            {
                list_literal->values.emplace_back(parse_expression());
            } while (peek_and_consume(Token::Comma));
        }

        confirm_and_consume(Token::SquareR);

        list_literal->span = finish_span();
        unresolved_literal = list_literal;
    }

    // Optional pattern literal
    if (peek(Token::Question))
    {
        auto option_literal = CREATE(OptionLiteral);
        option_literal->literal = unresolved_literal;

        Token question = consume(Token::Question);
        option_literal->span = merge(get_span(unresolved_literal), to_span(question));
        unresolved_literal = option_literal;
    }

    return unresolved_literal;
}

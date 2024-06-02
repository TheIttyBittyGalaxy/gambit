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
// | comfirm_and_consume | NO       | YES      | GAMBIT   |

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

void Parser::skip_to_end_of_line()
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
    auto &span = span_stack.emplace_back(
        token.line,
        token.column,
        token.position,
        0,     // A correct length will be generated when the span is finished
        false, // If a span is multiline will be determined when the span is finished
        source);
}

Span Parser::finish_span()
{
    Span span = *span_stack.end();
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
            skip_to_end_of_line();
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

    // Singleton code blocks
    if (peek_and_consume(Token::Colon))
    {
        // If there is no statement and the line is blank, skip to the next line.
        // This means that the "Expected statement" error will appear on the next line
        if (!peek_statement() && peek(Token::Line))
            skip_to_end_of_line();

        auto statement = parse_statement(code_block->scope);
        code_block->statements.emplace_back(statement);

        code_block->singleton_block = true;

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

    // Regular code blocks
    else if (confirm_and_consume(Token::CurlyL))
    {
        while (peek_statement())
        {
            auto statement = parse_statement(code_block->scope);
            code_block->statements.emplace_back(statement);
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
            if (peek_intrinsic_value())
            {
                auto expr = parse_intrinsic_value();
                if (IS_PTR(expr, IntrinsicValue))
                {
                    auto intrinsic_value = AS_PTR(expr, IntrinsicValue);
                    union_pattern->patterns.push_back(intrinsic_value);
                }
                else
                {
                    // FIXME: In this case expr is an invalid value - what should we do?
                }
            }
            else if (confirm(Token::Identity))
            {
                auto identity_token = consume(Token::Identity);

                auto enum_value = CREATE(EnumValue);
                enum_value->identity = identity_token.str;
                enum_value->type = enum_type;
                enum_value->span = to_span(identity_token);

                enum_type->values.emplace_back(enum_value);
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
        union_pattern->span = enum_type->span;
        declare(scope, union_pattern);
    }
}

bool Parser::peek_entity_definition()
{
    return peek(Token::KeyEntity);
}

void Parser::parse_entity_definition(ptr<Scope> scope)
{
    auto entity = CREATE(Entity);

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

    state->pattern = parse_pattern(false);

    if (confirm_and_consume(Token::ParenL))
    {
        do
        {
            start_span();
            auto parameter = CREATE(Variable);
            parameter->pattern = parse_pattern(false);

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

    funct->pattern = parse_pattern(false);

    if (confirm_and_consume(Token::ParenL))
    {
        do
        {
            start_span();
            auto parameter = CREATE(Variable);
            parameter->pattern = parse_pattern(false);

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

    if (peek_code_block())
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
        if (peek_pattern(false))
        {
            do
            {
                start_span();
                auto parameter = CREATE(Variable);
                parameter->pattern = parse_pattern(false);

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
    return peek_code_block() ||
           peek_expression();
}

Statement Parser::parse_statement(ptr<Scope> scope)
{
    Statement stmt;
    if (peek_code_block(false))
        stmt = parse_code_block(scope);
    else if (peek_if_statement())
        stmt = parse_if_statement(scope);
    else if (peek_variable_declaration())
        stmt = parse_variable_declaration(scope);
    else if (peek_expression())
        stmt = parse_expression();
    else
    {
        // FIXME: Instead of marking just the current token as an invalid statement,
        //        we should mark everything up to the end of the line as invalid.
        Token token = consume();
        gambit_error("Expected statement", token);

        auto stmt = CREATE(InvalidStatement);
        stmt->span = to_span(token);
        return stmt;
    }

    if (!end_of_file() && !confirm_and_consume(Token::Line))
    {
        skip_to_end_of_line();
    }

    return stmt;
}

bool Parser::peek_if_statement()
{
    return peek(Token::KeyIf);
}

[[nodiscard]] ptr<IfStatement> Parser::parse_if_statement(ptr<Scope> scope)
{
    auto if_statement = CREATE(IfStatement);

    start_span();
    start_span();
    confirm_and_consume(Token::KeyIf);

    IfStatement::Segment segment;
    segment.condition = parse_expression();
    segment.code_block = parse_code_block(scope);
    segment.span = finish_span();
    if_statement->segments.push_back(segment);

    while (peek(Token::KeyElse))
    {
        start_span();
        consume(Token::KeyElse);

        if (peek_and_consume(Token::KeyIf))
        {
            IfStatement::Segment segment;
            segment.condition = parse_expression();
            segment.code_block = parse_code_block(scope);
            segment.span = finish_span();
            if_statement->segments.push_back(segment);
        }
        else
        {
            if_statement->fallback = parse_code_block(scope);
            discard_span();
            break;
        }
    }

    if_statement->span = finish_span();
    return if_statement;
}

bool Parser::peek_variable_declaration()
{
    return peek(Token::KeyLet) || peek(Token::KeyVar);
}

ptr<VariableDeclaration> Parser::parse_variable_declaration(ptr<Scope> scope)
{
    auto assignment_stmt = CREATE(VariableDeclaration);
    auto variable = CREATE(Variable);
    assignment_stmt->variable = variable;

    start_span();
    if (peek_and_consume(Token::KeyVar))
    {
        variable->is_mutable = true;
    }
    else
    {
        confirm_and_consume(Token::KeyLet);
        variable->is_mutable = false;
    }

    // TODO: Mark this as an "unresolved pattern" that the type checker should then infer from inference
    variable->pattern = CREATE(AnyPattern);

    // TODO: Check what happens downstream in the compiler when attempting to compile a VariableDeclaration with
    //       an ill-defined variable (i.e. one where this if statement never runs.)
    if (confirm(Token::Identity))
    {
        variable->identity = consume(Token::Identity).str;

        if (!variable->is_mutable || peek(Token::Assign))
        {
            confirm_and_consume(Token::Assign);
            assignment_stmt->value = parse_expression();
        }

        declare(scope, variable);
    }

    Span span = finish_span();
    assignment_stmt->span = span;
    variable->span = span;
    return assignment_stmt;
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

// FIXME: This function will throw compiler error when the tokens expected are not present
ptr<UnresolvedIdentity> Parser::parse_unresolved_identity()
{
    auto identity = CREATE(UnresolvedIdentity);
    auto token = consume(Token::Identity);
    identity->identity = token.str;
    identity->span = to_span(token);
    return identity;
}

bool Parser::peek_expression()
{
    return peek_paren_expr() ||
           peek_match() ||
           peek_unary() ||
           peek(Token::Identity) ||
           peek_intrinsic_value() ||
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
    else if (peek_intrinsic_value())
        lhs = parse_intrinsic_value();
    else if (peek_list_value())
        lhs = parse_list_value();

    else
    {
        // FIXME: Currently, only the current token is turned into an invalid expression,
        //        should some larger portion of the code become invalid? (e.g. until the)
        //        end of the current line? maybe the current brackets?)
        Token token = consume();
        gambit_error("Expected expression", token);

        auto expr = CREATE(InvalidExpression);
        expr->span = to_span(token);
        return expr;
    }

    while (true)
    {
        if (peek_infix_factor() && operator_should_bind(Precedence::Factor, caller_precedence))
            lhs = parse_infix_factor(lhs);
        else if (peek_infix_term() && operator_should_bind(Precedence::Term, caller_precedence))
            lhs = parse_infix_term(lhs);
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

bool Parser::peek_match()
{
    return peek(Token::KeyMatch);
}

ptr<Match> Parser::parse_match()
{
    auto match = CREATE(Match);

    start_span();
    confirm_and_consume(Token::KeyMatch);

    match->subject = parse_expression();

    if (confirm_and_consume(Token::CurlyL))
    {
        while (peek_pattern(true) && !match->has_fallback_rule)
        {
            Match::Rule rule;

            start_span();
            rule.pattern = parse_pattern(true);
            if (IS_PTR(rule.pattern, AnyPattern))
                match->has_fallback_rule = true;

            confirm_and_consume(Token::Colon);
            rule.result = parse_expression();
            rule.span = finish_span();
            confirm_and_consume(Token::Line);

            match->rules.emplace_back(rule);
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

bool Parser::peek_intrinsic_value()
{
    return peek(Token::Number) ||
           peek(Token::String) ||
           peek(Token::Boolean);
}

Expression Parser::parse_intrinsic_value()
{
    auto intrinsic_value = CREATE(IntrinsicValue);
    auto token = consume();

    if (token.kind == Token::Number)
    {
        if (token.str.find(".") != std::string::npos)
        {
            intrinsic_value->value = stod(token.str);
            intrinsic_value->type = Intrinsic::type_num;
        }
        else
        {
            intrinsic_value->value = stoi(token.str);
            intrinsic_value->type = Intrinsic::type_amt; // We use `amt` instead of `int` as number literals cannot be negative
        }
    }

    else if (token.kind == Token::String)
    {
        intrinsic_value->value = token.str;
        intrinsic_value->type = Intrinsic::type_str;
    }

    else if (token.kind == Token::Boolean)
    {
        intrinsic_value->value = token.str == "true";
        intrinsic_value->type = Intrinsic::type_bool;
    }

    else
    {
        gambit_error("Expected literal", token);
        auto invalid_value = CREATE(InvalidValue);
        invalid_value->span = to_span(token);
        return invalid_value;
    }

    intrinsic_value->span = to_span(token);
    return intrinsic_value;
}

bool Parser::peek_list_value()
{
    return peek(Token::SquareL);
}

ptr<ListValue> Parser::parse_list_value()
{
    auto list = CREATE(ListValue);

    start_span();

    if (confirm_and_consume(Token::SquareL))
    {
        if (peek_expression())
        {
            do
            {
                list->values.push_back(parse_expression());
            } while (peek_and_consume(Token::Comma));
        }

        confirm_and_consume(Token::SquareR);
    }

    list->span = finish_span();
    return list;
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
    auto unresolved_identity = parse_unresolved_identity();
    property_index->property = unresolved_identity;

    property_index->span = merge(get_span(property_index->expr), unresolved_identity->span);
    return property_index;
}

// PATTERNS

bool Parser::peek_pattern(bool allow_intrinsic_values)
{
    return peek(Token::Identity) ||
           peek(Token::KeyAny) ||
           (peek_intrinsic_value() && allow_intrinsic_values);
}

Pattern Parser::parse_pattern(bool allow_intrinsic_values)
{
    // "Any" pattern
    if (peek(Token::KeyAny))
    {
        auto keyword = consume(Token::KeyAny);
        auto any_pattern = CREATE(AnyPattern);
        any_pattern->span = to_span(keyword);
        return any_pattern;
    }

    // Intrinsic value pattern
    Pattern pattern;
    if (peek_intrinsic_value() && allow_intrinsic_values)
    {
        auto expr = parse_intrinsic_value();
        if (IS_PTR(expr, IntrinsicValue))
        {
            auto intrinsic_value = AS_PTR(expr, IntrinsicValue);
            pattern = intrinsic_value;
        }
        else if (IS_PTR(expr, InvalidValue))
        {
            auto invalid_value = AS_PTR(expr, InvalidValue);
            auto invalid_pattern = CREATE(InvalidPattern);
            invalid_pattern->span = invalid_value->span;
            pattern = invalid_pattern;
        }
        else
        {
            throw CompilerError("Unable to resolve expression variant when creating pattern from intrinsic value", get_span(expr));
        }
    }

    // Unresolved identity
    else
    {
        pattern = parse_unresolved_identity();
    }

    // Optional pattern
    if (peek(Token::Question))
    {
        auto question = consume(Token::Question);
        auto optional_pattern = create_union_pattern(pattern, Intrinsic::none_val);
        optional_pattern->span = merge(get_span(pattern), to_span(question));
        return optional_pattern;
    }

    return pattern;
}

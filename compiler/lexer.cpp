#include "errors.h"
#include "lexer.h"
#include "token.h"

void Lexer::tokenise(Source &source)
{
    size_t line = 1;
    size_t column = 1;
    size_t position = 0;
    string sub = source.content;

    auto advance = [&](int amt)
    {
        column += amt;
        position += amt;
        sub = source.substr(position);
    };

    auto advance_line = [&]()
    {
        line += 1;
        column = 1;
        position += 1;
        sub = source.substr(position);
    };

    size_t multi_line_comment_nesting = 0;
    bool is_line_comment = false;
    bool panic_mode = false;

    Token phantom_newline;
    bool insert_phantom_newline = false;

    while (position < source.length)
    {
        bool error_occurred = false;
        string next = sub.substr(0, 1);
        string next_two = sub.substr(0, 2);

        if (is_line_comment)
        {
            if (next == "\n")
            {
                advance_line();
                is_line_comment = false;
            }
            else
            {
                advance(1);
            }
        }

        else if (next_two == "/*")
        {
            if (multi_line_comment_nesting == 0)
            {
                phantom_newline = Token(Token::Line, "\n", line, column, position);
                insert_phantom_newline = false;
            }

            multi_line_comment_nesting++;
            advance(2);
        }

        else if (multi_line_comment_nesting > 0)
        {
            if (next_two == "*/")
            {
                multi_line_comment_nesting--;
                advance(2);

                if (multi_line_comment_nesting == 0 && insert_phantom_newline)
                {
                    source.tokens.emplace_back(phantom_newline);
                }
            }
            else if (next == "\n")
            {
                advance_line();
                insert_phantom_newline = true;
            }
            else
            {
                advance(1);
            }
        }

        else if (next_two == "//")
        {
            source.tokens.emplace_back(Token(Token::Line, "\n", line, column, position));
            is_line_comment = true;
            advance(2);
        }

        else if (next == "\n")
        {
            source.tokens.emplace_back(Token(Token::Line, "\n", line, column, position));
            advance_line();
        }

        else if (next == " " || next == "\t")
        {
            advance(1);
        }

        else
        {
            bool character_parsed = false;
            for (auto rule : token_match_rules)
            {
                smatch info;
                if (regex_search(sub, info, rule.second, regex_constants::match_continuous))
                {
                    Token::Kind kind = rule.first;
                    string str = info.str();

                    if (kind == Token::Identity)
                    {
                        for (auto key_rule : keyword_match_rules)
                        {
                            if (str == key_rule.first)
                            {
                                kind = key_rule.second;
                                break;
                            }
                        }
                    }

                    source.tokens.emplace_back(Token(kind, str, line, column, position));
                    advance(info.length());
                    character_parsed = true;
                    break;
                }
            }

            if (!character_parsed)
            {
                if (!panic_mode)
                    source.log_error("Could not parse character '" + next + "', syntax not recognised.", line, column);
                advance(1);
                panic_mode = true;
                error_occurred = true;
            }
        }

        panic_mode = error_occurred;
    }

    source.tokens.emplace_back(Token(Token::EndOfFile, "", line, column, position));
}

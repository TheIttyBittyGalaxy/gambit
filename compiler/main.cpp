#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>
using namespace std;

// TOKENS //

enum class TokenKind
{
    Line,

    Equal,
    LessThanEqual,
    GreaterThanEqual,

    Add,
    Sub,
    Mul,
    Div,
    Dot,
    Comma,
    Colon,
    Question,
    Assign,
    ParenL,
    ParenR,
    CurlyL,
    CurlyR,
    SquareL,
    SquareR,
    TrigL,
    TrigR,

    KeyEnum,
    KeyExtend,
    KeyConstruct,
    KeyStatic,
    KeyState,
    KeyProperty,
    KeyBreak,
    KeyContinue,
    KeyElse,
    KeyFor,
    KeyIf,
    KeyIn,
    KeyMatch,
    KeyReturn,
    KeyUntil,
    KeyChoose,
    KeyFilter,
    KeyTransform,
    KeyAnd,
    KeyOr,
    KeyNot,

    Boolean,
    Number,
    String,
    Identity,
};

const map<TokenKind, string> token_name = {
    {TokenKind::Line, "Line"},

    {TokenKind::Equal, "Equal"},
    {TokenKind::LessThanEqual, "LessThanEqual"},
    {TokenKind::GreaterThanEqual, "GreaterThanEqual"},

    {TokenKind::Add, "Add"},
    {TokenKind::Sub, "Sub"},
    {TokenKind::Mul, "Mul"},
    {TokenKind::Div, "Div"},
    {TokenKind::Dot, "Dot"},
    {TokenKind::Comma, "Comma"},
    {TokenKind::Colon, "Colon"},
    {TokenKind::Question, "Question"},
    {TokenKind::Assign, "Assign"},
    {TokenKind::ParenL, "ParenL"},
    {TokenKind::ParenR, "ParenR"},
    {TokenKind::CurlyL, "CurlyL"},
    {TokenKind::CurlyR, "CurlyR"},
    {TokenKind::SquareL, "SquareL"},
    {TokenKind::SquareR, "SquareR"},
    {TokenKind::TrigL, "TrigL"},
    {TokenKind::TrigR, "TrigR"},

    {TokenKind::KeyEnum, "KeyEnum"},
    {TokenKind::KeyExtend, "KeyExtend"},
    {TokenKind::KeyConstruct, "KeyConstruct"},
    {TokenKind::KeyStatic, "KeyStatic"},
    {TokenKind::KeyState, "KeyState"},
    {TokenKind::KeyProperty, "KeyProperty"},
    {TokenKind::KeyBreak, "KeyBreak"},
    {TokenKind::KeyContinue, "KeyContinue"},
    {TokenKind::KeyElse, "KeyElse"},
    {TokenKind::KeyFor, "KeyFor"},
    {TokenKind::KeyIf, "KeyIf"},
    {TokenKind::KeyIn, "KeyIn"},
    {TokenKind::KeyMatch, "KeyMatch"},
    {TokenKind::KeyReturn, "KeyReturn"},
    {TokenKind::KeyUntil, "KeyUntil"},
    {TokenKind::KeyChoose, "KeyChoose"},
    {TokenKind::KeyFilter, "KeyFilter"},
    {TokenKind::KeyTransform, "KeyTransform"},
    {TokenKind::KeyAnd, "KeyAnd"},
    {TokenKind::KeyOr, "KeyOr"},
    {TokenKind::KeyNot, "KeyNot"},

    {TokenKind::Boolean, "Boolean"},
    {TokenKind::Number, "Number"},
    {TokenKind::String, "String"},
    {TokenKind::Identity, "Identity"},
};

const map<TokenKind, regex> token_match_rules = {
    {TokenKind::Line, regex("\n")},

    {TokenKind::Equal, regex("==")},
    {TokenKind::LessThanEqual, regex("<=")},
    {TokenKind::GreaterThanEqual, regex(">=")},

    {TokenKind::Add, regex("\\+")},
    {TokenKind::Sub, regex("\\-")},
    {TokenKind::Mul, regex("\\*")},
    {TokenKind::Div, regex("\\\\")},
    {TokenKind::Dot, regex("\\.")},
    {TokenKind::Comma, regex("\\,")},
    {TokenKind::Colon, regex("\\:")},
    {TokenKind::Question, regex("\\?")},
    {TokenKind::Assign, regex("\\=")},
    {TokenKind::ParenL, regex("\\(")},
    {TokenKind::ParenR, regex("\\)")},
    {TokenKind::CurlyL, regex("\\{")},
    {TokenKind::CurlyR, regex("\\}")},
    {TokenKind::SquareL, regex("\\[")},
    {TokenKind::SquareR, regex("\\]")},
    {TokenKind::TrigL, regex("\\<")},
    {TokenKind::TrigR, regex("\\>")},

    {TokenKind::Number, regex("[0-9]+(\\.[0-9]+)?")},
    {TokenKind::String, regex("\"(\\.|.)*?\"")}, // FIXME: This regular expression matches "\", which is incorrect
    {TokenKind::Identity, regex("[a-zA-Z][a-zA-Z0-9_]*")},
};

const map<string, TokenKind> keyword_match_rules = {
    {"enum", TokenKind::KeyEnum},
    {"extend", TokenKind::KeyExtend},
    {"construct", TokenKind::KeyConstruct},

    {"static", TokenKind::KeyStatic},
    {"state", TokenKind::KeyState},
    {"property", TokenKind::KeyProperty},

    {"break", TokenKind::KeyBreak},
    {"continue", TokenKind::KeyContinue},
    {"else", TokenKind::KeyElse},
    {"for", TokenKind::KeyFor},
    {"if", TokenKind::KeyIf},
    {"in", TokenKind::KeyIn},
    {"match", TokenKind::KeyMatch},
    {"return", TokenKind::KeyReturn},
    {"until", TokenKind::KeyUntil},

    {"choose", TokenKind::KeyChoose},
    {"filter", TokenKind::KeyFilter},
    {"transform", TokenKind::KeyTransform},

    {"and", TokenKind ::KeyAnd},
    {"or", TokenKind::KeyOr},
    {"not", TokenKind::KeyNot},

    {"true", TokenKind::Boolean},
    {"false", TokenKind::Boolean},
};

struct Token
{
    TokenKind kind;
    string str;
    size_t line;
    size_t column;

    Token(TokenKind kind, string str, size_t line, size_t column) : kind(kind),
                                                                    str(str),
                                                                    line(line),
                                                                    column(column) {}
};

string to_string(Token t)
{
    if (t.kind == TokenKind::Line)
    {
        return "[/]";
    }
    return "[" + token_name.at(t.kind) + " " + t.str + "]";
}

// ERRORS //

vector<string> errors;
bool panic_mode = false;

void emit_error(string msg)
{
    if (panic_mode)
        return;

    errors.emplace_back("[Error] " + msg);
    panic_mode = true;
}

void emit_error(string msg, size_t line, size_t column)
{
    if (panic_mode)
        return;

    errors.emplace_back("[Error at " + to_string(line) + ":" + to_string(column) + "] " + msg);
    panic_mode = true;
}

void emit_error(string msg, Token t)
{
    emit_error(msg, t.line, t.column);
}

// LEXING //

vector<Token> generate_tokens(string src)
{
    vector<Token> tokens;
    size_t line = 1;
    size_t column = 1;
    size_t position = 0;
    string sub = src;

    bool char_error = false;
    bool previous_char_error = false;

    auto advance = [&](int amt)
    {
        column += amt;
        position += amt;
        sub = src.substr(position);
    };

    auto advance_line = [&]()
    {
        line += 1;
        column = 1;
        position += 1;
        sub = src.substr(position);
    };

    size_t multi_line_comment_nesting = 0;
    bool is_line_comment = false;

    while (position < src.length())
    {
        string next = sub.substr(0, 1);
        string next_two = sub.substr(0, 2);

        if (is_line_comment)
        {
            if (next == "\n")
            {
                tokens.emplace_back(TokenKind::Line, "\n", line, column);
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
            multi_line_comment_nesting++;
            advance(2);
        }

        else if (multi_line_comment_nesting > 0)
        {
            if (next_two == "*/")
            {
                multi_line_comment_nesting--;
                advance(2);
            }
            else if (next == "\n")
            {
                advance_line();
            }
            else
            {
                advance(1);
            }
        }

        else if (next_two == "//")
        {
            is_line_comment = true;
            advance(2);
        }

        else if (next == "\n")
        {
            tokens.emplace_back(TokenKind::Line, "\n", line, column);
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
                    TokenKind kind = rule.first;
                    string str = info.str();

                    if (kind == TokenKind::Identity)
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

                    tokens.emplace_back(kind, str, line, column);
                    advance(info.length());
                    character_parsed = true;
                    break;
                }
            }

            if (!character_parsed)
            {
                emit_error("Unrecognised character " + next, line, column); // FIXME: Make this a proper error message.}
                advance(1);
                char_error = true;
            }
        }

        if (previous_char_error && !char_error)
        {
            panic_mode = false;
        }
        previous_char_error = char_error;
        char_error = false;
    }

    return tokens;
}

// MAIN //

int main(int argc, char *argv[])
{
    // FIXME: Allow the path to the source project to be passed directly, rather than infered.
    //        I've just made it easier this way for now to make testing easier for myself.

    // FIXME: Allow for generation of multiple source files.

    string game = (argc == 2) ? (string)argv[1] : "tic-tac-toe";
    string src_path = "game/" + game + "/main.gambit";

    // File loading

    ifstream src_file;
    src_file.open(src_path, ios::in);
    if (!src_file)
    {
        cout << "Source file " << src_path << " could not be loaded" << endl;
        return 0;
    }

    string src((istreambuf_iterator<char>(src_file)), istreambuf_iterator<char>());
    src_file.close();

    // Compile

    auto tokens = generate_tokens(src);

    for (auto t : tokens)
        cout << to_string(t) << endl;
    cout << endl;

    // for (auto t : tokens)
    //     cout << t.str << " ";
    // cout << endl;

    for (auto err : errors)
        cout << err << "\n\n";
    cout << endl;

    return 0;
}
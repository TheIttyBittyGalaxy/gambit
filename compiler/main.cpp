#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>
using namespace std;

// MACROS //

#define first(x) get<0>(x)
#define second(x) get<1>(x)

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

    {TokenKind::Number, "Number"},
    {TokenKind::String, "String"},
    {TokenKind::Identity, "Identity"},
};

const tuple<TokenKind, regex> token_match_rules[] = {
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
    {TokenKind::String, regex("\"(\\.|.)*?\"")},           // FIXME: This regular expression matches "\", which is incorrect
    {TokenKind::Identity, regex("[a-zA-Z][a-zA-Z0-9_]*")}, // FIXME: Parse keywords as seperate tokens
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
                if (regex_search(sub, info, second(rule), regex_constants::match_continuous))
                {
                    tokens.emplace_back(first(rule), info.str(), line, column);
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

    for (auto err : errors)
        cout << err << "\n\n";
    cout << endl;

    for (auto t : tokens)
        cout << t.str << " ";
    cout << endl;

    return 0;
}
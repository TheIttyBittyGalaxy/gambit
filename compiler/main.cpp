#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <variant>
#include <vector>
using namespace std;

// POINTERS //

template <class T>
using ptr = shared_ptr<T>;

template <class T>
using wptr = weak_ptr<T>;

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

void emit_error(string msg)
{
    errors.emplace_back("[Error] " + msg);
}

void emit_error(string msg, size_t line, size_t column)
{
    errors.emplace_back("[Error at " + to_string(line) + ":" + to_string(column) + "] " + msg);
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
    bool panic_mode = false;

    while (position < src.length())
    {
        bool error_occurred = false;
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
                if (!panic_mode)
                    emit_error("Unrecognised character " + next, line, column); // FIXME: Make this a proper error message.}
                advance(1);
                panic_mode = true;
                error_occurred = true;
            }
        }

        panic_mode = error_occurred;
    }

    return tokens;
}

// PROGRAM MODEL //

struct Program;
struct Scope;

struct NativeType;
struct Construct;
struct ConstructField;
using Entity = std::variant<ptr<NativeType>, ptr<Construct>>;

struct Literal;
using Expression = std::variant<ptr<Literal>>;

// Macros

#define IS(variant_value, T) (holds_alternative<ptr<T>>(variant_value))
#define AS(variant_value, T) (get<ptr<T>>(variant_value))

// Program

struct Program
{
    ptr<Scope> global_scope;
};

struct Scope
{
    wptr<Scope> parent;
    map<string, Entity> lookup;
};

// Entities

struct NativeType
{
    string identity;
    string cpp_identity;
};

struct Construct
{
    string identity;
    map<string, ptr<ConstructField>> fields;
};

struct ConstructField
{
    string identity;
    string type;
    bool is_static = false;
    bool is_property = false;
};

// Expressions

struct Literal
{
    variant<double, int, bool, string> value;
};

// PARSER //

class Parser
{
public:
    ptr<Program> parse(vector<Token> new_tokens)
    {
        tokens = new_tokens;
        current_token = 0;
        panic_mode = false;

        parse_program();
        return program;
    };

private:
    vector<Token> tokens;
    ptr<Program> program = nullptr;
    size_t current_token;
    bool panic_mode;

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

    bool end_of_file()
    {
        return current_token >= tokens.size();
    }

    bool peek(TokenKind kind)
    {
        if (end_of_file())
            return false;

        if (kind == TokenKind::Line)
            return tokens.at(current_token).kind == kind;

        size_t i = current_token;
        while (tokens.at(i).kind == TokenKind::Line)
            i++;
        return tokens.at(i).kind == kind;
    };

    Token eat(TokenKind kind)
    {
        if (end_of_file())
        {
            Token token = tokens.at(tokens.size() - 1);
            throw Error("Expected " + token_name.at(kind) + ", got end of file", token);
        }

        Token token = tokens.at(current_token);

        if (!peek(kind))
        {
            TokenKind other = token.kind;
            throw Error("Expected " + token_name.at(kind) + ", got " + token_name.at(other), token);
        }

        if (kind != TokenKind::Line)
        {
            while (token.kind == TokenKind::Line)
            {
                current_token++;
                token = tokens.at(current_token);
            }
        }

        current_token++;
        return token;
    };

    bool match(TokenKind kind)
    {
        if (peek(kind))
        {
            eat(kind);
            return true;
        }
        return false;
    };

    void parse_program()
    {
        program = ptr<Program>(new Program);
        program->global_scope = ptr<Scope>(new Scope);

        while (current_token < tokens.size())
        {
            try
            {
                while (match(TokenKind::Line))
                    if (end_of_file())
                        break;

                if (peek_construct_definition())
                {
                    panic_mode = false;
                    parse_construct_definition(program->global_scope);
                }
                else
                {
                    throw Error("Expected Construct definition", tokens.at(current_token));
                }
            }
            catch (Error err)
            {
                if (!panic_mode)
                    emit_error(err.msg, err.token);
                panic_mode = true;

                while (!match(TokenKind::Line))
                    eat(tokens.at(current_token).kind);
            }
        }
    };

    bool peek_construct_definition()
    {
        return peek(TokenKind::KeyExtend);
    }

    void parse_construct_definition(ptr<Scope> scope)
    {
        auto construct = ptr<Construct>(new Construct);

        eat(TokenKind::KeyExtend);
        construct->identity = eat(TokenKind::Identity).str;

        // FIXME: Use some form of "declare" function, rather than adding to the map directly
        scope->lookup.insert({construct->identity, construct});

        eat(TokenKind::CurlyL);
        while (peek_construct_field())
        {
            parse_construct_field(scope, construct);
        }
        eat(TokenKind::CurlyR);
    };

    bool peek_construct_field()
    {
        return peek(TokenKind::KeyStatic) || peek(TokenKind::KeyState) || peek(TokenKind::KeyProperty);
    }

    void parse_construct_field(ptr<Scope> scope, ptr<Construct> construct)
    {
        auto field = ptr<ConstructField>(new ConstructField);
        if (match(TokenKind::KeyStatic))
            field->is_static = true;
        else if (match(TokenKind::KeyProperty))
            field->is_property = true;
        else
            eat(TokenKind::KeyState);

        field->type = eat(TokenKind::Identity).str;
        field->identity = eat(TokenKind::Identity).str;

        construct->fields.insert({field->identity, field});
    }
};

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

    cout << "\nLEXING" << endl;
    auto tokens = generate_tokens(src);

    // for (auto t : tokens)
    //     cout << to_string(t) << endl;
    // cout << endl;

    // for (auto t : tokens)
    //     cout << t.str << " ";
    // cout << endl;

    cout << "\nPARSING" << endl;
    Parser parser;
    auto program = parser.parse(tokens);

    cout << "\nERRORS" << endl;
    for (auto err : errors)
        cout << err << endl;
    cout << endl;

    return 0;
}
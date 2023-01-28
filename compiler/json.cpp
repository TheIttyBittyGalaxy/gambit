#include "json.h"

string to_json(const JsonContainer &value, const size_t &depth)
{
    return (string)value;
}

string to_json(const int &value, const size_t &depth)
{
    return to_string(value);
}

string to_json(const double &value, const size_t &depth)
{
    return to_string(value);
}

string to_json(const bool &value, const size_t &depth)
{
    return value ? "true" : "false";
}

string to_json(const monostate &value, const size_t &depth)
{
    return "null";
}

string to_json(const string &value, const size_t &depth)
{
    string json;
    for (const char c : value)
    {
        switch (c)
        {
        case '\"':
            json += "\\\"";
            break;
        case '\\':
            json += "\\\\";
            break;
        case '\b':
            json += "\\b";
            break;
        case '\f':
            json += "\\f";
            break;
        case '\n':
            json += "\\n";
            break;
        case '\r':
            json += "\\r";
            break;
        case '\t':
            json += "\\t";
            break;
        default:
            if (c >= 0 && c <= 31)
            {
                char buf[8];
                snprintf(buf, sizeof buf, "\\u%04x", c);
                json += buf;
                continue;
            }
            else
            {
                json += c;
            }
        }
    }
    return "\"" + json + "\"";
}
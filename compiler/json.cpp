#include "json.h"

string to_json(JsonContainer value)
{
    return (string)value;
};

string to_json(int value)
{
    return to_string(value);
}

string to_json(double value)
{
    return to_string(value);
}

string to_json(bool value)
{
    return value ? "true" : "false";
}

string to_json(monostate value)
{
    return "null";
}

string to_json(string value)
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
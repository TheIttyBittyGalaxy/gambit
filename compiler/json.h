#ifndef JSON_H
#define JSON_H

#include <map>
#include <optional>
#include <stack>
#include <string>
#include <variant>
#include <vector>
using namespace std;

// Forward declarations

class JsonContainer;

string to_json(const JsonContainer &value, const size_t &depth = 0);
string to_json(const int &value, const size_t &depth = 0);
string to_json(const double &value, const size_t &depth = 0);
string to_json(const bool &value, const size_t &depth = 0);
string to_json(const monostate &value, const size_t &depth = 0);
string to_json(const string &value, const size_t &depth = 0);

template <typename T>
string to_json(const optional<T> &opt, const size_t &depth = 0);

template <typename T>
string to_json(const vector<T> &value, const size_t &depth = 0);

template <typename T>
string to_json(const map<string, T> &value, const size_t &depth = 0);

// Json container

class JsonContainer
{
private:
    enum class Container
    {
        None,
        Array,
        Object
    };

    string result = "";
    size_t base_depth = 0;
    stack<Container> container_stack;
    bool is_first_in_container = true;

    size_t stack_depth() const
    {
        return container_stack.size();
    }

    size_t depth() const
    {
        return base_depth + stack_depth();
    }

    Container current_container() const
    {
        if (stack_depth() == 0)
            return Container::None;
        return container_stack.top();
    }

public:
    JsonContainer(){};
    JsonContainer(size_t depth) : base_depth(depth){};

    void new_line()
    {
        result += "\n" + string(depth(), '\t');
    }

    void on_add_value()
    {
        if (is_first_in_container)
        {
            is_first_in_container = false;
            return;
        }

        result += ",";
        new_line();
    }

    void array()
    {
        on_add_value();
        container_stack.emplace(Container::Array);
        is_first_in_container = true;

        result += "[";
        new_line();
    }

    void object()
    {
        on_add_value();
        container_stack.emplace(Container::Object);
        is_first_in_container = true;

        result += "{";
        new_line();
    }

    void close()
    {
        if (stack_depth() == 0)
            throw runtime_error("Cannot close JSON container as no container has been opened."); // FIXME: Use a more appropriate exception type

        Container container = current_container();
        container_stack.pop();

        if (!is_first_in_container)
            new_line();
        is_first_in_container = false;

        result += (container == Container::Array) ? "]" : "}";
    }

    template <typename T>
    void
    add(T value)
    {
        if (current_container() != Container::Array)
            throw runtime_error("Cannot add a value as current container is not an Array."); // FIXME: Use a more appropriate exception type

        on_add_value();
        result += to_json(value, depth());
    };

    template <typename T>
    void add(string key, T value)
    {
        if (current_container() != Container::Object)
            throw runtime_error("Cannot add a key-value pair as current container is not an Object."); // FIXME: Use a more appropriate exception type

        on_add_value();
        result += to_json(key) + ": " + to_json(value, depth());
    };

    operator string() const
    {
        return result;
    };
};

// to_json implementations

template <typename T>
string to_json(const optional<T> &opt, const size_t &depth)
{
    return opt.has_value() ? to_json(opt.value()) : to_json(monostate());
}

template <typename T>
string to_json(const vector<T> &value, const size_t &depth)
{
    JsonContainer json(depth);
    json.array();
    for (auto elem : value)
        json.add(elem);
    json.close();
    return (string)json;
}

template <typename T>
string to_json(const map<string, T> &value, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    for (auto entry : value)
        json.add(entry.first, entry.second);
    json.close();
    return (string)json;
}

#endif
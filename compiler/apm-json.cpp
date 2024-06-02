#include "apm.h"
#include "json.h"

// Macros

#define STRUCT_FIELD(field) json.add(#field, node.field);

#define STRUCT_PTR_FIELD(field) json.add(#field, node->field);

#define STRUCT_PTR_FIELD_IDENTITY(field) json.add(#field, node->field->identity);

#define VARIANT(T)   \
    if (IS(node, T)) \
        return to_json(AS(node, T), depth);

#define VARIANT_PTR(T)   \
    if (IS_PTR(node, T)) \
        return to_json(AS_PTR(node, T), depth);

#define VARIANT_PTR_IDENTITY(T) \
    if (IS_PTR(node, T))        \
        return to_json(AS_PTR(node, T)->identity, depth);

// Serialisation

string to_json(const ptr<Program> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Program"));
    STRUCT_PTR_FIELD(global_scope);
    json.close();
    return (string)json;
}

string to_json(const ptr<CodeBlock> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("CodeBlock"));
    STRUCT_PTR_FIELD(singleton_block);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(statements);
    json.close();
    return (string)json;
}

string to_json(const Scope::LookupValue &node, const size_t &depth)
{
    VARIANT_PTR(Variable);
    VARIANT_PTR(UnionPattern);
    VARIANT_PTR(IntrinsicType);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);
    VARIANT_PTR(StateProperty);
    VARIANT_PTR(FunctionProperty);
    VARIANT_PTR(Procedure);
    VARIANT_PTR(Scope::OverloadedIdentity);

    throw json_serialisation_error("Could not serialise Scope::LookupValue variant.");
}

string to_json(const ptr<Scope::OverloadedIdentity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope::OverloadedIdentity"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(overloads);
    json.close();
    return (string)json;
}

string to_json(const ptr<Scope> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope"));
    STRUCT_PTR_FIELD(lookup);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidStatement"));
    json.close();
    return (string)json;
}

string to_json(const ptr<UnresolvedIdentity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UnresolvedIdentity"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<UninferredPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UninferredPattern"));
    json.close();
    return (string)json;
}

string to_json(const ptr<Variable> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Variable"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    STRUCT_PTR_FIELD(is_mutable);
    json.close();
    return (string)json;
}

string to_json(const ptr<AnyPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("AnyPattern"));
    json.close();
    return (string)json;
}

string to_json(const ptr<UnionPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UnionPattern"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(patterns);
    json.close();
    return (string)json;
}

string to_json(const ptr<ListPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ListPattern"));
    STRUCT_PTR_FIELD(list_of);
    STRUCT_PTR_FIELD(fixed_size);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidPattern"));
    json.close();
    return (string)json;
}

string to_json(const ptr<EnumType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumType"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<EnumValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumValue"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<Entity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Entity"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<StateProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("StateProperty"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(initial_value);
    json.close();
    return (string)json;
}

string to_json(const ptr<FunctionProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("FunctionProperty"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(body);
    json.close();
    return (string)json;
}

string to_json(const ptr<Procedure> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Procedure"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(body);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidProperty"));
    json.close();
    return (string)json;
}

string to_json(const Property &node, const size_t &depth)
{

    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(StateProperty);
    VARIANT_PTR(FunctionProperty);
    VARIANT_PTR(InvalidProperty);

    throw json_serialisation_error("Could not serialise Property variant.");
}

string to_json(const ptr<IntrinsicType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IntrinsicType"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(cpp_identity);
    json.close();
    return (string)json;
}

string to_json(const Pattern &node, const size_t &depth)
{
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(UninferredPattern);
    VARIANT_PTR(InvalidPattern);
    VARIANT_PTR(AnyPattern);
    VARIANT_PTR(UnionPattern);
    VARIANT_PTR(ListPattern);
    VARIANT_PTR_IDENTITY(IntrinsicType);
    VARIANT_PTR_IDENTITY(EnumType);
    VARIANT_PTR_IDENTITY(Entity);
    VARIANT_PTR(IntrinsicValue);
    VARIANT_PTR_IDENTITY(EnumValue);

    throw json_serialisation_error("Could not serialise Pattern variant.");
}

/*
string to_json(const ptr<IntrinsicValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IntrinsicValue"));
    if (IS(node->value, double))
        json.add("value", AS(node->value, double));
    else if (IS(node->value, int))
        json.add("value", AS(node->value, int));
    else if (IS(node->value, bool))
        json.add("value", AS(node->value, bool));
    else if (IS(node->value, string))
        json.add("value", AS(node->value, string));
    STRUCT_PTR_FIELD(type);
    json.close();
    return (string)json;
}
*/

string to_json(const ptr<IntrinsicValue> &node, const size_t &depth)
{
    if (IS(node->value, double))
        return to_json(AS(node->value, double));
    if (IS(node->value, int))
        return to_json(AS(node->value, int));
    if (IS(node->value, bool))
        return to_json(AS(node->value, bool));
    if (IS(node->value, string))
        return to_json(AS(node->value, string));

    throw json_serialisation_error("Could not serialise IntrinsicValue.");
}

string to_json(const ptr<ListValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ListValue"));
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<InstanceList> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InstanceList"));
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<Unary> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Unary"));
    STRUCT_PTR_FIELD(op);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<Binary> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Binary"));
    STRUCT_PTR_FIELD(op);
    STRUCT_PTR_FIELD(lhs);
    STRUCT_PTR_FIELD(rhs);
    json.close();
    return (string)json;
}

string to_json(const ptr<ExpressionIndex> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ExpressionIndex"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(index);
    json.close();
    return (string)json;
}

string to_json(const ptr<PropertyIndex> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PropertyIndex"));
    STRUCT_PTR_FIELD(expr);

    // TODO: Instead of just printing out the identity, print out a signature that
    //       allows different overloads to be distinguished.
    if (IS_PTR(node->property, FunctionProperty))
        json.add("property", AS_PTR(node->property, FunctionProperty)->identity);
    else if (IS_PTR(node->property, StateProperty))
        json.add("property", AS_PTR(node->property, StateProperty)->identity);
    else
        STRUCT_PTR_FIELD(property)

    json.close();
    return (string)json;
}

string to_json(const Match::Rule &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(pattern);
    STRUCT_FIELD(result);
    json.close();
    return (string)json;
}

string to_json(const ptr<Match> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Match"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(rules);
    STRUCT_PTR_FIELD(has_fallback_rule);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidValue"));
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidExpression> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidExpression"));
    json.close();
    return (string)json;
}

string to_json(const Expression &node, const size_t &depth)
{
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR_IDENTITY(Variable);
    VARIANT_PTR_IDENTITY(EnumValue);
    VARIANT_PTR(IntrinsicValue);
    VARIANT_PTR(ListValue);
    VARIANT_PTR(InstanceList);
    VARIANT_PTR(Unary);
    VARIANT_PTR(Binary);
    VARIANT_PTR(ExpressionIndex);
    VARIANT_PTR(PropertyIndex);
    VARIANT_PTR(Match);
    VARIANT_PTR(InvalidValue);
    VARIANT_PTR(InvalidExpression);

    throw json_serialisation_error("Could not serialise Expression variant.");
};

string to_json(const IfStatement::Segment &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(condition);
    STRUCT_FIELD(code_block);
    json.close();
    return (string)json;
}

string to_json(const ptr<IfStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IfStatement"));
    STRUCT_PTR_FIELD(segments);
    STRUCT_PTR_FIELD(fallback);
    json.close();
    return (string)json;
}

string to_json(const ptr<AssignmentStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("AssignmentStatement"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<VariableDeclaration> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("VariableDeclaration"));
    STRUCT_PTR_FIELD_IDENTITY(variable);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const Statement &node, const size_t &depth)
{
    VARIANT(Expression);
    VARIANT_PTR(CodeBlock);
    VARIANT_PTR(IfStatement);
    VARIANT_PTR(AssignmentStatement);
    VARIANT_PTR(VariableDeclaration);
    VARIANT_PTR(InvalidStatement);

    throw json_serialisation_error("Could not serialise Statement variant.");
};

#include "intrinsic.h"

namespace Intrinsic
{
    ptr<IntrinsicType> type_str = ptr<IntrinsicType>(new IntrinsicType({"str", "std::string"}));
    ptr<IntrinsicType> type_num = ptr<IntrinsicType>(new IntrinsicType({"num", "double"}));
    ptr<IntrinsicType> type_int = ptr<IntrinsicType>(new IntrinsicType({"int", "int"}));
    ptr<IntrinsicType> type_amt = ptr<IntrinsicType>(new IntrinsicType({"amt", "int"}));
    ptr<IntrinsicType> type_bool = ptr<IntrinsicType>(new IntrinsicType({"bool", "bool"}));

    ptr<Entity> entity_player = ptr<Entity>(new Entity({"Player"}));

    // FIXME: As of writing, the `player` parameter is never declared in the state's
    //        scope, as normally the parser would declare it. This may not be an issue
    //        for properties that are implemented intrinsically, but worth noting.
    //        Could it potentially be worth adding an `IntrinsicProperty` type?
    ptr<StateProperty> state_player_number = ptr<StateProperty>(new StateProperty({
        "number",                                                 // identity
        type_amt,                                                 // pattern
        CREATE(Scope),                                            // scope
        {ptr<Variable>(new Variable({"player", entity_player}))}, // parameters
        {}                                                        // initial_value
    }));
}
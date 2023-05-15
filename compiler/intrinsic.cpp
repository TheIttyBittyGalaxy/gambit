#include "intrinsic.h"
#include "span.h"

namespace Intrinsic
{
    // FIXME: Give intrinsics some kind of 'intrinsic spans. Eventually, if the compiler tries to access the span of an
    //        intrinsic, that should be a compiler error, as that's never going to make a useful error message for the user.
    //        For the purposes of building the compiler however, it would be useful to not have to deal with those compiler
    //        errors yet. (let alone the kind of memory segmentation errors that can occur due to null spans).

    ptr<IntrinsicType> type_str = ptr<IntrinsicType>(new IntrinsicType({"str", "std::string"}));
    ptr<IntrinsicType> type_num = ptr<IntrinsicType>(new IntrinsicType({"num", "double"}));
    ptr<IntrinsicType> type_int = ptr<IntrinsicType>(new IntrinsicType({"int", "int"}));
    ptr<IntrinsicType> type_amt = ptr<IntrinsicType>(new IntrinsicType({"amt", "int"}));
    ptr<IntrinsicType> type_bool = ptr<IntrinsicType>(new IntrinsicType({"bool", "bool"}));

    // FIXME: Is it worth adding an 'IntrinsicEntity' type?
    ptr<Entity> entity_player = ptr<Entity>(new Entity({Span(), "Player"}));

    // FIXME: As of writing, the `player` parameter is never declared in the state's scope, as normally the parser would
    //        declare it. This may not be an issue for properties that are implemented intrinsically, but worth noting.
    //        Could it potentially be worth adding an `IntrinsicProperty` type?
    ptr<StateProperty> state_player_number = ptr<StateProperty>(new StateProperty({
        Span(),                                                           // span
        "number",                                                         // identity
        type_amt,                                                         // pattern
        CREATE(Scope),                                                    // scope
        {ptr<Variable>(new Variable({Span(), "player", entity_player}))}, // parameters
        {}                                                                // initial_value
    }));
}
#include "intrinsic.h"

namespace Intrinsic
{
    ptr<IntrinsicType> type_str = ptr<IntrinsicType>(new IntrinsicType({"str", "std::string"}));
    ptr<IntrinsicType> type_num = ptr<IntrinsicType>(new IntrinsicType({"num", "double"}));
    ptr<IntrinsicType> type_int = ptr<IntrinsicType>(new IntrinsicType({"int", "int"}));
    ptr<IntrinsicType> type_amt = ptr<IntrinsicType>(new IntrinsicType({"amt", "int"}));
    ptr<IntrinsicType> type_bool = ptr<IntrinsicType>(new IntrinsicType({"bool", "bool"}));
}
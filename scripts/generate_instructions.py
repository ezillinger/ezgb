import os.path as path
import json

this_script_path = path.dirname(path.realpath(__file__))
json_path = path.realpath(path.join(this_script_path, "../data/opcodes_2.json"))

json_dict = None 
with open(json_path, "r") as f:
    json_dict = json.loads(f.read())
assert(json_dict)

# hack second version of opcodes.json to match first one
def apply_hacks(dict_name):
    for k,v in json_dict[dict_name].items():
        v["addr"] = k
        if len(v["operands"]) > 0:
            op1 = v["operands"][0]
            name = op1["name"]
            if "increment" in op1:
                name += "+"
            elif "decrement" in op1:
                name += "-"
            if "$" in name:
                name = name.removeprefix("$") + "h"
            op1name = name if op1["immediate"] == True else f"({name})"
            v["operand1"]  = op1name
        if len(v["operands"]) > 1:
            op2 = v["operands"][1]
            name :str = op2["name"]
            if "increment" in op2:
                name += "+"
            elif "decrement" in op2:
                name += "-"
            if "$" in name:
                name = name.removeprefix("$") + "h"
            op2name = name if op2["immediate"] == True else f"({name})"
            v["operand2"]  = op2name

apply_hacks("unprefixed")
apply_hacks("cbprefixed")

# we treat prefix as its own instruction
for k,v in json_dict["cbprefixed"].items():
    v["bytes"] = v["bytes"] - 1
    v["cycles"][0] = v["cycles"][0] - 4

codeHeader = \
"""#pragma once

/*
    This file is auto-generated by generate_instructions.py
*/ 

#include "Base.h"

namespace ez {

    enum class FlagEffect {
        NONE,
        UNSET,
        SET,
        ZERO,
        CARRY,
        HALF_CARRY,
        SUBTRACTION,
    };

    struct OpCodeInfo {
        bool m_prefixed = false;
        uint8_t m_addr = 0x00;
        const char* m_mnemonic = "";
        int m_size = 0;
        int m_cycles = 0;
        std::optional<int> m_cyclesIfBranch = {};
        FlagEffect m_flagZero = FlagEffect::NONE;
        FlagEffect m_flagSubtract = FlagEffect::NONE;
        FlagEffect m_flagHalfCarry = FlagEffect::NONE;
        FlagEffect m_flagCarry = FlagEffect::NONE;
        const char* m_operandName1 = "";
        const char* m_operandName2 = "";
    };
"""

codeFooter = \
"""
} // namespace ez

// format structs must be outside ez namespace 

template<>                                                   
struct std::formatter<ez::OpCodeInfo> {
    constexpr auto parse(std::format_parse_context& context) { 
        return context.begin();
    }

    template<typename TContext>
    auto format(const ez::OpCodeInfo& oc, TContext& context) const {  
        return std::format_to(context.out(), "{} {} {} (A: {:#2x} L: {})", oc.m_mnemonic, oc.m_operandName1, 
                                                oc.m_operandName2, oc.m_addr,
                                                oc.m_size);
    }
};

"""



def generate_switch_case(prefixed: bool, oc: dict) -> str:

    def parseFlag(c: str):
        if c == "-": 
            return "FlagEffect::NONE"
        if c == "0": 
            return "FlagEffect::UNSET"
        if c == "1": 
            return "FlagEffect::SET"
        if c == "Z": 
            return "FlagEffect::ZERO"
        if c == "H": 
            return "FlagEffect::HALF_CARRY"
        if c == "N": 
            return "FlagEffect::SUBTRACTION"
        if c == "C": 
            return "FlagEffect::CARRY"

        print("Invalid flag: ", c)
        assert(False)

    cyclesWo = oc["cycles"][0]
    cyclesW = oc["cycles"][1] if len(oc["cycles"]) == 2 else "{}"

    caseTemplate = \
f"""
        case {oc["addr"]}:
            return {{{"true" if prefixed else "false"}, 
                    {oc["addr"]}, 
                    "{oc["mnemonic"]}",
                    {oc["bytes"]},
                    {cyclesWo},
                    {cyclesW},
                    {parseFlag(oc["flags"]["Z"])},
                    {parseFlag(oc["flags"]["N"])},
                    {parseFlag(oc["flags"]["H"])},
                    {parseFlag(oc["flags"]["C"])},
                    "{oc.get("operand1", "")}",
                    "{oc.get("operand2", "")}",
                   }};
"""
    return caseTemplate

def generate_switches():
    # TODO split into header/cpp

    funcHeaderTemplate = \
"""
    inline OpCodeInfo {}(uint8_t code) {{

        switch(code){{
"""

    funcFooter = \
"""
        default:
            EZ_FAIL("Unknown OpCode: {}", code);
        }
    }

"""

    code = funcHeaderTemplate.format("getOpCodeInfoUnprefixed")
    for k, oc in json_dict["unprefixed"].items():
        code += generate_switch_case(False, oc)
    code += funcFooter

    code += funcHeaderTemplate.format("getOpCodeInfoPrefixed")
    for k, oc in json_dict["cbprefixed"].items():
        code += generate_switch_case(True, oc)
    code += funcFooter

    return code

def make_enum_name(oc: dict) -> str:
    name: str = oc["mnemonic"]
    if "operand1" in oc:
        name += f"_{oc["operand1"]}"
    if "operand2" in oc:
        name += f"_{oc["operand2"]}"

    name = name.replace("(", "_").replace(")", "_")
    name = name.replace("+", "plus").replace("-", "minus")

    return name


def generate_enums() -> str:
    enumHeader = \
"""
    enum class {} : uint8_t {{
"""
    enumFooter = \
"""    };

"""
    def make_row(oc: dict) -> str:
        name: str = make_enum_name(oc)
        
        return f"        {name} = {oc["addr"]}, // {oc["mnemonic"]} {oc.get("operand1", "")} {oc.get("operand2", "")}\n"

    code = enumHeader.format("OpCode")
    for k, oc in json_dict["unprefixed"].items():
        code += make_row(oc)
    code += enumFooter

    code += enumHeader.format("OpCodeCB")
    for k, oc in json_dict["cbprefixed"].items():
        code += make_row(oc)
    code += enumFooter
    return code


if __name__ == "__main__":

    code = codeHeader
    code += generate_enums()
    code += generate_switches()
    code += codeFooter

    with open(path.join(this_script_path, "../src/OpCodes.h"), "w") as f:
        f.write(code)



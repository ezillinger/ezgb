import os.path as path
import json

this_script_path = path.dirname(path.realpath(__file__))
json_path = path.realpath(path.join(this_script_path, "../data/opcodes.json"))

jsonDict = None 
with open(json_path, "r") as f:
    jsonDict = json.loads(f.read())

assert(jsonDict)
#print(json.dumps(jsonDict, indent=2))


codeHeader = """
#pragma once

/*
    This file is auto-generated by generate_instructions.py
*/ 

#include "base.h"

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

    struct OpCode {
        const bool m_prefixed = false;
        const uint8_t m_addr = 0x00;
        const char* m_mnemonic = nullptr;
        const int m_lengthBytes = 0;
        const int m_cyclesWithoutBranch = 0;
        const int m_cyclesWithBranch = 0;
        FlagEffect m_flagZero = FlagEffect::NONE;
        FlagEffect m_flagSubtract = FlagEffect::NONE;
        FlagEffect m_flagHalfCarry = FlagEffect::NONE;
        FlagEffect m_flagCarry = FlagEffect::NONE;
        const char* m_groupName = nullptr;
        const char* m_operandName1 = nullptr;
        const char* m_operandName2 = nullptr;
    };
    
        

"""

codeFooter = \
"""

} // namespace ez

// format structs must be outside ez namespace 

template<>                                                   
struct std::formatter<ez::OpCode> {
    constexpr auto parse(std::format_parse_context& context) { 
        return context.begin();
    }
    auto format(const ez::OpCode& oc, std::format_context& context) const {  
        return std::format_to(context.out(), "{} {} {} {} (A: {:#2x} L: {})", oc.m_mnemonic, oc.m_operandName1, 
                                                oc.m_operandName2, oc.m_groupName, oc.m_addr,
                                                oc.m_lengthBytes);
    }
};

"""



def generateSwitchCase(prefixed: bool, oc: dict) -> str:

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
    cyclesW = oc["cycles"][1] if len(oc["cycles"]) == 2 else 0

    caseTemplate = \
f"""
        case {oc["addr"]}:
            return {{{"true" if prefixed else "false"}, 
                    {oc["addr"]}, 
                    "{oc["mnemonic"]}",
                    {oc["length"]},
                    {cyclesWo},
                    {cyclesW},
                    {parseFlag(oc["flags"][0])},
                    {parseFlag(oc["flags"][1])},
                    {parseFlag(oc["flags"][2])},
                    {parseFlag(oc["flags"][3])},
                    "{oc["group"]}",
                    "{oc.get("operand1", "")}",
                    "{oc.get("operand2", "")}",
                   }};
"""
    return caseTemplate

def generateSwitches():
    funcHeaderTemplate = \
"""
    OpCode {}(uint8_t code) {{

        switch(code){{

"""

    funcFooter = \
"""
        default:
            log_error("Unknown OpCode: {}", code);
            EZ_FAIL();
        }
    }


"""

    code = funcHeaderTemplate.format("getOpCodeUnprefixed")
    for k, oc in jsonDict["unprefixed"].items():
        code += generateSwitchCase(False, oc)
    code += funcFooter

    code += funcHeaderTemplate.format("getOpCodePrefixed")
    for k, oc in jsonDict["cbprefixed"].items():
        code += generateSwitchCase(True, oc)
    code += funcFooter

    return code

if __name__ == "__main__":

    code = codeHeader
    code += generateSwitches()
    code += codeFooter

    with open(path.join(this_script_path, "../src/OpCodes.h"), "w") as f:
        f.write(code)



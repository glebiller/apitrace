#pragma once

#include <map>

namespace shaders {

    enum OPERATION {
        UNKNOWN_OP = 0,
        MOV          ,
        ADD          ,
        MAD          ,
        MUL          ,
        DP2          ,
        DP3          ,
        DP4          ,
        SQRT         ,
        FTOI         ,
        IMUL         ,
        RSQ          ,
        DIV          ,
        EXP          ,
        SINCOS       ,
    };

    const std::map<OPERATION, std::string> OperationToString = {
        {UNKNOWN_OP, "UNKNOWN"},
        {MOV, "mov"},
        {ADD, "add"},
        {MAD, "mad"},
        {MUL, "mul"},
        {DP2, "dp2"},
        {DP3, "dp3"},
        {DP4, "dp4"},
        {SQRT, "sqrt"},
        {FTOI, "ftoi"},
        {IMUL, "imul"},
        {RSQ, "rsq"},
        {DIV, "div"},
        {EXP, "exp"},
        {SINCOS, "sincos"},
    };

    const std::map<std::string, OPERATION> StringToOperation = {
        {"mov", MOV},
        {"add", ADD},
        {"mad", MAD},
        {"mul", MUL},
        {"dp2", DP2},
        {"dp3", DP3},
        {"dp4", DP4},
        {"sqrt", SQRT},
        {"ftoi", FTOI},
        {"imul", IMUL},
        {"rsq", RSQ},
        {"div", DIV},
        {"exp", EXP},
        {"sincos", SINCOS},
    };

    inline std::string operationToString(const OPERATION op) {
        if (const auto it = OperationToString.find(op); it != OperationToString.end()) {
            return it->second;
        }
        return "UNKNOWN_OP";
    }

    inline OPERATION stringToOperation(const std::string& str) {
        if (const auto it = StringToOperation.find(str); it != StringToOperation.end()) {
            return it->second;
        }
        return UNKNOWN_OP;
    }

}

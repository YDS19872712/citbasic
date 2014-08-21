#ifndef INTERPRETER_HPP_INCLUDED
#define INTERPRETER_HPP_INCLUDED

#include <cstdint>
#include <memory>
#include <map>
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include "Token.hpp"

class Interpreter
{
public:

    enum OperandType
    {
        OPERAND_TYPE_REAL,
        OPERAND_TYPE_INTEGER,
        OPERAND_TYPE_STRING,
        OPERAND_TYPE_BOOLEAN
    };

    bool load(std::istream& file);
    bool run();

private:

    typedef std::vector<Token>    VectorOfTokens;
    typedef std::unique_ptr<char> SourceLine;
    
    std::vector<VectorOfTokens> m_tokens;
    std::vector<SourceLine>     m_source;

    std::map<std::string, std::size_t> m_labels;
    
    std::map<std::string, long double>  m_realVars;
    std::map<std::string, std::int64_t> m_intVars;
    std::map<std::string, std::string>  m_strVars;

    std::stack<std::size_t> m_callStack;

    bool registerLabel(const Token& token);

    std::size_t execute(
        std::size_t line,
        std::size_t begin,
        std::size_t end);

    bool Interpreter::evaluate(
        VectorOfTokens::iterator& begin,
        VectorOfTokens::iterator  end,
        bool&                     boolResult,
        const std::string&        varResult = "",
        const OperandType         varType = OPERAND_TYPE_BOOLEAN);
};

#endif // INTERPRETER_HPP_INCLUDED

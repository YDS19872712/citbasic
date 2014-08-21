#include <cassert>
#include <cctype>
#include <cmath>
#include <ctime>
#include <sstream>
#include "Interpreter.hpp"

#pragma warning(disable: 4996)

bool Interpreter::load(std::istream& file)
{
    m_source.clear();
    m_tokens.clear();

    std::string buffer;

    while (!file.eof())
    { 
        buffer.clear();

        while (true)
        {
            std::string line;
            std::getline(file, line);

            while (!line.empty() && !std::isgraph(line[line.size() - 1]))
                line.resize(line.size() - 1);

            if (line.empty())
                break;

            if ('&' == line[line.size() - 1])
            {
                line.resize(line.size() - 1);
                buffer.append(line);
                buffer += ' ';
            }
            else
            {
                buffer.append(line);
                break;
            }
        }

        if (!buffer.empty())
        {
            Token token;

            m_tokens.push_back(VectorOfTokens());
            m_source.push_back(SourceLine(new char[buffer.size() + 1]));

            std::strcpy(m_source.back().get(), buffer.c_str());

            const char* end = m_source.back().get() + buffer.size();
            const char* current = token.parse(m_source.back().get(), end);

            bool expectColon = false;

            if (Token::LITERAL_INTEGER == token.getValue())
            {
                if (!registerLabel(token))
                    return false;

                current = token.parse(current, end);
            }
            else if (Token::IDENTIFIER_LABEL == token.getValue())
            {
                expectColon = true;
            }

            while ((Token::HAPPY_END   != token.getType()) &&
                   (Token::KEYWORD_REM != token.getValue()))
            {
                m_tokens.back().push_back(token);
                current = token.parse(current, end);

                if (expectColon &&
                    (Token::PUNCTUATION_MARK_COLON == token.getValue()))
                {
                    if (!registerLabel(m_tokens.back().back()))
                        return false;

                    m_tokens.back().pop_back();
                    current = token.parse(current, end);
                    expectColon = false;
                }
            }
        }
    }

    return true;
}


bool Interpreter::run()
{
    m_realVars.clear();
    m_intVars.clear();
    m_strVars.clear();

    std::time_t t;
    std::time(&t);
    std::srand(static_cast<unsigned int>(t));

    std::size_t line = 0;
    
    while (line < m_tokens.size())
    {
        std::size_t k = line;
        line = execute(line, 0, m_tokens[line].size());
        if (SIZE_MAX == line)
        {
            std::cerr << m_source[k].get() << std::endl;
            return false;
        }
    }

    return true;
}


bool Interpreter::registerLabel(const Token& token)
{
    assert(!m_tokens.empty());

    std::stringstream key;

    switch (token.getValue())
    {
    case Token::IDENTIFIER_LABEL:
        key << token.getIdentifier();
        break;

    case Token::LITERAL_INTEGER:
        key << token.getInteger();
        break;

    default:
        std::cerr << "Internal error!\n";
        return false;
    }

    std::pair<std::map<std::string, std::size_t>::iterator, bool> result =
        m_labels.insert(std::pair<std::string, std::size_t>(
            key.str(), m_tokens.size() - 1));

    if (!result.second)
    {
        std::cerr << "Duplicate label \'" << key.str() << "\'!\n";
        return false;
    }

    return true;
}


std::size_t Interpreter::execute(
    std::size_t line,
    std::size_t begin,
    std::size_t end)
{
    if (begin < m_tokens[line].size())
    {
    restart:

        switch(m_tokens[line][begin].getType())
        {
        case Token::TYPE_IDENTIFIER:
            if (end - begin >= 3)
            {
                Token::Value value = m_tokens[line][begin].getValue();

                OperandType operandType =
                    Token::IDENTIFIER_REAL ==  value
                        ? OPERAND_TYPE_REAL
                        : Token::IDENTIFIER_INTEGER == value
                            ? OPERAND_TYPE_INTEGER
                            : OPERAND_TYPE_STRING;

                value = m_tokens[line][begin + 1].getValue();
                if (Token::OPERATOR_EQUAL != value)
                {
                    std::cerr << "Assignment operator expected!\n";
                    return SIZE_MAX;
                }

                bool boolResult;

                if (!evaluate(
                    m_tokens[line].begin() + begin + 2, 
                    m_tokens[line].begin() + end,
                    boolResult,
                    m_tokens[line][begin].getIdentifier(),
                    operandType))
                {
                    return SIZE_MAX;
                }

                break;
            }
            std::cerr << "Incomplete assignment statement!\n";
            return SIZE_MAX;

        case Token::TYPE_KEYWORD:
            switch (m_tokens[line][begin].getValue())
            {
            case Token::KEYWORD_LET:
                if (++begin < end)
                    goto restart;
                std::cerr << "Incomplete LET statement!\n";
                return SIZE_MAX;

            case Token::KEYWORD_PRINT:
                begin++;
                while (begin < end)
                {
                    std::size_t i;

                    for (i = begin; i < end; i++)
                    {
                        if (Token::PUNCTUATION_MARK_SEMICOLON ==
                            m_tokens[line][i].getValue())
                            break;
                    }

                    bool boolResult;

                    if (!evaluate(
                        m_tokens[line].begin() + begin,
                        m_tokens[line].begin() + i,
                        boolResult, "$", OPERAND_TYPE_STRING))
                    {
                        return SIZE_MAX;
                    }

                    std::cout << m_strVars["$"] << " ";

                    begin = i + 1;
                }
                std::cout << std::endl;
                break;

            case Token::KEYWORD_INPUT:
                if (begin + 1 < end)
                {
                    std::size_t id = begin + 1;

                    while (id < end)
                    {
                        if (Token::LITERAL_STRING == m_tokens[line][id].getValue())
                        {
                            std::cout << m_tokens[line][id].getString() << " ";
                        }
                        else if (Token::TYPE_IDENTIFIER == m_tokens[line][id].getType())
                        {
                            switch (m_tokens[line][id].getValue())
                            {
                            case Token::IDENTIFIER_REAL:
                                std::cin >> m_realVars[
                                    m_tokens[line][id].getIdentifier()];
                                break;

                            case Token::IDENTIFIER_INTEGER:
                                std::cin >> m_intVars[
                                    m_tokens[line][id].getIdentifier()];
                                break;

                            case Token::IDENTIFIER_STRING:
                                if (id + 1 < end)
                                {
                                    std::cin >> m_strVars[
                                        m_tokens[line][id].getIdentifier()];
                                }
                                else
                                {
                                    std::getline(std::cin, m_strVars[
                                        m_tokens[line][id].getIdentifier()]);
                                }
                                break;
                            }
                            if (std::cin.fail())
                            {
                                std::string t;
                                std::cin.clear();
                                std::cin >> t;
                                std::cerr << "[ " << t
                                    << " ] inappropriate input value!\n";
                            }
                        }
                        else
                        {
                            std::cerr << "Unsuitable INPUT parameter!\n";
                            return SIZE_MAX;
                        }

                        id++;

                        if ((id < end) && (Token::PUNCTUATION_MARK_COMMA ==
                            m_tokens[line][id].getValue()))
                        {
                            if (++id < end)
                                continue;

                            std::cerr << "Extra comma on line!\n";
                            return SIZE_MAX;
                        }
                    }

                    if (1 != id - begin)
                        break;
                }
                std::cerr << "Incomplete INPUT statement!\n";
                return SIZE_MAX;

            case Token::KEYWORD_GOTO:
            case Token::KEYWORD_GOSUB:
                if (2 != end - begin)
                {
                    std::cerr << "Bad jump!\n";
                    return SIZE_MAX;
                }
                else
                {
                    std::stringstream key;
                    switch (m_tokens[line][begin + 1].getValue())
                    {
                    case Token::LITERAL_INTEGER:
                        key << m_tokens[line][begin + 1].getInteger();
                        break;

                    case Token::IDENTIFIER_LABEL:
                        key << m_tokens[line][begin + 1].getIdentifier();
                        break;

                    default:
                        std::cerr << "Bad jump label!\n";
                        return SIZE_MAX;
                    }

                    auto label = m_labels.find(key.str());

                    if (m_labels.end() == label)
                    {
                        std::cerr << "Jump label \'" << key.str()
                                  << "\' does not exist!\n";
                        return SIZE_MAX;
                    }

                    if (Token::KEYWORD_GOSUB == m_tokens[line][begin].getValue())
                    {
                        if (0xFFFF < m_callStack.size())
                        {
                            std::cerr << "Call stack overflow!\n";
                            return SIZE_MAX;
                        }
                        m_callStack.push(line + 1);
                    }

                    return label->second;
                }

            case Token::KEYWORD_RETURN:
                if (!m_callStack.empty())
                {
                    auto result = m_callStack.top();
                    m_callStack.pop();
                    return result;
                }
                std::cerr << "Call stack is empty!\n";
                return SIZE_MAX;

            case Token::KEYWORD_END:
                return m_tokens.size();

            case Token::KEYWORD_STOP:
                return SIZE_MAX;

            case Token::KEYWORD_IF:
                {
                    std::size_t beginCond = begin + 1;
                    std::size_t endCond = beginCond;
                    while (true)
                    {
                        if (endCond >= end)
                        {
                            std::cerr << "Incomplete IF statement!\n";
                            return SIZE_MAX;
                        }

                        Token::Value value =
                            m_tokens[line][endCond].getValue();
                        
                        if ((Token::KEYWORD_THEN  == value) ||
                            (Token::KEYWORD_GOTO  == value) ||
                            (Token::KEYWORD_GOSUB == value))
                            break;

                        endCond++;
                    }

                    std::stack<std::size_t> elseStack;
                    for (std::size_t i = end - 1; i > begin; i--)
                    {
                        if (Token::KEYWORD_ELSE == m_tokens[line][i].getValue())
                            elseStack.push(i);
                        else if (Token::KEYWORD_IF == m_tokens[line][i].getValue())
                            if (!elseStack.empty())
                                elseStack.pop();
                    }

                    if (1 < elseStack.size())
                    {
                        std::cerr << "Extra ELSE statement on line!\n";
                        return SIZE_MAX;
                    }

                    bool boolResult;

                    if (!evaluate(
                        m_tokens[line].begin() + beginCond,
                        m_tokens[line].begin() + endCond,
                        boolResult))
                    {
                        return SIZE_MAX;
                    }
                    
                    auto beginThen = Token::KEYWORD_THEN ==
                        m_tokens[line][endCond].getValue()
                            ? endCond + 1 : endCond;

                    if (boolResult)
                        return execute(line, beginThen, 
                            elseStack.empty() ? end : elseStack.top());
                    else
                        if (!elseStack.empty())
                            return execute(line, elseStack.top() + 1, end);
                }
                break;

            default:
                std::cerr << "Improper keyword placement!\n";
                return SIZE_MAX;
            }
            break;

        default:
            std::cerr << "Bad statement!\n";
            return SIZE_MAX;
        }
    }

    return line + 1;
}


bool Interpreter::evaluate(
    VectorOfTokens::iterator& begin,
    VectorOfTokens::iterator  end,
    bool&                     boolResult,
    const std::string&        varResult,
    const OperandType         varType)
{
    static const char TYPE_MISMATCH[] = "Type mismatch!\n";
    static const char DIVISION_BY_ZERO[] = "Division by zero!\n";
    static const char BAD_EXPRESSION[]="Bad expression!\n";
    static const char UNEXPECTED_PUNCTUATION_MARK[] =
        "Unexpected punctuation mark!\n";

    struct Operation
    {
        Token::Value tokenValue;
        std::size_t  totalOperands;
        int          priority;
    };

    std::stack<Operation>    operations;
    std::stack<OperandType>  types;
    std::stack<long double>  reals;
    std::stack<std::int64_t> integers;
    std::stack<std::string>  strings;
    std::stack<bool>         booleans;

    auto performTopmostOperation = [&]() -> bool
        {
            auto operation = operations.top();
            operations.pop();

            if (types.size() < operation.totalOperands)
            {
                std::cerr << "Too few operands in expression!\n";
                return false;
            }

            if ((operation.tokenValue & Token::TYPE_MASK) ==
                Token::TYPE_FUNCTION)
            {
                assert(1 == operation.totalOperands);

                if (::Interpreter::OPERAND_TYPE_STRING == types.top())
                {
                    switch (operation.tokenValue)
                    {
                    case Token::FUNCTION_SHELL:
                        types.top() = ::Interpreter::OPERAND_TYPE_INTEGER;
                        integers.push(std::system(strings.top().c_str()));
                        strings.pop();
                        return true;

                    case Token::FUNCTION_VAL:
                        types.top() = ::Interpreter::OPERAND_TYPE_REAL;
                        reals.push(std::stold(strings.top()));
                        strings.pop();
                        return true;
                    }
                }

                if (::Interpreter::OPERAND_TYPE_INTEGER == types.top())
                {
                    switch (operation.tokenValue)
                    {
                    case Token::FUNCTION_ABS:
                        integers.top() = std::abs(integers.top());
                        return true;

                    case Token::FUNCTION_RND:
                        integers.top() = std::rand() % (1 + integers.top());
                        return true;

                    case Token::FUNCTION_SGN:
                        integers.top() = (0 == integers.top())
                            ? 0 : integers.top() / std::abs(integers.top());
                        return true;
                    }

                    types.top() = ::Interpreter::OPERAND_TYPE_REAL;
                    reals.push(static_cast<long double>(integers.top()));
                    integers.pop();
                }

                if (::Interpreter::OPERAND_TYPE_REAL == types.top())
                {
                    switch (operation.tokenValue)
                    {
                    case Token::FUNCTION_ABS:
                        reals.top() = std::abs(reals.top());
                        return true;

                    case Token::FUNCTION_ATN:
                        reals.top() = std::atan(reals.top());
                        return true;

                    case Token::FUNCTION_COS:
                        reals.top() = std::cos(reals.top());
                        return true;

                    case Token::FUNCTION_FIX:
                        reals.top() = std::floor(reals.top());
                        return true;

                    case Token::FUNCTION_LOG:
                        reals.top() = std::log(reals.top());
                        return true;

                    case Token::FUNCTION_RND:
                        reals.top() = (static_cast<long double>(std::rand()) /
                            RAND_MAX) * reals.top();
                        return true;

                    case Token::FUNCTION_SGN:
                        reals.top() = (0 == reals.top())
                            ? 0 : reals.top() / std::abs(reals.top());
                        return true;

                    case Token::FUNCTION_SIN:
                        reals.top() = std::sin(reals.top());
                        return true;

                    case Token::FUNCTION_SQR:
                        reals.top() = std::sqrt(reals.top());
                        return true;

                    case Token::FUNCTION_TAN:
                        reals.top() = std::tan(reals.top());
                        return true;

                    case Token::FUNCTION_EXP:
                        {
                            int exp;
                            std::frexp(reals.top(), &exp);
                            reals.pop();
                            types.top() = ::Interpreter::OPERAND_TYPE_INTEGER;
                            integers.push(exp);
                        }
                        return true;

                    case Token::FUNCTION_INT:
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_INTEGER;
                            integers.push(static_cast<std::int64_t>(
                                std::floor(reals.top())));
                            reals.pop();
                        }
                        return true;
                    }
                }
            }
            else
            {
                assert((operation.tokenValue & Token::TYPE_MASK) ==
                    Token::TYPE_OPERATOR);

                switch (operation.totalOperands)
                {
                case 1:
                    switch (operation.tokenValue)
                    {
                    case Token::OPERATOR_ADD:
                        if ((::Interpreter::OPERAND_TYPE_REAL    != types.top()) &&
                            (::Interpreter::OPERAND_TYPE_INTEGER != types.top()))
                        {
                            std::cerr << TYPE_MISMATCH;
                            return false;
                        }
                        return true;

                    case Token::OPERATOR_SUBTRACT:
                        switch (types.top())
                        {
                        case ::Interpreter::OPERAND_TYPE_REAL:
                            reals.top() = -reals.top();
                            return true;

                        case ::Interpreter::OPERAND_TYPE_INTEGER:
                            integers.top() = -integers.top();
                            return true;
                        }
                        std::cerr << TYPE_MISMATCH;
                        return false;

                    case Token::OPERATOR_NOT:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN == types.top())
                        {
                            booleans.top() = !booleans.top();
                            return true;
                        }
                        std::cerr << TYPE_MISMATCH;
                        return false;
                    }
                    break;

                case 2:
                    auto typeOfB = types.top();
                    types.pop();

                    auto typeOfA = types.top();

                    ::Interpreter::OperandType operandsType =
                        ::Interpreter::OPERAND_TYPE_REAL;

                    if ((::Interpreter::OPERAND_TYPE_BOOLEAN == typeOfA) ||
                        (::Interpreter::OPERAND_TYPE_BOOLEAN == typeOfB))
                    {
                        if (typeOfA != typeOfB)
                        {
                            std::cerr << TYPE_MISMATCH;
                            return false;
                        }
                        operandsType = ::Interpreter::OPERAND_TYPE_BOOLEAN;
                    }
                    else if ((::Interpreter::OPERAND_TYPE_INTEGER == typeOfA) &&
                             (::Interpreter::OPERAND_TYPE_INTEGER == typeOfB))
                    {
                        operandsType = ::Interpreter::OPERAND_TYPE_INTEGER;
                    }
                    else if ((::Interpreter::OPERAND_TYPE_STRING == typeOfA) ||
                             (::Interpreter::OPERAND_TYPE_STRING == typeOfB))
                    {
                        if (typeOfA != typeOfB)
                        {
                            bool nonStrA =
                                ::Interpreter::OPERAND_TYPE_STRING != typeOfA;

                            auto type = (nonStrA ? typeOfA : typeOfB);

                            std::stringstream result;

                            switch (type)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                result << reals.top();
                                reals.pop();
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                result << integers.top();
                                integers.pop();
                                break;

                            case ::Interpreter::OPERAND_TYPE_BOOLEAN:
                                std::cerr << TYPE_MISMATCH;
                                return false;
                            }

                            if (nonStrA)
                                std::swap(strings.top(), result.str());

                            strings.push(result.str());
                        }
                        operandsType = ::Interpreter::OPERAND_TYPE_STRING;
                    }
                    else
                    {
                        if (typeOfA != typeOfB)
                        {
                            bool nonRealA =
                                ::Interpreter::OPERAND_TYPE_REAL != typeOfA;

                            long double result = static_cast<long double>(
                                integers.top());

                            integers.pop();

                            if (nonRealA)
                                std::swap(reals.top(), result);

                            reals.push(result);
                        }
                    }

                    types.top() = operandsType;

                    switch (operation.tokenValue)
                    {
                    case Token::OPERATOR_ADD:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto t = reals.top();
                                    reals.pop();
                                    reals.top() += t;
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto t = integers.top();
                                    integers.pop();
                                    integers.top() += t;
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto t = std::move(strings.top());
                                    strings.pop();
                                    strings.top().append(t);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_AND:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN == operandsType)
                        {
                            auto t = booleans.top();
                            booleans.pop();
                            booleans.top() = booleans.top() && t;
                            return true;
                        }
                        break;

                    case Token::OPERATOR_DIVIDE:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto t = reals.top();
                                    reals.pop();

                                    if (0 == t)
                                    {
                                        std::cerr << DIVISION_BY_ZERO;
                                        return false;
                                    }

                                    reals.top() /= t;
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    types.top() = ::Interpreter::OPERAND_TYPE_REAL;

                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    if (0 == b)
                                    {
                                        std::cerr << DIVISION_BY_ZERO;
                                        return false;
                                    }

                                    reals.push(static_cast<long double>(a) / b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_EQUAL:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a == b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a == b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a == b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_GREATER:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a > b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a > b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a > b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_GREATER_OR_EQUAL:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a >= b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a >= b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a >= b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_INEQUAL:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a != b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a != b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a != b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_INTEGER_DIVIDE:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    types.top() = ::Interpreter::OPERAND_TYPE_INTEGER;

                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    if (0 == b)
                                    {
                                        std::cerr << DIVISION_BY_ZERO;
                                        return false;
                                    }

                                    integers.push(
                                        static_cast<std::int64_t>(a) /
                                        static_cast<std::int64_t>(b));
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto t = integers.top();
                                    integers.pop();

                                    if (0 == t)
                                    {
                                        std::cerr << DIVISION_BY_ZERO;
                                        return false;
                                    }

                                    integers.top() /= t;
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_LESS:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a < b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a < b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a < b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_LESS_OR_EQUAL:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType)
                        {
                            types.top() = ::Interpreter::OPERAND_TYPE_BOOLEAN;

                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto b = reals.top();
                                    reals.pop();
                                    auto a = reals.top();
                                    reals.pop();

                                    booleans.push(a <= b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto b = integers.top();
                                    integers.pop();
                                    auto a = integers.top();
                                    integers.pop();

                                    booleans.push(a <= b);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_STRING:
                                {
                                    auto b = strings.top();
                                    strings.pop();
                                    auto a = strings.top();
                                    strings.pop();

                                    booleans.push(a <= b);
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_MODULO:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            std::int64_t a;
                            std::int64_t b;
                            if (operandsType == ::Interpreter::OPERAND_TYPE_REAL)
                            {
                                b = static_cast<std::int64_t>(reals.top());
                                reals.pop();
                                a = static_cast<std::int64_t>(reals.top());
                                reals.pop();
                            }
                            else
                            {
                                b = integers.top();
                                integers.pop();
                                a = integers.top();
                                integers.pop();
                            }
                            integers.push(a % b);
                            return true;
                        }
                        break;

                    case Token::OPERATOR_MULTIPLY:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto t = reals.top();
                                    reals.pop();
                                    reals.top() *= t;
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto t = integers.top();
                                    integers.pop();
                                    integers.top() *= t;
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_OR:
                        if (::Interpreter::OPERAND_TYPE_BOOLEAN == operandsType)
                        {
                            auto t = booleans.top();
                            booleans.pop();
                            booleans.top() = booleans.top() || t;
                            return true;
                        }
                        break;

                    case Token::OPERATOR_POWER:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto t = reals.top();
                                    reals.pop();
                                    reals.top() = std::pow(reals.top(), t);
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto t = integers.top();
                                    integers.pop();
                                    integers.top() = static_cast<std::int64_t>(
                                        std::pow(
                                            static_cast<long double>(
                                                integers.top()),
                                            static_cast<int>(t)));
                                }
                                break;
                            }
                            return true;
                        }
                        break;

                    case Token::OPERATOR_SUBTRACT:
                        if ((::Interpreter::OPERAND_TYPE_BOOLEAN != operandsType) &&
                            (::Interpreter::OPERAND_TYPE_STRING  != operandsType))
                        {
                            switch (operandsType)
                            {
                            case ::Interpreter::OPERAND_TYPE_REAL:
                                {
                                    auto t = reals.top();
                                    reals.pop();
                                    reals.top() -= t;
                                }
                                break;

                            case ::Interpreter::OPERAND_TYPE_INTEGER:
                                {
                                    auto t = integers.top();
                                    integers.pop();
                                    integers.top() -= t;
                                }
                                break;
                            }
                            return true;
                        }
                        break;
                    }
                }
            }

            std::cerr << TYPE_MISMATCH;
            return false;
        };

    enum State
    {
        STATE_A,
        STATE_B
    };

    const int PRIORITY_STEP = 10;

    int priorityBase = 0;

    State state = STATE_A;

    for (; begin != end; begin++)
    {
        switch (state)
        {
        case STATE_A:
            if ((Token::TYPE_IDENTIFIER == begin->getType()) ||
                (Token::TYPE_LITERAL    == begin->getType()))
            {
                bool isLiteral = (Token::TYPE_LITERAL == begin->getType());

                switch (begin->getValue())
                {
                case Token::IDENTIFIER_REAL:
                case Token::LITERAL_REAL:
                    types.push(OPERAND_TYPE_REAL);
                    reals.push(isLiteral 
                        ? begin->getReal()
                        : m_realVars[begin->getIdentifier()]);
                    break;

                case Token::IDENTIFIER_INTEGER:
                case Token::LITERAL_INTEGER:
                    types.push(OPERAND_TYPE_INTEGER);
                    integers.push(isLiteral 
                        ? begin->getInteger()
                        : m_intVars[begin->getIdentifier()]);
                    break;

                case Token::IDENTIFIER_STRING:
                case Token::LITERAL_STRING:
                    types.push(OPERAND_TYPE_STRING);
                    strings.push(isLiteral 
                        ? begin->getString()
                        : m_strVars[begin->getIdentifier()]);
                    break;
                }

                state = STATE_B;
            }
            else if (Token::TYPE_OPERATOR == begin->getType())
            {
                if ((Token::OPERATOR_ADD      == begin->getValue()) ||
                    (Token::OPERATOR_SUBTRACT == begin->getValue()) ||
                    (Token::OPERATOR_NOT      == begin->getValue()))
                {
                    Operation operation;
                    operation.tokenValue    = begin->getValue();
                    operation.totalOperands = 1;
                    operation.priority = priorityBase +
                        (Token::OPERATOR_NOT == operation.tokenValue)
                        ? 2 : 8;
                    operations.push(operation);
                }
                else
                {
                    std::cerr << "Unexpected operator!\n";
                    return false;
                }
            }
            else if (Token::TYPE_FUNCTION == begin->getType())
            {
                Operation operation;
                operation.tokenValue    = begin->getValue();
                operation.totalOperands = 1;
                operation.priority      = priorityBase + 6;
                operations.push(operation);
            }
            else if (Token::TYPE_PUNCTUATION_MARK == begin->getType())
            {
                switch (begin->getValue())
                {
                case Token::PUNCTUATION_MARK_PARENTHESIS_LEFT:
                    priorityBase += PRIORITY_STEP;
                    break;

                case Token::PUNCTUATION_MARK_PARENTHESIS_RIGHT:
                    std::cerr << "Unexpected closing parenthesis!\n";
                    return false;

                default:
                    std::cerr << UNEXPECTED_PUNCTUATION_MARK;
                    return false;
                }
            }
            else
            {
                std::cerr << BAD_EXPRESSION;
                return false;
            }
            break;

        case STATE_B:
            if (Token::TYPE_OPERATOR == begin->getType())
            {
                Operation operation;
                operation.tokenValue    = begin->getValue();
                operation.totalOperands = 2;

                switch (operation.tokenValue)
                {
                case Token::OPERATOR_AND:
                case Token::OPERATOR_OR:
                    operation.priority = priorityBase + 1;
                    break;

                case Token::OPERATOR_EQUAL:
                case Token::OPERATOR_GREATER:
                case Token::OPERATOR_GREATER_OR_EQUAL:
                case Token::OPERATOR_INEQUAL:
                case Token::OPERATOR_LESS:
                case Token::OPERATOR_LESS_OR_EQUAL:
                    operation.priority = priorityBase + 3;
                    break;

                case Token::OPERATOR_ADD:        
                case Token::OPERATOR_SUBTRACT:
                    operation.priority = priorityBase + 4;
                    break;

                case Token::OPERATOR_DIVIDE:
                case Token::OPERATOR_INTEGER_DIVIDE:
                case Token::OPERATOR_MODULO:
                case Token::OPERATOR_MULTIPLY:
                    operation.priority = priorityBase + 5;
                    break;

                case Token::OPERATOR_POWER:
                    operation.priority = priorityBase + 7;
                    break;
                }

                while ((!operations.empty()) && 
                    (operations.top().priority >= operation.priority))
                {
                    if (!performTopmostOperation())
                        return false;
                }

                operations.push(operation);
                state = STATE_A;
            }
            else if (Token::TYPE_PUNCTUATION_MARK == begin->getType())
            {
                switch (begin->getValue())
                {
                case Token::PUNCTUATION_MARK_PARENTHESIS_LEFT:
                    std::cerr << "Unexpected opening parenthesis!\n";
                    return false;

                case Token::PUNCTUATION_MARK_PARENTHESIS_RIGHT:
                    priorityBase -= PRIORITY_STEP;
                    if (priorityBase < 0)
                    {
                        std::cerr << "Unmatched closing parenthesis!\n";
                        return false;
                    }
                    break;

                default:
                    std::cerr << UNEXPECTED_PUNCTUATION_MARK;
                    return false;
                }
            }            
            else
            {
                std::cerr << BAD_EXPRESSION;
                return false;
            }
            break;
        };
    }

    if (0 != priorityBase)
    {
        std::cerr << "Unmatched opening parenthesis!\n";
        return false;
    }

    while (!operations.empty())
        if (!performTopmostOperation())
            return false;

    if (1 != types.size())
    {
        std::cerr << BAD_EXPRESSION;
        return false;
    }

    if (varType != types.top())
    {
        if ((OPERAND_TYPE_REAL    == types.top()) &&
            (OPERAND_TYPE_INTEGER == varType))
        {
            integers.push(static_cast<std::int64_t>(reals.top()));
        }
        else if ((OPERAND_TYPE_INTEGER == types.top()) &&
                 (OPERAND_TYPE_REAL    == varType))
        {
            reals.push(static_cast<long double>(integers.top()));
        }
        else if (OPERAND_TYPE_STRING == varType)
        {
            std::stringstream result;

            switch (types.top())
            {
            case OPERAND_TYPE_REAL:
                result << reals.top();
                break;

            case OPERAND_TYPE_INTEGER:
                result << integers.top();
                break;

            default:
                std::cerr << TYPE_MISMATCH;
                return false;
            }

            strings.push(result.str());
        }
        else
        {
            std::cerr << TYPE_MISMATCH;
            return false;
        }

        types.top() = varType;
    }

    switch (varType)
    {
    case OPERAND_TYPE_REAL:
        assert(!reals.empty());
        m_realVars[varResult] = reals.top();
        break;

    case OPERAND_TYPE_INTEGER:
        assert(!integers.empty());
        m_intVars[varResult] = integers.top();
        break;

    case OPERAND_TYPE_STRING:
        assert(!strings.empty());
        m_strVars[varResult] = strings.top();
        break;

    case OPERAND_TYPE_BOOLEAN:
        assert(!booleans.empty());
        boolResult = booleans.top();
        break;
    }

    return true;
}

#ifndef TOKEN_HPP_INCLUDED
#define TOKEN_HPP_INCLUDED

#include <cassert>
#include <cctype>
#include <cstdint>
#include <algorithm>
#include <string>

class Token
{
public:

    enum Value
    {
        INVALID   = 0,
        HAPPY_END = 0xF000,
        TYPE_MASK = 0xF000,

        TYPE_FUNCTION = 0x1000,
        FUNCTION_ABS,
        FUNCTION_ATN,
        FUNCTION_COS,
        FUNCTION_EXP,
        FUNCTION_FIX,
        FUNCTION_INT,
        FUNCTION_LOG,
        FUNCTION_RND,
        FUNCTION_SGN,
        FUNCTION_SHELL,
        FUNCTION_SIN,
        FUNCTION_SQR,
        FUNCTION_TAN,
        FUNCTION_VAL,

        TYPE_IDENTIFIER = 0x2000,
        IDENTIFIER_REAL,
        IDENTIFIER_LABEL = IDENTIFIER_REAL,
        IDENTIFIER_INTEGER,
        IDENTIFIER_STRING,

        TYPE_KEYWORD = 0x3000,
        KEYWORD_ELSE,
        KEYWORD_END,
        KEYWORD_FOR,
        KEYWORD_GOSUB,
        KEYWORD_GOTO,
        KEYWORD_IF,
        KEYWORD_INPUT,
        KEYWORD_LET,
        KEYWORD_NEXT,
        KEYWORD_PRINT,
        KEYWORD_REM,
        KEYWORD_RETURN,
        KEYWORD_STEP,
        KEYWORD_STOP,
        KEYWORD_THEN,
        KEYWORD_TO,

        TYPE_LITERAL = 0x4000,
        LITERAL_REAL,
        LITERAL_INTEGER,
        LITERAL_STRING,

        TYPE_OPERATOR = 0x5000,
        OPERATOR_ADD,
        OPERATOR_AND,
        OPERATOR_DIVIDE,
        OPERATOR_EQUAL,
        OPERATOR_GREATER,
        OPERATOR_GREATER_OR_EQUAL,
        OPERATOR_INEQUAL,
        OPERATOR_INTEGER_DIVIDE,
        OPERATOR_NOT,
        OPERATOR_LESS,
        OPERATOR_LESS_OR_EQUAL,
        OPERATOR_MODULO,
        OPERATOR_MULTIPLY,
        OPERATOR_OR,
        OPERATOR_POWER,
        OPERATOR_SUBTRACT,

        TYPE_PUNCTUATION_MARK = 0x6000,
        PUNCTUATION_MARK_COLON,
        PUNCTUATION_MARK_COMMA,
        PUNCTUATION_MARK_PARENTHESIS_LEFT,
        PUNCTUATION_MARK_PARENTHESIS_RIGHT,
        PUNCTUATION_MARK_SEMICOLON
    };

    Token()
    {
        ::memset(this, 0, sizeof(Token));
    }

    Value getType() const
    {
        return static_cast<Value>(TYPE_MASK & m_value);
    }

    Value getValue() const
    {
        return m_value;
    }

    long double getReal() const
    {
        assert(LITERAL_REAL == m_value);
        return m_real;
    }

    std::int64_t getInteger() const
    {
        assert(LITERAL_INTEGER == m_value);
        return m_integer;
    }

    std::string getString() const
    {
        assert((INVALID != m_value) && (m_string.begin < m_string.end));
        return std::string(m_string.begin, m_string.end);
    }

    std::string getIdentifier() const
    {
        assert(TYPE_IDENTIFIER == (TYPE_MASK & m_value));
        std::string id(getString());
        std::transform(id.begin(), id.end(), id.begin(), std::toupper);
        return id;
    }

    const char* parse(const char* begin, const char* end);

private:

    struct String
    {
        const char* begin;
        const char* end;
    };

    union
    {
        long double  m_real;
        std::int64_t m_integer;
        String       m_string;
    };

    Value m_value;
};

#endif // TOKEN_HPP_INCLUDED

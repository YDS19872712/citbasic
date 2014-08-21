#include "Token.hpp"

const char* Token::parse(const char* begin, const char* end)
{
    struct Entry
    {
        const char* id;
        const Value value;
    };

    static const Entry table[] =
    {
        { "ABS"    , FUNCTION_ABS    },
        { "AND"    , OPERATOR_AND    },
        { "ATN"    , FUNCTION_ATN    },
        { "COS"    , FUNCTION_COS    },
        { "ELSE"   , KEYWORD_ELSE    },
        { "END"    , KEYWORD_END     },
        { "EXP"    , FUNCTION_EXP    },
        { "FIX"    , FUNCTION_FIX    },
        { "FOR"    , KEYWORD_FOR     },
        { "GOSUB"  , KEYWORD_GOSUB   },
        { "GOTO"   , KEYWORD_GOTO    },
        { "IF"     , KEYWORD_IF      },
        { "INPUT"  , KEYWORD_INPUT   },
        { "INT"    , FUNCTION_INT    },
        { "LET"    , KEYWORD_LET     },
        { "LOG"    , FUNCTION_LOG    },
        { "MOD"    , OPERATOR_MODULO },
        { "NEXT"   , KEYWORD_NEXT    },
        { "NOT"    , OPERATOR_NOT    },
        { "OR"     , OPERATOR_OR     },
        { "PRINT"  , KEYWORD_PRINT   },
        { "REM"    , KEYWORD_REM     },
        { "RETURN" , KEYWORD_RETURN  },
        { "RND"    , FUNCTION_RND    },
        { "SGN"    , FUNCTION_SGN    },
        { "SHELL"  , FUNCTION_SHELL  },
        { "SIN"    , FUNCTION_SIN    },
        { "SQR"    , FUNCTION_SQR    },
        { "STEP"   , KEYWORD_STEP    },
        { "STOP"   , KEYWORD_STOP    },
        { "TAN"    , FUNCTION_TAN    },
        { "THEN"   , KEYWORD_THEN    },
        { "TO"     , KEYWORD_TO      },
        { "VAL"    , FUNCTION_VAL    }
    };

    enum State
    {
        STATE_INITIAL,
        STATE_STRING,
        STATE_INTEGER,
        STATE_REAL,
        STATE_ID
    };

    State state = STATE_INITIAL;

    for (const char* current = begin;
            STATE_INITIAL == state ? current < end : current <= end;
            current++)
    {
        const char* next = current + 1;

        const char currentChar = current < end ? *current : 0;
        const char nextChar = next < end ? *next : 0;

        switch (state)
        {
        case STATE_INITIAL:
            switch (currentChar)
            {
            case ' ':
            case '\t':
                break;

            case ':':
                m_value = PUNCTUATION_MARK_COLON;
                return next;

            case ',':
                m_value = PUNCTUATION_MARK_COMMA;
                return next;

            case '(':
                m_value = PUNCTUATION_MARK_PARENTHESIS_LEFT;
                return next;

            case ')':
                m_value = PUNCTUATION_MARK_PARENTHESIS_RIGHT;
                return next;

            case ';':
                m_value = PUNCTUATION_MARK_SEMICOLON;
                return next;

            case '+':
                m_value = OPERATOR_ADD;
                return next;

            case '/':
                m_value = OPERATOR_DIVIDE;
                return next;

            case '=':
                m_value = OPERATOR_EQUAL;
                return next;

            case '\\':
                m_value = OPERATOR_INTEGER_DIVIDE;
                return next;

            case '*':
                m_value = OPERATOR_MULTIPLY;
                return next;

            case '^':
                m_value = OPERATOR_POWER;
                return next;

            case '-':
                m_value = OPERATOR_SUBTRACT;
                return next;

            case '>':
                if ('=' == nextChar)
                {
                    m_value = OPERATOR_GREATER_OR_EQUAL;
                    return next + 1;
                }

                m_value = OPERATOR_GREATER;
                return next;

            case '<':
                switch (nextChar)
                {
                case '>':
                    m_value = OPERATOR_INEQUAL;
                    return next + 1;

                case '=':
                    m_value = OPERATOR_LESS_OR_EQUAL;
                    return next + 1;
                }

                m_value = OPERATOR_LESS;
                return next;

            case '?':
                m_value = KEYWORD_PRINT;
                return next;

            case '\'':
                m_value = KEYWORD_REM;
                return next;

            case '\"':
                m_string.begin = next;
                state = STATE_STRING;
                break;

            case '_':
                m_string.begin = current;
                state = STATE_ID;
                break;

            case '.':
                m_string.begin = current;
                state = STATE_REAL;
                break;

            default:
                if (std::isalpha(currentChar))
                {
                    m_string.begin = current;
                    state = STATE_ID;
                    break;
                }

                if (std::isdigit(currentChar))
                {
                    m_string.begin = current;
                    state = STATE_INTEGER;
                    break;
                }

                m_value = INVALID;
                return begin + 1;
            }
            break;

        case STATE_STRING:
            if (std::iscntrl(currentChar))
            {
                m_value = INVALID;
                return begin + 1;
            }

            if ('\"' == currentChar)
            {
                m_string.end = current;
                m_value = LITERAL_STRING;
                return next;
            }
            break;

        case STATE_ID:
            if (!std::isalnum(currentChar) && ('_' != currentChar))
            {
                m_string.end = current;

                const size_t COUNT_TABLE = sizeof(table) / sizeof(table[0]);

                bool entryFound = false;

                auto entry = std::lower_bound(
                    &table[0],
                    &table[COUNT_TABLE],
                    m_string,
                    [&entryFound](const Entry& entry, const String& id) -> bool
                    {
                        size_t sizeEntry =
                            static_cast<size_t>(std::strlen(entry.id));
                        size_t sizeId = static_cast<size_t>(id.end - id.begin);
                        size_t size = std::min(sizeEntry, sizeId);

                        auto result = ::_memicmp(entry.id, id.begin, size);

                        if (0 == result)
                        {
                            if (sizeEntry == sizeId)
                                entryFound = true;
                            else if (sizeEntry < sizeId)
                                return true;
                        }

                        return 0 > result;
                    });

                if (entryFound)
                {
                    m_value = entry->value;
                    return current;
                }

                switch (currentChar)
                {
                case '!':
                case '#':
                case ' ':
                case '\t':
                    m_value = IDENTIFIER_REAL;
                    break;

                case '%':
                    m_value = IDENTIFIER_INTEGER;
                    break;

                case '$':
                    m_value = IDENTIFIER_STRING;
                    break;

                default:
                    m_value = IDENTIFIER_REAL;
                    return current;
                }

                return next;
            }
            break;

        case STATE_INTEGER:
            if (!std::isdigit(currentChar))
            {
                if ('.' == currentChar)
                {
                    state = STATE_REAL;
                    break;
                }

                m_value = LITERAL_INTEGER;
                m_string.end = current;
                m_integer = std::stoll(getString());
                return current;
            }
            break;

        case STATE_REAL:
            if (!std::isdigit(currentChar))
            {
                m_value = LITERAL_REAL;
                m_string.end = current;
                m_real = std::stold(getString());
                return current;
            }
            break;
        }
    }

    assert(STATE_INITIAL == state);
    m_value = HAPPY_END;
    return end;
}

//  Oak programming language
//  Copyright (C) 2019  Nikita Dubovikov
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
#include "Tokenizer.h"
#include "ErrorChecking.h"

bool EofReturned = false;

wchar_t Tokenizer::GetChar()
{
    if (m_code[m_offset] == '\n')
    {
        ++line;
        col = 1;
    }
    else
    {
        ++col;
    }

    return m_code[m_offset++];
}

void Tokenizer::EatChars(size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        GetChar();
    }
}

void Tokenizer::SkipWhite()
{
    wchar_t c = m_code[m_offset];
    while (iswspace(c) || iswblank(c) || c == L'\t' || c == 0)
    {
        GetChar();
        if (m_offset == m_code.size()) break;
        c = m_code[m_offset];
    }
}

// check if given number token is valid, i.e. specified number of bits is enough
bool Tokenizer::IsNumValid(const Token& t)
{
    switch (t.type)
    {
    case TokenType::Int8L:
        return IntToString(StringToInt<int8_t>(t.data)) == t.data;
    case TokenType::Int16L:
        return IntToString(StringToInt<int16_t>(t.data)) == t.data;
    case TokenType::Int32L:
        return IntToString(StringToInt<int32_t>(t.data)) == t.data;
    case TokenType::Int64L:
        return IntToString(StringToInt<int64_t>(t.data)) == t.data;

    case TokenType::Uint8L:
        return IntToString(StringToInt<uint8_t>(t.data)) == t.data;
    case TokenType::Uint16L:
        return IntToString(StringToInt<uint16_t>(t.data)) == t.data;
    case TokenType::Uint32L:
        return IntToString(StringToInt<uint32_t>(t.data)) == t.data;
    case TokenType::Uint64L:
        return IntToString(StringToInt<uint64_t>(t.data)) == t.data;

    default:
        return false;
    }
}

Keyword Tokenizer::IsKeyword(const String& str)
{
    for (int i = 0; i < (int)Keyword::Last; ++i)
    {
        if (str == KeywordStr[i])
        {
            return (Keyword)i;
        }
    }

    return Keyword::Last;
}

Tokenizer::Tokenizer(const std::vector<String>& str, bool path)
{
    EofReturned = false;
    m_code = L"";
    if (path)
    {
        for (const String& s : str)
        {
            if (!ReadFile(s, m_code))
            {
                throw Error(L"file " + s + L" not found\n");
            }
        }
    }
    else
    {
        for (const String& s : str)
        {
            m_code += s;
        }
    }
    m_code += L" \n \n "; // adding a bit of garbage to prevent overflow
    m_offset = 0;
}

void Tokenizer::UnexpToken(const String& msg)
{
    uint64_t n_col;
    String r = GetLine(n_col);
    throw UnexpectedToken(prevLine, n_col, msg, r);
}

Token Tokenizer::ParseName()
{
    String r = L"";

    wchar_t c = m_code[m_offset];

    while (IsAlpha(c) || IsNumber(c) || c == L'_')
    {
        r += c;
        GetChar();
        c = m_code[m_offset];
    }

    SkipWhite();

    Keyword kw = IsKeyword(r);

    return Token(r, TokenType::Name, kw);
}

String Tokenizer::ParseNumber()
{
    String r = L"";

    wchar_t c = m_code[m_offset];

    while (IsNumber(c) || c == L'_')
    {
        if (c != L'_') r += c;
        GetChar();
        c = m_code[m_offset];
    }

    SkipWhite();

    return r;
}

Token Tokenizer::ParseNumberLiteral()
{
    String r = ParseNumber();

    if (m_code[m_offset] == L'i')
    {
        GetChar();
        String bits = ParseNumber();
        if (bits == L"8")
        {
            return Token(r, TokenType::Int8L);
        }
        if (bits == L"16")
        {
            return Token(r, TokenType::Int16L);
        }
        if (bits == L"32")
        {
            return Token(r, TokenType::Int32L);
        }
        if (bits == L"64")
        {
            return Token(r, TokenType::Int64L);
        }
        UnexpToken(L"\"i\" after a number literal must be followed by number of bits (8, 16, 32 or 64).");
    }

    if (m_code[m_offset] == L'u')
    {
        ++m_offset;
        String bits = ParseNumber();
        if (bits == L"8")
        {
            return Token(r, TokenType::Uint8L);
        }
        if (bits == L"16")
        {
            return Token(r, TokenType::Uint16L);
        }
        if (bits == L"32")
        {
            return Token(r, TokenType::Uint32L);
        }
        if (bits == L"64")
        {
            return Token(r, TokenType::Uint64L);
        }
        UnexpToken(L"\"u\" after a number literal must be followed by number of bits (8, 16, 32 or 64).");
    }
    return Token(r, TokenType::Int32L);
}

wchar_t Tokenizer::ParseEscapeChar()
{
    switch (m_code[m_offset])
    {
        // punctuation
    case L'\\':
        return L'\\';
    case L'\'':
        return L'\'';
    case L'"':
        return L'"';

        // control characters
    case L'r':
        return L'\r';
    case L'f':
        return L'\f';
    case L'v':
        return L'\v';
    case L'n':
        return L'\n';
    case L't':
        return L'\t';
    case L'b':
        return L'\b';
    case L'a':
        return L'\a';
    case L'0':
        return L'\0';
    }
    UnexpToken(L"Invalid escape character. If you need to use a backslash (\\), double it (\\\\) to avoid this error.");
}

String Tokenizer::GetLine(uint64_t& n_col)
{
    size_t s = m_code.find_last_of(L'\n', prevOffset);
    size_t e = m_code.find_first_of(L'\n', prevOffset);

    String res = m_code.substr(s, e - s);

    int i = 0;
    for (; i < res.length(); ++i)
    {
        if (!iswspace(res[i]))
        {
            break;
        }
    }

    n_col = prevCol - i;

    return res.substr(i);
}

void Tokenizer::NextLine()
{
    while (m_code[m_offset] != L'\n')
    {
        GetChar();
    }
    GetChar();
}

void Tokenizer::SkipComments()
{
    while (true)
    {
        if (m_code[m_offset] == L'/')
        {
            if (m_code[m_offset + 1] == L'/')
            {
                while (m_code[m_offset] == L'/' && m_code[m_offset + 1] == L'/')
                {
                    NextLine();
                    SkipWhite();
                }
            }
            else if (m_code[m_offset + 1] == L'*')
            {
                GetChar(); // skip /
                GetChar(); // skip *
                while (true)
                {
                    if (m_code[m_offset] == L'*' && m_code[m_offset + 1] == L'/')
                    {
                        GetChar(); // skip *
                        GetChar(); // skip /
                        break;
                    }
                    GetChar();
                }
            }
        }
        else
        {
            SkipWhite();
            return;
        }
    }
}

template<>
Token Tokenizer::ParseStringLiteral<Tokenizer::StringType::Regular>()
{
    String r = L"";
    while (m_offset < m_code.length())
    {
        switch (m_code[m_offset])
        {
        case L'"':
        {
            GetChar();
            return Token(r, TokenType::String);
        }
        case L'\n':
        {
            UnexpToken(L"End of line while parsing a string literal. Did you forget to close a quote (\")?");
        }
        case L'\\': // escape character
        {
            GetChar();
            r += ParseEscapeChar();
            GetChar();
            continue;
        }
        }

        r += m_code[m_offset];
        GetChar();
    }
    UnexpToken(L"End of file while parsing a string literal. Did you forget to close a quote (\")?");
}

template<>
Token Tokenizer::ParseStringLiteral<Tokenizer::StringType::Raw>()
{
    String r = L"";
    while (m_offset < m_code.length())
    {
        if (m_code[m_offset] == L')' && m_code[m_offset + 1] == L'"')
        {
            GetChar();
            GetChar();
            return Token(r, TokenType::String);
        }

        r += m_code[m_offset];
        GetChar();
    }

    UnexpToken(L"End of file while parsing a raw string literal. Did you forget to close a quote with bracket?");
}

Token Tokenizer::ParseRawUntil(wchar_t tt)
{
    size_t off = m_code.find_first_of(tt, m_offset) - m_offset;
    String str = m_code.substr(m_offset - 2, off);
    EatChars(off);
    prevCol = col;
    prevLine = line;
    prevOffset = m_offset;
    return Token(str, TokenType::String);
}

Token Tokenizer::ParseOperator()
{
    switch (m_code[m_offset])
    {
    case L'+':
        GetChar();
        if (m_code[m_offset] == L'+')
        {
            GetChar();
            return Token(L"++", TokenType::OperInc);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"+=", TokenType::AssignPlus);
        }
        return Token(L"+", TokenType::OperPlus);

    case L'-':
        GetChar();
        if (m_code[m_offset] == L'-')
        {
            GetChar();
            return Token(L"--", TokenType::OperDec);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"-=", TokenType::AssignMin);
        }
        else if (m_code[m_offset] == L'>')
        {
            GetChar();
            return Token(L"->", TokenType::Arrow);
        }
        return Token(L"-", TokenType::OperMin);

    case L'*':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"*=", TokenType::AssignMul);
        }
        else if (m_code[m_offset] == L'*')
        {
            GetChar();
            if (m_code[m_offset] == L'=')
            {
                GetChar();
                return Token(L"**=", TokenType::AssignPow);
            }
            return Token(L"**", TokenType::OperPow);
        }
        return Token(L"*", TokenType::OperMul);
    case L'/':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"/=", TokenType::AssignDiv);
        }
        return Token(L"/", TokenType::OperDiv);
    case L'%':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"%=", TokenType::AssignPCent);
        }
        return Token(L"%", TokenType::OperPCent);
    case L'^':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"^=", TokenType::AssignXor);
        }
        return Token(L"^", TokenType::OperXor);
    case L'<':
        GetChar();
        if (m_code[m_offset] == L'<')
        {
            GetChar();
            if (m_code[m_offset] == L'=')
            {
                GetChar();
                return Token(L"<<=", TokenType::AssignLShift);
            }
            return Token(L"<<", TokenType::OperLShift);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"<=", TokenType::OperLEqual);
        }
        return Token(L"<", TokenType::OperLess);
    case L'>':
        GetChar();
        if (m_code[m_offset] == L'>')
        {
            GetChar();
            if (m_code[m_offset] == L'=')
            {
                GetChar();
                return Token(L">>=", TokenType::AssignRShift);
            }
            return Token(L">>", TokenType::OperRShift);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L">=", TokenType::OperGEqual);
        }
        return Token(L">", TokenType::OperGreater);
    case L'&':
        GetChar();
        if (m_code[m_offset] == L'&')
        {
            GetChar();
            return Token(L"&&", TokenType::OperLAnd);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"&=", TokenType::AssignBWAnd);
        }
        return Token(L"&", TokenType::OperBWAnd);
    case L'|':
        GetChar();
        if (m_code[m_offset] == L'|')
        {
            GetChar();
            return Token(L"||", TokenType::OperLOr);
        }
        else if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"|=", TokenType::AssignBWOr);
        }
        return Token(L"|", TokenType::OperBWOr);
    case L'~':
        GetChar();
        return Token(L"~", TokenType::OperNot);
    case L'!':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"!=", TokenType::OperNEqual);
        }
        return Token(L"!", TokenType::OperLNot);
    case L';':
        GetChar();
        return Token(L";", TokenType::Semi);
    case L'=':
        GetChar();
        if (m_code[m_offset] == L'=')
        {
            GetChar();
            return Token(L"==", TokenType::OperEqual);
        }
        return Token(L"=", TokenType::Assign);

    case L'{':
        GetChar();
        return Token(L"{", TokenType::LBrace);
    case L'}':
        GetChar();
        return Token(L"}", TokenType::RBrace);

    case L'(':
        GetChar();
        return Token(L"(", TokenType::LParen);
    case L')':
        GetChar();
        if (m_code[m_offset] == L'!')
        {
            GetChar();
            return Token(L")!", TokenType::PPEnd);
        }
        return Token(L")", TokenType::RParen);

    case L'[':
        GetChar();
        return Token(L"[", TokenType::LBracket);
    case L']':
        GetChar();
        return Token(L"]", TokenType::RBracket);

    case L'.':
        GetChar();
        return Token(L".", TokenType::Dot);
    case L',':
        GetChar();
        return Token(L",", TokenType::Comma);
    case L':':
        GetChar();
        return Token(L":", TokenType::Colon);
    case L'?':
        GetChar();
        return Token(L"?", TokenType::Quest);
        // #!( PREPROCESSOR )!
    case L'#':
        GetChar();
        if (m_code[m_offset] == L'!')
        {
            GetChar();
            if (m_code[m_offset == '('])
            {
                GetChar();
                return Token(L"#!(", TokenType::PPBegin);
            }
        }
        UnexpToken(L"Invalid preprocessor directive.");

    default:
        GetChar();
        UnexpToken(L"Expected an operator. Unknown character " + m_code[m_offset]);
    }
}

Token Tokenizer::Next()
{
    if (SkipNext)
    {
        SkipNext = false;
        return SkipToken;
    }

    if (m_offset >= m_code.size())
    {
        if (EofReturned) UnexpToken(L"End of file");
        EofReturned = true;
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return Token(L"EoF", TokenType::EoF);
    }

    SkipWhite();

    if (m_offset >= m_code.size())
    {
        if (EofReturned) UnexpToken(L"End of file");
        EofReturned = true;
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return Token(L"EoF", TokenType::EoF);
    }

    SkipComments();

    if (m_offset >= m_code.size())
    {
        if (EofReturned) UnexpToken(L"End of file");
        EofReturned = true;
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return Token(L"EoF", TokenType::EoF);
    }

    wchar_t s = m_code[m_offset];
    wchar_t ns = m_code[m_offset + 1];

    // "Regular string"
    if (s == L'"')
    {
        GetChar(); // remove "
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return ParseStringLiteral<StringType::Regular>();
    }

    // @"(Raw string)"
    if (s == L'@')
    {
        if (ns == L'"' && m_code[m_offset + 2] == L'(')
        {
            GetChar(); // remove @
            GetChar(); // remove "
            GetChar(); // remove (
            prevCol = col;
            prevLine = line;
            prevOffset = m_offset;
            return ParseStringLiteral<StringType::Raw>();
        }
        else
        {
            UnexpToken(L"Invalid syntax for Raw string. It must look like this: @\"(Raw string)\"");
        }
    }

    if (IsAlpha(s) || s == L'_')
    {
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return ParseName();
    }

    if (IsNumber(s))
    {
        auto r = ParseNumberLiteral();
        if (!IsNumValid(r))
        {
            throw Error(L"Invalid number literal. It doesn't fit the given number of bits.\n");
        }
        SkipWhite();
        prevCol = col;
        prevLine = line;
        prevOffset = m_offset;
        return r;
    }

    auto r = ParseOperator();
    SkipWhite();
    prevCol = col;
    prevLine = line;
    prevOffset = m_offset;
    return r;
}

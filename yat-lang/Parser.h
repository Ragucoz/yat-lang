//  Yat programming language
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
#pragma once
#include <vector>
#include "Tokenizer.h"
#include "AST.h"

class Parser
{
    // preprocessor
    enum class PPDir
    {
        // used to allow pointers and _asm
        unsafe,
        // used to define default string class
        default_str,
        // last enum's element
        Last
    };

    struct
    {
        PPDir type = PPDir::Last;

        void Reset()
        {
            type = PPDir::Last;
        }
    } m_pp;

    // current token
    Token cur_tok;
    // previous token
    Token prev_tok;

    // tokenizer
    Tokenizer* m_tok = nullptr;
    // the current namespace
    Namespace* m_nspace = nullptr;

    std::vector<std::vector<Var*>> m_vars;

    // type of the statement
    enum class StateType
    {
        // assignment statement
        Assign,
        // function call
        FuncCall
    };

    struct TemplateParam
    {
        Keyword kw = Keyword::Last;
        String name{};

        TemplateParam(Keyword k)
        {
            kw = k;
        }

        TemplateParam(const String& n)
        {
            name = n;
        }
    };

public:
    Parser(Tokenizer* t);
    void Parse(AST& ast);

    inline Var* GetVariable(String v, String* fname = nullptr);
    inline Var* GetVariable_(const String& v);
    inline void AddVariable(Var* var, int64_t idx = -1);

    inline ASTNode* ParseExpression(bool fn, Keyword& exp_type);
    inline IfStatement* ParseIf();
    inline StatementBlock* ParseBlock(bool is_fn);
    inline ASTNode* ParseStatement();
    inline std::vector<Var*> ParseParamList();
    inline ASTNode* ShuntingYard();
    inline Var* ParseVarDecl(bool add = true);
    inline void ParseTypeParams(std::vector<TemplateParam>& p);
    inline String ParseUsing();
    inline void ParsePreProc();
    inline Range* ParseRange();
};


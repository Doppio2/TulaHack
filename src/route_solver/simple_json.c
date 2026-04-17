/* 
    TODOLIST:
    1. Change token lifetiem and storing.
    1. Change lexer(tokenaizer). Change how they live in programm.
        a. Move lexer to separte data structure.
        b. Change linked list to the basic array('cause we dont that pointers mess).
        c. Store in tokens metadata about size, head and tail of token.
    2. Change malloc to my own allocator.
       2.1 Write own strdup.
    4. Write a some wrap for json handler. 'Cause now it's not something, that someone liked to work with
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 

// TODO: need to be replaced.
#include <malloc.h>            
#include <stdbool.h>
#include <assert.h>

#include "simple_json.h"
#include "arena_allocator.h"

#include "arena_allocator.c"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

bool StringToBool(char *String)
{
    bool Result = false;                        // For safe.

    if(strcmp(String, "true") == 0)
    {
        Result = true;
    }
    else if(strcmp(String, "false") == 0)
    {
        Result = false;
    }

    return Result;
}

const char *GetTokenType(token_type Type)
{
    const char *Result;
    switch(Type)
    {
        case 0:
        {
            Result = "TOKENTYPE_BRACE_OPEN";
        } break;
        case 1:
        {
            Result = "TOKENTYPE_BRACE_CLOSE";
        } break;
        case 2:
        {
            Result = "TOKENTYPE_BRACKET_OPEN";
        } break;
        case 3:
        {
            Result = "TOKENTYPE_BRACKET_CLOSE";
        } break;
        case 4:
        {
            Result = "TOKENTYPE_STRING";
        } break;
        case 5:
        {
            Result = "TOKENTYPE_NUMBER";
        } break;
        case 6:
        {
            Result = "TOKENTYPE_COMMA";
        } break;
        case 7:
        {
            Result = "TOKENTYPE_COLON";
        } break;
        case 8:
        {
            Result = "TOKENTYPE_BOOL";
        } break;
        case 9:
        {
            Result = "TOKENTYPE_NULL";
        } break;
        default:
        {
            Result = "";
        }
    }

    return Result;
}

void AppendToken(token_list *Tokens, token Token, arena *Arena)
{
    token *NewToken = PushStruct(Arena, token); // (token*)malloc(sizeof(token));
    *NewToken = Token;

    if(Tokens->Head != 0)
    {
        Tokens->Tail->Next = NewToken;
        Tokens->Tail = NewToken;
    }
    else
    {
        Tokens->Head = NewToken;
        Tokens->Tail = NewToken;
    }

    ++Tokens->Count;
}

void FreeTokenList(token_list *Tokens, arena *TokensArena)
{
    Tokens->Head = 0;
    Tokens->Tail = 0;
    Tokens->Count = 0;
    // NOTE: Should i use ArenaClear first?
    //ArenaClear(TokensArena);
    ArenaFree(TokensArena);
}

// NOTE: For debug
void PrintTokens(token_list *Tokens) 
{
    int TokenIndex = 0;
    for(token *Current = Tokens->Head; Current != 0; Current = Current->Next)
    {
        printf("-----------------\n");
        printf("Token number %d:\nToken Type = %s\nValue = %s\n", TokenIndex, GetTokenType(Current->Type), Current->Value);
        printf("-----------------\n");
        ++TokenIndex;
    }
}

// NOTE: probably we should pass a arena, instead context.
char *Strdup(const char *Ch, simple_json_context *Ctx)
{
    char *Result = PushArray(Ctx->StringsArena, char, 1);
    // NOTE(denis): how can i get the char value from.
    *Result = *((char*)Ch);

    return Result;
}

char *AppendChar(char *String, char Ch)
{
    size_t NewLength = strlen(String)+2;
    char *Result = (char*)malloc(NewLength);
    Result[0] = '\0';
    
    strncat(Result, String, NewLength-2);
    strncat(Result, &Ch, 1);

    free(String);

    return Result;
}

token_list Lexer(char *JsonString, simple_json_context *Ctx)
{
    int JsonStringLength = (int)strlen(JsonString);
    token_list Tokens = {0};      // FIXME: not working on linux.

    int CharIndex = 0;

    // TODO: cleanup loop.
    while(CharIndex < JsonStringLength)
    {
        char Ch = JsonString[CharIndex];

        if(IsWhitespace(Ch)) 
        {
            ++CharIndex;
            continue;
        }
        else if(Ch == '{') 
        {
            token NewToken = {0};                 // FIXME: not working on linux.
            NewToken.Value = Strdup("{", Ctx);
            NewToken.Type = TOKENTYPE_BRACE_OPEN;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else if(Ch == '}') 
        {
            token NewToken = {0};                // FIXME: not working on linux.
            NewToken.Value = Strdup("}", Ctx); 
            NewToken.Type  = TOKENTYPE_BRACE_CLOSE;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else if(Ch == '[') 
        {
            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = Strdup("[", Ctx);
            NewToken.Type = TOKENTYPE_BRACKET_OPEN;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else if(Ch == ']') 
        {
            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = Strdup("]", Ctx);
            NewToken.Type =TOKENTYPE_BRACKET_CLOSE;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else if(Ch == ':')
        {
            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = Strdup(":", Ctx);
            NewToken.Type = TOKENTYPE_COLON;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else if(Ch == ',')
        {
            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = Strdup(",", Ctx);
            NewToken.Type = TOKENTYPE_COMMA;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }

        else if(isdigit(Ch) || Ch == '-')     // If number. We dont increment CharIndexValue, 'cause we need comma token.
        {
            // TODO(denis): use arena strings instead.
            // I do know should i use the dynamic array or just fixed string.
            char *String = PushArray(Ctx->StringsArena, char, 256);
            int StringIndex = 0;

            if(Ch == '-')
            {
                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];
            }

            while(isdigit(Ch) && CharIndex < JsonStringLength)
            {
                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];
            }

            if(Ch == '.')
            {
                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];

                while(isdigit(Ch) && CharIndex < JsonStringLength)
                {
                    String[StringIndex] = Ch;
                    ++CharIndex;
                    ++StringIndex;
                    Ch = JsonString[CharIndex];
                }
            }

            String[StringIndex] = '\0';

            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = String;
            NewToken.Type = TOKENTYPE_NUMBER;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
        }
        else if(Ch == 'n')      // If null. We dont increment CharIndexValue, 'cause we need comma token.
        {
            char *String = PushArray(Ctx->StringsArena, char, 256);
            int StringIndex = 0;

            // TODO: collapse it into function.
            while(strchr("nul", Ch) && CharIndex < JsonStringLength)
            {
                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];
            }

            String[StringIndex] = '\0';

            if(strcmp(String, "null") == 0)
            {
                token NewToken = {0};               // FIXME: not working on linux.
                NewToken.Value = String;
                NewToken.Type = TOKENTYPE_NULL;

                AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            }
            else
            {
                // TODO(denis): put some logic here.
            }
        }
        // TODO: a boolean parse logic need to be refactor.
        else if(Ch == 't' || Ch == 'f')      // If boolean. We dont increment CharIndexValue, 'cause we need comma token.
        {
            // TODO(denis): change boolean parse logic.  
            char *String = PushArray(Ctx->StringsArena, char, 6);
            int StringIndex = 0;

            // TODO: collapse it into function.
            while((strchr("true", Ch) || strchr("false", Ch)) && CharIndex < JsonStringLength)
            {
                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];
            }
            String[StringIndex] = '\0';
            if(strcmp(String, "false") == 0 || strcmp(String, "true") == 0)   // NOTE: maybe i should write a macro for strcmp?????
            {
                token NewToken = {0};               // FIXME: not working on linux.
                NewToken.Value = String;
                NewToken.Type = TOKENTYPE_BOOL;

                AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            }
            else
            {
                // TODO(denis): put some logic here.
            }
        }
        else if(Ch == '"')   
        {
            // TODO(denis): Change the string parsing logic.
            ++CharIndex;
            Ch = JsonString[CharIndex];

            char *String = PushArray(Ctx->StringsArena, char, 256);
            int StringIndex = 0;

            while(CharIndex < JsonStringLength)
            {
                // TODO: Rewrite!!!
                if(Ch == '"' && 
                   CharIndex > 0 &&
                   JsonString[CharIndex-1] != '\\')
                {
                    break;
                }

                /*
                String = AppendChar(String, Ch);
                ++CharIndex;
                Ch = JsonString[CharIndex];
                */

                String[StringIndex] = Ch;
                ++CharIndex;
                ++StringIndex;
                Ch = JsonString[CharIndex];
            }

            String[StringIndex] = '\0';

            token NewToken = {0};               // FIXME: not working on linux.
            NewToken.Value = String;
            NewToken.Type = TOKENTYPE_STRING;

            AppendToken(&Tokens, NewToken, Ctx->TokensArena);
            ++CharIndex;
        }
        else
        {
            // TODO: Log it later.
            printf("Unexpected character %c", Ch);
            ++CharIndex;
            continue;
        }
    }

    //PrintTokens(&Tokens);                         // For debug.

    return(Tokens);
}

ast_node *CreateASTNode(ast_node NewASTNodeData, simple_json_context *Ctx)
{
    ast_node *Result = PushStruct(Ctx->ASTArena, ast_node);
    *Result = NewASTNodeData;
    
    return Result;
}

ast_node *CreateASTNodeNull(simple_json_context *Ctx)
{
    ast_node *Result = PushStruct(Ctx->ASTArena, ast_node);

    return Result;
}

void UpdateToken(token **Token) 
{
    if((*Token)->Next)
    {
        (*Token) = (*Token)->Next;
    }
    else 
    {
        (*Token) = 0;
    }
}

ast_node *ParseValue(token **Token, simple_json_context *Ctx)
{
    // TODO: Collapse this rerturns later.
    // TODO: Parser use 2 heap structs. Array and objects.
    switch((*Token)->Type)
    {
        case TOKENTYPE_BRACE_OPEN:
        {
            ast_node *NewASTNode = ParseJsonObject(Token, Ctx);

            return NewASTNode;
        } break;
        case TOKENTYPE_BRACKET_OPEN:
        {
            ast_node *NewASTNode = ParseJsonArray(Token, Ctx);

            return NewASTNode;
        } break;
        case TOKENTYPE_STRING:
        {
            ast_node ResultASTNode;
            ZeroStruct(&ResultASTNode);
            ResultASTNode.Type = ASTNODE_STRING;
            // FIXME(denis): Now we just copy string from token to AST.
            // I do not want that. I can't use strdup now.
            ResultASTNode.JsonStr = (*Token)->Value;
            ast_node *NewASTNode = CreateASTNode(ResultASTNode, Ctx);

            return NewASTNode;
        } break;
        case TOKENTYPE_NUMBER:
        {
            ast_node ResultASTNode;
            ZeroStruct(&ResultASTNode);
            ResultASTNode.Type = ASTNODE_NUMBER;
            ResultASTNode.JsonNum = atoi((*Token)->Value);
            ResultASTNode.JsonFloat = atof((*Token)->Value);
            ast_node *NewASTNode = CreateASTNode(ResultASTNode, Ctx);

            return NewASTNode;
        } break;
        case TOKENTYPE_BOOL:
        {
            ast_node ResultASTNode;
            ZeroStruct(&ResultASTNode);
            ResultASTNode.Type = ASTNODE_BOOL;
            ResultASTNode.JsonBool = StringToBool((*Token)->Value);
            ast_node *NewASTNode = CreateASTNode(ResultASTNode, Ctx);

            return NewASTNode;
        } break;
        case TOKENTYPE_NULL:
        {
            ast_node ResultASTNode;
            ZeroStruct(&ResultASTNode);
            ResultASTNode.Type = ASTNODE_NULL;
            ResultASTNode.JsonNum = 0;           // NULL.
            ast_node *NewASTNode = CreateASTNode(ResultASTNode, Ctx);

            return NewASTNode;
        } break;
        default:
        {
            // TODO: Log later.
            printf("Unexpected token! %s\n", (*Token)->Value);
            return 0;
        }
    }
}

ast_node *ParseJsonObject(token **Token, simple_json_context *Ctx)
{
    ast_node *Result = CreateASTNodeNull(Ctx);
    Result->JsonObj = 0;
    Result->Type = ASTNODE_OBJECT;
    UpdateToken(Token);

    while((*Token)->Type != TOKENTYPE_BRACE_CLOSE)
    {
        // Get key.
        if((*Token)->Type == TOKENTYPE_STRING)
        {
            char *Key = (*Token)->Value;     // Get the key name and put it into hash table later.
            UpdateToken(Token);

            if((*Token)->Type != TOKENTYPE_COLON)
            {
                // TODO: Log later.
                assert((*Token)->Type == TOKENTYPE_COLON);
            }

            UpdateToken(Token);

            ast_node *ValueNode = ParseValue(Token, Ctx);              // Get the value from current token recursively.
            shput(Result->JsonObj, Key, ValueNode);           // Continue later.
        }

        UpdateToken(Token);

        if((*Token)->Type == TOKENTYPE_COMMA) 
        {
            UpdateToken(Token);
        }
    }

    return Result;
}

ast_node *ParseJsonArray(token **Token, simple_json_context *Ctx)
{
    ast_node *Result = CreateASTNodeNull(Ctx);
    Result->JsonArr = 0;
    Result->Type = ASTNODE_ARRAY;
    UpdateToken(Token);     // Eat '['

    while((*Token)->Type != TOKENTYPE_BRACKET_CLOSE) 
    {
        ast_node *ValueNode = ParseValue(Token, Ctx);
        arrput(Result->JsonArr, ValueNode);

        UpdateToken(Token);

        if((*Token)->Type == TOKENTYPE_COMMA) 
        {
            UpdateToken(Token);
        }
    }

    return Result;
}

ast_node *Marshal(char *String)
{
    simple_json_context *Ctx = (simple_json_context*)malloc(sizeof(simple_json_context));
    Ctx->TokensArena   = ArenaAlloc(ArenaDefaultSize);
    Ctx->ASTArena      = ArenaAlloc(ArenaDefaultSize); // NOTE: do i need separate data structure for syntax tree?
    Ctx->StringsArena  = ArenaAlloc(ArenaDefaultSize);

    token_list Tokens = Lexer(String, Ctx);

    if(Tokens.Count == 0)
    {
        // TODO: Log later.
        printf("Nothing to parse\n");
    }

    token *Token = Tokens.Head;    // NOTE: Get the first token in the list.

    ast_node *AST = ParseValue(&Token, Ctx);   // NOTE: Start parsing proccess here.

    // NOTE: return a more readable object, than just raw ast. 
    return AST;
}

void PrintValue(ast_node *ASTNode) 
{
    switch(ASTNode->Type)
    {
        case ASTNODE_OBJECT:
        {
            PrintJsonObject(ASTNode);
        } break;
        case ASTNODE_ARRAY:
        {
            PrintJsonArray(ASTNode);
        } break;
        case ASTNODE_STRING:
        {
            printf("%s\n", ASTNode->JsonStr);
        } break;
        case ASTNODE_NUMBER:
        {
            printf("%d\n", ASTNode->JsonNum);
        } break;
        case ASTNODE_BOOL:
        {
            printf("%d\n", ASTNode->JsonBool);
        } break;
        case ASTNODE_NULL:
        {
            printf("%d\n", ASTNode->JsonNum);
        } break;
        default:
        {
        }
    }
}

void PrintJsonArray(ast_node *ASTNode)
{
    for(int index = 0; index < arrlen(ASTNode->JsonArr); index++)
    {
        PrintValue(ASTNode->JsonArr[index]);
    }
}

void PrintJsonObject(ast_node *ASTNode) 
{
    for(int index = 0; index < hmlen(ASTNode->JsonObj); index++) 
    {
        PrintValue(ASTNode->JsonObj[index].value);
    }
}

void PrintJsonAST(ast_node *AST)
{
    PrintValue(AST);
}

void FreeJsonObject(ast_node **ASTNodePtr)
{
    if(ASTNodePtr != 0 && *ASTNodePtr != 0)
    {
        // Loop through the hash table and free all nodes.
        for(int index = 0; index < shlen((*ASTNodePtr)->JsonObj); index++)
        {
//            FreeJsonString(&((*ASTNodePtr)->JsonObj[index].key));
            FreeASTNode(&((*ASTNodePtr)->JsonObj[index].value));
        }

        shfree((*ASTNodePtr)->JsonObj);
        (*ASTNodePtr)->JsonObj = 0;
        *ASTNodePtr = 0;
    }
}

void FreeJsonArray(ast_node **ASTNodePtr)
{
    if(ASTNodePtr != 0 && *ASTNodePtr != 0)
    {
        for(int index = 0; index < arrlen((*ASTNodePtr)->JsonArr); index++)
        {
            FreeASTNode(&((*ASTNodePtr)->JsonArr[index]));
        }

        arrfree((*ASTNodePtr)->JsonArr);
        (*ASTNodePtr)->JsonArr = 0;
        *ASTNodePtr = 0;
    }
}

// FreeTokenList() function free's all dynamic strings anyway, 
// so, this function just causes erreos anyway.
/*
void FreeJsonString(char **JsonStringPtr)
{
    if (JsonStringPtr != 0 && *JsonStringPtr != 0)
    {
        free(*(JsonStringPtr));
        *JsonStringPtr = 0;
    }
}
*/

void FreeASTNode(ast_node **ASTNodePtr)
{
    switch((*ASTNodePtr)->Type)
    {
        case ASTNODE_OBJECT:
        {
            FreeJsonObject(ASTNodePtr);
        } break;
        case ASTNODE_ARRAY:
        {
            FreeJsonArray(ASTNodePtr);
        } break;
        default:
        {
        }
    }

    free(*ASTNodePtr);
    *ASTNodePtr = 0;
}

void FreeJsonASTRecursively(ast_node *AST)
{
    // TODO: too slow. I need arena free here. But i can't now, 'cause
    // I use stb array and hash maps here.
    FreeASTNode(&AST);
}

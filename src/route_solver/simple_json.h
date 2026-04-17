#if !defined(_SIMPLE_JSON_H)
#define _SIMPLE_JSON_H

#include "arena_allocator.h"

#define ArrayCount(Array) (sizeof(*Array) / sizeof((Array)[0]))

#define IsWhitespace(Ch) (Ch == ' ' || Ch == '\n' || Ch == '\r' || Ch == '\t')

// TODO: i need my own strdup, that use my allocator.
/*
#if defined(_WIN32)
#define Strdup(ch) (_strdup((ch)))
#elif defined(__clang__) || defined(__GNUC__)
#define Strdup(ch) (strdup((ch)))
#endif
*/

typedef enum
{
    TOKENTYPE_BRACE_OPEN = 0,
    TOKENTYPE_BRACE_CLOSE,
    TOKENTYPE_BRACKET_OPEN,
    TOKENTYPE_BRACKET_CLOSE,
    TOKENTYPE_STRING,
    TOKENTYPE_NUMBER,
    TOKENTYPE_COMMA,
    TOKENTYPE_COLON,
    TOKENTYPE_BOOL,
    TOKENTYPE_NULL
} token_type;

// struct for meta information.
typedef struct simple_json_context {
    arena *TokensArena;
    arena *ASTArena;         // NOTE: do i need separate data structure for syntax tree?
    arena *StringsArena;
} simple_json_context;

// TODO: refactor the token struct. Instead of using linked list, i should use a default array and store it in lexer struct.
typedef struct token
{
    struct token *Next;
    char *Value;
    token_type Type;
} token;

// TODO: Rename it to lexer struct and refactor from linked list to array.
typedef struct 
{
    token *Head;
    token *Tail;
    int Count;
} token_list;

// NOTE: Maybe i should put it in struct.
typedef enum
{
    ASTNODE_OBJECT = 0,
    ASTNODE_ARRAY,
    ASTNODE_STRING,
    ASTNODE_NUMBER,
    ASTNODE_BOOL,
    ASTNODE_NULL
} ast_node_type;

typedef struct json_object
{
    // TODO: While i do not have my own hash map, i using stb one here.
    char *key;
    struct ast_node *value;

} json_object;

typedef struct ast_node
{
    ast_node_type Type;

    // TODO: In c++ you can not write anonymous structures or unions with name.
    // So, for copabilites compatibility i write it more unreadeble ):
    // This union is value of ast_node.
    union
    {
        json_object *JsonObj;
        struct ast_node **JsonArr;        // TODO: Implement my own dynamic array, or just use the arena.
        char *JsonStr;
        int JsonNum;
        bool JsonBool;
    };

    double JsonFloat;

} ast_node;

/*                  For tokens                     */
// TODO: rename it to convention: ObjName-Operation like.
const char *GetTokenType(token_type Type);
void AppendToken(token_list *Tokens, token Token, arena *TokensArena);
void FreeTokenList(token_list *Tokens, arena *TokensArena);
void PrintTokens(token_list *Tokens);                  // For debug.

token_list Lexer(char *JsonString, simple_json_context *Ctx);

ast_node *CreateASTNode(ast_node NewASTNodeData, simple_json_context *Ctx);
ast_node *CreateASTNodeNull(simple_json_context *Ctx);

token *GetNextToken(token **Token);
ast_node *ParseValue(token **Token, simple_json_context *Ctx);
ast_node *ParseJsonObject(token **Token, simple_json_context *Ctx);
ast_node *ParseJsonArray(token **Token, simple_json_context *Ctx);

ast_node *Marshal(char *String);

void FreeASTNode(ast_node **ASTNodePtr);
void FreeJsonObject(ast_node **ASTNodePtr);
void FreeJsonArray(ast_node **ASTNodePtr);
void FreeJsonString(char **JsonStringPtr);
void FreeJsonASTRecursively(ast_node *AST);

/*           For Debugging           */
void PrintJsonAST(ast_node *ASTNode);
void PrintJsonObject(ast_node *ASTNode);
void PrintJsonArray(ast_node *ASTNode);
void PrintValue(ast_node *ASTNode);

/*           For strings             */
char *AppendChar(char *String, char Ch);

#endif

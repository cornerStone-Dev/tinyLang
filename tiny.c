/* tiny.c */

#include <stdio.h>
#include <stdlib.h>   
#include <string.h>

#define  HASHTABLE_STATIC_BUILD_IN
#include "hashTable/hashTable.c"
#define  STRINGLISTSTACK_ASSUME_SPACE
#define  STRINGLISTSTACK_STATIC_BUILD_IN
#include "stringListStack/stringListStack.c"
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;
typedef float     f32;
typedef double    f64;
#define alignOf   _Alignof
#define atomic    _Atomic
#define noReturn  _Noreturn
#define complex   _Complex
#define imaginary _Imaginary
#define alignAs   _Alignas

#define NDEBUG
#define tinyParse_ENGINEALWAYSONSTACK
#include <assert.h>

/*******************************************************************************
 * Section Types
*******************************************************************************/
typedef struct AST_s AST;

typedef struct {
	u8                 *fileName;
	HashTable          *hashTable;
	S_stringListStack  *stringLS;
	AST                *parseTree;
	AST               **scopedSymbols;
	u8                 *programStart;
	AST                *returnNode;
	AST                *mainFunction;
	u32                 numLocalDecls;
	u32                 maxLocalStack;
	u8                  errorPrinted;
} Context;

typedef struct {
	u8  *s;
	union{
		struct{
			u32 lineNumber;
			u32 attribute;
		};
		AST *ast;
	};
} Token;

typedef union Data Data;

typedef union Data {
	u8   *s;
	s64   i;
	f64   d;
	Data *a;
} Data;

typedef struct GlobalInfo {
	u8  *buffp;
	u8  *start;
} GlobalInfo;

enum e_nodeKind{
	OP_CODE=1,
	OP_ASSIGN,
	OP_EQUALS,
	OP_NOTEQUALS,
	OP_LT,
	OP_GT,
	OP_LTEQ,
	OP_GTEQ,
	OP_OR,
	OP_XOR,
	OP_AND,
	OP_LBITSHIFT,
	OP_RBITSHIFT,
	OP_PLUS,
	OP_MINUS,
	OP_STAR,
	OP_DIVI,
	OP_MOD,
	OP_NOT,
	OP_TILDA,
	OP_NEG,
	CONST_VAL,
	IDENT_INC,
	GLOBAL_IDENT_INC,
	IDENT_VAL,
	ARRAY_VAL,
	IDENT_STORE,
	ARRAY_STORE,
	GLOBAL_IDENT_VAL,
	GLOBAL_ARRAY_VAL,
	GLOBAL_IDENT_STORE,
	GLOBAL_ARRAY_STORE,
	FUNC_CALL,
	FUNC_CALL_RETURN_IGNORED,
	PRINT_INTEGER,
	TYPE_CAST,
	ST_RETURN,
	ST_WHILE,
	ST_IF,
	ST_IFELSE,
	ST_PARAMETER,
	ST_DECL_VAR,
	ST_DECL_VAR_ARRAY,
	ST_PARAM_VAR,
	ST_PARAM_VAR_ARRAY,
	ST_COMPOUND,
	ST_GLOBAL_DECL_VAR,
	ST_GLOBAL_DECL_VAR_ARRAY,
	ST_FUNC_DECL,
	ST_EXPR,
	ST_UJMP,
	ST_CJMP
};

enum e_exprType{
	S64_TYPE=1,
	F64_TYPE,
	ARRAY_S64_TYPE,
	ARRAY_F64_TYPE,
	VOID_TYPE
};

typedef struct Symbol_s {
	u32 type;
} Symbol;

typedef struct LexInfo_s {
	u8  *cursor;
	u64  token;
} LexInfo;

typedef struct AST_s {
	AST  *children;
	AST  *siblings;
	Data  attribute;
	u32   lineNumber;
	u16   exprType;
	u8    nodeKind;
	u8    identLen;
} AST;

#include "vm.c"

/*******************************************************************************
 * Section Construct AST
*******************************************************************************/

static AST*
statement(u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->lineNumber = lineNumber;
	return node;
}

static AST*
expression(u32 lineNumber, u32 opCode)
{
	AST *node = calloc(1, sizeof(AST));
	node->lineNumber = lineNumber;
	node->nodeKind = opCode;
	return node;
}

static AST*
intConstant(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.i = strtol(t.s, NULL, 0);
	node->lineNumber = lineNumber;
	node->nodeKind = CONST_VAL;
	node->exprType = S64_TYPE;
	return node;
}

static AST*
floatConstant(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.d = atof(t.s);
	node->lineNumber = lineNumber;
	node->nodeKind = CONST_VAL;
	node->exprType = F64_TYPE;
	return node;
}

static AST*
charConstant(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.s = t.s;
	node->lineNumber = lineNumber;
	node->nodeKind = CONST_VAL;
	node->exprType = S64_TYPE;
	return node;
}

static AST*
indentifierExpr(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.s = t.s;
	node->lineNumber = lineNumber;
	node->nodeKind = IDENT_VAL;
	node->identLen = t.attribute;
	return node;
}

static AST*
indentifierIncExpr(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.s = t.s;
	node->lineNumber = lineNumber;
	node->nodeKind = IDENT_INC;
	node->identLen = t.attribute;
	return node;
}

static AST*
indentifierArrayExpr(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.s = t.s;
	node->lineNumber = lineNumber;
	node->nodeKind = ARRAY_VAL;
	node->identLen = t.attribute;
	return node;
}

static AST*
indentifierFuncExpr(Token t, u32 lineNumber)
{
	AST *node = calloc(1, sizeof(AST));
	node->attribute.s = t.s;
	node->lineNumber = lineNumber;
	node->nodeKind = FUNC_CALL;
	node->identLen = t.attribute;
	return node;
}

/*******************************************************************************
 * Section AST Processing Functions
*******************************************************************************/

static void
localVarsScoped(Context *c, AST *node)
{
	
	u32 index;
	s32 returnValue;
	
	switch(node->nodeKind)
	{
		case OP_ASSIGN:
		{
			node->children->siblings->nodeKind+=2;
		}
		break;
		
		case ST_DECL_VAR:
		// add to stack list
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_insert_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			printf("localVarsScoped: %s",
				stringListStack_debugString(returnValue));
		}
		c->scopedSymbols[index] = node;
		printf("DECL %s is %d, at %d\n", 
			node->attribute.s, node->exprType, index);
		break;
		
		case ST_DECL_VAR_ARRAY:
		// add to stack list
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_insert_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			printf("localVarsScoped: %s",
				stringListStack_debugString(returnValue));
		}
		c->scopedSymbols[index] = node;
		node->identLen = index;
		printf("DECL %s is %d, at %d\n", 
			node->attribute.s, node->exprType, index);
		break;
		
		case ST_PARAM_VAR:
		case ST_PARAM_VAR_ARRAY:
		// add to stack list
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_insert_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			printf("localVarsScoped: %s",
				stringListStack_debugString(returnValue));
		}
		c->scopedSymbols[index] = node;
		printf("Parameter %s is %d, at %d\n", 
			node->attribute.s, node->exprType, index);
		break;
		
		case ST_COMPOUND:
		{
			stringListStack_enterScope(c->stringLS);
			printf("Start of Block: Entering Scope. Num of local vars = %d\n"
				, node->exprType);
			if (node->exprType){
				*c->programStart = VM_OP_PUSH_LOCALS;
				c->programStart++;
				*c->programStart = node->exprType;
				c->programStart++;
			}
		}
		break;
		
		case ST_FUNC_DECL:
		stringListStack_enterScope(c->stringLS);
		//printf("Start of Function: Entering Scope.\n");
		// record return type
		c->returnNode = node;
		printf("Return Type defined as %d\n", node->exprType);
		
		// need to record starting point of function for jumping
		node->attribute.s = c->programStart;
		break;
		
		case IDENT_INC:
		// look up type in symbol table
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_find_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			// do a global symbol table look up
			hashTableNode *result=
			hashTable_find_internal(
				c->hashTable,
				node->attribute.s,
				node->identLen);
			
			if(result == 0){
				printf("error: symbol '%s' not found.\n",node->attribute.s);
			} else {
				AST *var = (AST*)(result->value);
				node->exprType = var->exprType;
				node->nodeKind+=1;
				node->identLen = var->attribute.i;
			}
		} else {
			node->exprType = c->scopedSymbols[index]->exprType;
			node->identLen = index;
		}
		//printf("%s is %d\n", node->attribute.s, node->exprType);
		break;
		
		case IDENT_VAL:
		case IDENT_STORE:
		// look up type in symbol table
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_find_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			// do a global symbol table look up
			hashTableNode *result=
			hashTable_find_internal(
				c->hashTable,
				node->attribute.s,
				node->identLen);
			
			if(result == 0){
				printf("error: symbol '%s' not found.\n",node->attribute.s);
			} else {
				AST *var = (AST*)(result->value);
				node->exprType = var->exprType;
				node->nodeKind+=4;
				node->identLen = var->attribute.i;
			}
		} else {
			node->exprType = c->scopedSymbols[index]->exprType;
			node->identLen = index;
		}
		//printf("%s is %d\n", node->attribute.s, node->exprType);
		break;
		
		case ARRAY_VAL:
		case ARRAY_STORE:
		// look up type in symbol table
		node->attribute.s[node->identLen] = 0;
		if( (returnValue =
		stringListStack_find_internal(
			c->stringLS,
			node->attribute.s,
			node->identLen,
			&index))
		){
			// do a global symbol table look up
			hashTableNode *result=
			hashTable_find_internal(
				c->hashTable,
				node->attribute.s,
				node->identLen);
			
			if(result == 0){
				printf("error: symbol '%s' not found.\n",node->attribute.s);
			} else {
				AST *var = (AST*)(result->value);
				//printf("AST   %s is %d\n", var->attribute.s, var->exprType);
				// change expr type to underlying type
				node->exprType = var->exprType-2;
				// change node to be global type
				node->nodeKind+=4;
				// save off offset into globals
				node->identLen = var->attribute.i;
			}
		} else {
			//printf("AST   %s is %d looking at %d\n", c->scopedSymbols[index]->attribute.s, c->scopedSymbols[index]->exprType, index);
			node->exprType = c->scopedSymbols[index]->exprType-2;
			node->identLen = index;
		}
		//printf("%s is %d\n", node->attribute.s, node->exprType);
		break;
		
		case FUNC_CALL:
		{
			node->attribute.s[node->identLen] = 0;
			if (strcmp(
				(const char *)node->attribute.s,
				"print") == 0)
			{
				node->exprType = VOID_TYPE;
				node->identLen = 1;
				node->nodeKind = PRINT_INTEGER;
				break;
			}
			hashTableNode *result=
			hashTable_find_internal(
				c->hashTable,
				node->attribute.s,
				node->identLen);
			
			if(result == 0){
				printf("error FUNC_CALL: symbol '%s' not found.\n",node->attribute.s);
			} else {
				AST *var = (AST*)(result->value);
				node->attribute.s = (u8*)var;
				node->exprType = var->exprType;
				node->identLen = var->identLen;
			}
		}
		break;
		
		case FUNC_CALL_RETURN_IGNORED:
		{
			node->attribute.s[node->identLen] = 0;
			if (strcmp(
				(const char *)node->attribute.s,
				"print") == 0)
			{
				node->exprType = VOID_TYPE;
				node->identLen = 1;
				node->nodeKind = PRINT_INTEGER;
				break;
			}
			hashTableNode *result=
			hashTable_find_internal(
				c->hashTable,
				node->attribute.s,
				node->identLen);
			
			if(result == 0){
				printf("error FUNC_CALL: symbol '%s' not found.\n",node->attribute.s);
			} else {
				AST *var = (AST*)(result->value);
				node->attribute.s = (u8*)var;
				node->exprType = var->exprType;
				node->identLen = var->identLen;
			}
		}
		break;
		
		case ST_WHILE:
		{
			// save off current ip for jumping
			node->attribute.s = c->programStart;
		}
		break;
		
		default:
		break;
	}
}

static void
typeCheckNode(Context *c, AST *node)
{
	//~ u32 index=0;
	//~ s32 returnValue;
	switch(node->nodeKind)
	{
		case OP_ASSIGN:
		{
			// type check
			if( (node->children->exprType != node->children->siblings->exprType)
			 )
			{
				printf("Invalid types: %d and %d for operation on line %d.\n",
					node->children->exprType,node->children->siblings->exprType,
					node->lineNumber);
			}
		}
		break;
		
		case OP_OR ... OP_RBITSHIFT:
		if( (node->children->exprType != node->children->siblings->exprType)
			|| (node->children->exprType != S64_TYPE) )
		{
			printf("Invalid types: %d and %d for operation on line %d.\n",
				node->children->exprType, node->children->siblings->exprType,
				node->lineNumber);
		} else {
			node->exprType = node->children->exprType;
		}
		
		// code gen
		*c->programStart = node->nodeKind - OP_OR + VM_OP_BITWISE_OR;
		c->programStart++;
		break;
		
		case OP_EQUALS ... OP_GTEQ:
		if( (node->children->exprType != node->children->siblings->exprType)
			 )
		{
			printf("Invalid types: %d and %d for operation on line %d.\n",
				node->children->exprType, node->children->siblings->exprType,
				node->lineNumber);
		} else {
			node->exprType = S64_TYPE;
		}
		
		// code gen
		*c->programStart = node->nodeKind - OP_EQUALS + VM_OP_EQUALS;
		c->programStart++;
		break;
		
		case OP_PLUS ... OP_MOD:
		if( (node->children->exprType != node->children->siblings->exprType)
			 )
		{
			printf("Invalid types: %d and %d for operation on line %d.\n",
				node->children->exprType, node->children->siblings->exprType,
				node->lineNumber);
		} else {
			node->exprType = node->children->exprType;
		}
		
		// code gen
		*c->programStart = node->nodeKind - OP_PLUS + VM_OP_ADD;
		c->programStart++;
		break;
		
		case OP_NOT ... OP_TILDA:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation NOT or TILDA on line %d.\n",
				node->children->exprType, node->lineNumber);
		} else {
			node->exprType = node->children->exprType;
		}
		
		// code gen
		*c->programStart = node->nodeKind - OP_NOT + VM_OP_LOGICAL_NOT;
		c->programStart++;
		break;
		
		case CONST_VAL:
		{
			c->programStart = listWriteInt(c->programStart, node->attribute.i);
		}
		break;
		
		case IDENT_INC:
		// code gen of local variable
		*c->programStart = VM_OP_PUSH_VAR_INC;
		c->programStart++;
		// offset into local variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case GLOBAL_IDENT_INC:
		// code gen of local variable
		*c->programStart = VM_OP_PUSH_GLOBAL_INC;
		c->programStart++;
		// offset into local variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case IDENT_VAL:
		// code gen of local variable
		*c->programStart = VM_OP_PUSH_VAR_00 + node->identLen;
		c->programStart++;
		// offset into local variables
		//~ *c->programStart = node->identLen;
		//~ c->programStart++;
		break;
		
		case ARRAY_VAL:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation array [INDEX] on line %d.\n",
				node->children->exprType, node->lineNumber);
		}
		// code gen of local variable array indexing to value
		// load the index command
		*c->programStart = VM_OP_LOAD_INDEX;
		c->programStart++;
		// offset into local variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case IDENT_STORE:
		{
			// code gen of storing to local variable
			*c->programStart = VM_OP_STORE_VAR_00 + node->identLen;
			c->programStart++;
			// offset into local variables
			//~ *c->programStart = node->identLen;
			//~ c->programStart++;
		}
		break;
		
		case ARRAY_STORE:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation array [INDEX] on line %d.\n",
				node->children->exprType, node->lineNumber);
		}
		// code gen of local variable array indexing to value
		// load the index command
		*c->programStart = VM_OP_STORE_INDEX;
		c->programStart++;
		// offset into local variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;

		case GLOBAL_IDENT_VAL:
		// code gen of local variable
		*c->programStart = VM_OP_PUSH_GLOBAL;
		c->programStart++;
		// offset into global variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case GLOBAL_ARRAY_VAL:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation array [INDEX] on line %d.\n",
				node->children->exprType, node->lineNumber);
		}
		// code gen of local variable array indexing to value
		// load the index command
		*c->programStart = VM_OP_LOAD_INDEX_GLOBAL;
		c->programStart++;
		// offset into global variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case GLOBAL_IDENT_STORE:
		{
			// code gen of storing to local variable
			*c->programStart = VM_OP_STORE_GLOBAL;
			c->programStart++;
			// offset into local variables
			*c->programStart = node->identLen;
			c->programStart++;
		}
		break;
		
		case GLOBAL_ARRAY_STORE:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation array [INDEX] on line %d.\n",
				node->children->exprType, node->lineNumber);
		}
		// code gen of local variable array indexing to value
		// load the index command
		*c->programStart = VM_OP_STORE_INDEX_GLOBAL;
		c->programStart++;
		// offset into local variables
		*c->programStart = node->identLen;
		c->programStart++;
		break;

		case OP_NEG:
		if( (node->children->exprType != S64_TYPE)
			&& (node->children->exprType != F64_TYPE))
		{
			printf("Invalid types: %d for operation NEG on line %d.\n",
				node->children->exprType, node->lineNumber);
		} else {
			node->exprType = node->children->exprType;
		}
		
		// code gen
		*c->programStart = node->nodeKind - OP_NEG + VM_OP_NEG;
		c->programStart++;
		break;
		
		case FUNC_CALL:
		{
			//printf("** I AM HERE **\n");
			AST *funcDecl = (AST*)node->attribute.s;
			if ((node->children == 0) && (funcDecl->children->exprType == VOID_TYPE))
			{
				//printf("** GOOD EXIT 1 **\n");
				break;
			} else if  (node->children == 0) {
				printf("error FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
				break;
			}
			if(node->children->exprType != funcDecl->children->exprType)
			{
				printf("errorA FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
				break;
			} else if((node->children->siblings == 0) && (funcDecl->children->siblings== 0))
			{
				//printf("** GOOD EXIT 2 **\n");
				break;
			} else {
				AST *walkNode = node->children->siblings;
				AST *walkDecl = funcDecl->children->siblings;
				while(1)
				{
					if(walkDecl->nodeKind == ST_COMPOUND)
					{
						break;
					}
					if ( (((((u64)walkNode!=0)) + ((u64)walkDecl!=0)) != 2) 
						|| (walkNode->exprType != walkDecl->exprType)
						)
					{
						printf("errorB FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
						goto end;
					}
					walkNode = walkNode->siblings;
					walkDecl = walkDecl->siblings;
					if(walkNode==0 && walkDecl==0)
					{
						break;
					}
				}
			}
			//printf("** GOOD EXIT 3 **\n");
			// code gen
			// arguments are pushed
			//~ *c->programStart = 0;
			//~ c->programStart++;
			//~ s64 offset = (funcDecl->attribute.s - (c->programStart+4));
			//~ *c->programStart = VM_OP_CALL;
			//~ c->programStart++;
			//~ *c->programStart = (offset >> 8) & 0xFF;
			//~ c->programStart++;
			//~ *c->programStart = offset & 0xFF;
			//~ c->programStart++;
			//~ *c->programStart = node->identLen;
			//~ if (node->identLen > 0)
			//~ {
				//~ *c->programStart = node->identLen-1;
			//~ } else {
				//~ *c->programStart = node->identLen;
			//~ }
			//~ c->programStart++;
			s64 offset = (funcDecl->attribute.s - (c->programStart+3));
			*c->programStart = VM_OP_CALL0+node->identLen;
			c->programStart++;
			*c->programStart = (offset >> 8) & 0xFF;
			c->programStart++;
			*c->programStart = offset & 0xFF;
			c->programStart++;
		}
		break;

		case FUNC_CALL_RETURN_IGNORED:
		{
			//printf("** I AM HERE **\n");
			AST *funcDecl = (AST*)node->attribute.s;
			if ((node->children == 0) && (funcDecl->children->exprType == VOID_TYPE))
			{
				//printf("** GOOD EXIT 1 **\n");
				break;
			} else if  (node->children == 0) {
				printf("error FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
				break;
			}
			if(node->children->exprType != funcDecl->children->exprType)
			{
				printf("errorA FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
				break;
			} else if((node->children->siblings == 0) && (funcDecl->children->siblings== 0))
			{
				//printf("** GOOD EXIT 2 **\n");
				break;
			} else {
				AST *walkNode = node->children->siblings;
				AST *walkDecl = funcDecl->children->siblings;
				while(1)
				{
					if(walkDecl->nodeKind == ST_COMPOUND)
					{
						break;
					}
					if ( (((((u64)walkNode!=0)) + ((u64)walkDecl!=0)) != 2) 
						|| (walkNode->exprType != walkDecl->exprType)
						)
					{
						printf("errorB FUNC_CALL: wrong parameters line %d.\n", node->lineNumber);
						goto end;
					}
					walkNode = walkNode->siblings;
					walkDecl = walkDecl->siblings;
					if(walkNode==0 && walkDecl==0)
					{
						break;
					}
				}
			}
			//printf("** GOOD EXIT 3 **\n");
			// code gen
			// arguments are pushed
			//~ *c->programStart = 0;
			//~ c->programStart++;
			s64 offset = (funcDecl->attribute.s - (c->programStart+3));
			*c->programStart = VM_OP_CALL0+node->identLen;
			c->programStart++;
			*c->programStart = (offset >> 8) & 0xFF;
			c->programStart++;
			*c->programStart = offset & 0xFF;
			c->programStart++;
			//~ *c->programStart = node->identLen;
			//~ c->programStart++;
		}
		break;
		
		case PRINT_INTEGER:
		{
			*c->programStart = VM_OP_PRINT;
			c->programStart++;
		}
		break;

		case ST_RETURN:
		// look up type in symbol table
		node->exprType = c->returnNode->exprType;
		//printf("Return type is %d\n", c->returnNode->exprType);
		if( ((node->children == 0) && (node->exprType!=VOID_TYPE))
			|| ((node->children != 0)
			&& (node->children->exprType != node->exprType)) )
		{
			printf("Invalid return type %d: expected %d line %d.\n",
				node->children->exprType, node->exprType, node->lineNumber);
		}
		// code gen
		*c->programStart = VM_OP_RETURN;
		c->programStart++;
		break;
		
		case ST_WHILE:
		{
			// type check
			if(node->children->exprType != S64_TYPE)
			{
				printf("Invalid types: %d for operation %d on line %d.\n",
					node->children->exprType, node->nodeKind, node->lineNumber);
			}
			// code gen stuff
			// program is pointed at newest slot
			// need to issue unconditional jump to top of while loop
			// address needed is conviently already stored in this node
			s64 offset = node->attribute.s - c->programStart;
			*c->programStart = VM_OP_JUMP;
			c->programStart++;
			*c->programStart = (offset >> 8) & 0xFF;
			c->programStart++;
			*c->programStart = offset & 0xFF;
			c->programStart++;
			// we now have the true end of the while statement
			// fill in conditional jump
			AST *cJump = node->children->siblings;
			offset = c->programStart - cJump->attribute.s;
			cJump->attribute.s[1] = (offset >> 8) & 0xFF;
			cJump->attribute.s[2] = offset & 0xFF;
		}
		break;
		
		case ST_IF:
		{
			// type check
			if(node->children->exprType != S64_TYPE)
			{
				printf("Invalid types: %d for operation %d on line %d.\n",
					node->children->exprType, node->nodeKind, node->lineNumber);
			}
			// we now have the true end of the if statement
			// fill in conditional jump
			AST *cJump = node->children->siblings;
			s64 offset = c->programStart - cJump->attribute.s;
			cJump->attribute.s[1] = (offset >> 8) & 0xFF;
			cJump->attribute.s[2] = offset & 0xFF;
		}
		break;
		
		case ST_IFELSE:
		{
			// type check
			if(node->children->exprType != S64_TYPE)
			{
				printf("Invalid types: %d for operation %d on line %d.\n",
					node->children->exprType, node->nodeKind, node->lineNumber);
			}
			// we now have the true end of the if statement
			// fill in conditional jump
			AST *cJump = node->children->siblings;
			AST *uJump = cJump->siblings->siblings;
			s64 offset = (uJump->attribute.s+3) - cJump->attribute.s;
			cJump->attribute.s[1] = (offset >> 8) & 0xFF;
			cJump->attribute.s[2] = offset & 0xFF;
			// we can also fill in the unconditional jump
			if (uJump->identLen != 1){
				offset = c->programStart - uJump->attribute.s;
				uJump->attribute.s[1] = (offset >> 8) & 0xFF;
				uJump->attribute.s[2] = offset & 0xFF;
			}
		}
		break;
		
		case ST_DECL_VAR_ARRAY:
		if(node->children->exprType != S64_TYPE)
		{
			printf("Invalid types: %d for operation %d on line %d.\n",
				node->children->exprType, node->nodeKind, node->lineNumber);
		}
		
		*c->programStart = VM_OP_PUSH_LOCAL_ARRAY;
		c->programStart++;
		*c->programStart = node->identLen;
		c->programStart++;
		break;
		
		case ST_COMPOUND:
			stringListStack_leaveScope(c->stringLS);
			//printf("End of Block: Leaving Scope.\n");
		break;
		
		case ST_FUNC_DECL:
			stringListStack_leaveScope(c->stringLS);
			//printf("End of Function: Leaving Scope.\n");
		break;
		
		case ST_UJMP:
		{
			// code gen stuff
			if (*(c->programStart-1) == VM_OP_RETURN)
			{
				node->attribute.s = c->programStart-3;
				node->identLen = 1;
			} else {
				node->attribute.s = c->programStart;
				*c->programStart = VM_OP_JUMP;
				// reserve space for jump target
				c->programStart+=3;
			}
		}
		break;
		
		case ST_CJMP:
		{
			// code gen stuff
			node->attribute.s = c->programStart;
			*c->programStart = VM_OP_CJUMP;
			// reserve space for jump target
			c->programStart+=3;
		}
		break;
		
		default:
		break;
	}
	end: ;
}

/*******************************************************************************
 * Section AST Pass 1
*******************************************************************************/

static void
traverse(Context *c, AST *t)
{
	AST *stack[64];
	u64 index = 0;
	while(1) {
		while(1) {
			if (t == 0){break;}
			// pre-order functions section
			localVarsScoped(c, t);
			// end inorder functions section
			stack[index] = t;
			index++;
			t = t->children;
		}
		if (index == 0){break;}
		index--;
		t = stack[index];
		// inorder functions section
		typeCheckNode(c, t);
		// end inorder functions section
		t = t->siblings;
	}
}

/*******************************************************************************
 * Section Symbol Table Walk
*******************************************************************************/

static void
getArraySize(AST *node);

static void
getArraySize(AST *node)
{
	if (node->nodeKind == CONST_VAL)
	{
		// already good
	} else if (node->nodeKind == OP_PLUS) {
		node->attribute.i = node->children->attribute.i
			+ node->children->siblings->attribute.i;
	} else if (node->nodeKind == OP_MINUS) {
		node->attribute.i = node->children->attribute.i
			- node->children->siblings->attribute.i;
	} else if (node->nodeKind == OP_STAR) {
		node->attribute.i = node->children->attribute.i
			* node->children->siblings->attribute.i;
	} else if (node->nodeKind == OP_DIVI) {
		node->attribute.i = node->children->attribute.i
			/ node->children->siblings->attribute.i;
	} else {
		printf("wrong type in decl of global array\n");
	}
}

static void
traverseGetSizeArray(AST *t)
{
	AST *stack[64];
	u64 index = 0;
	while(1){
		while(1){
			if (t == 0){break;}
			/// pre-order functions section
			
			/// end inorder functions section
			stack[index] = t;
			index++;
			t = t->children;
		}
		if (index == 0){break;}
		index--;
		t = stack[index];
		/// inorder functions section
		getArraySize(t);
		/// end inorder functions section
		t = t->siblings;
	}
}

static u32
processGlobals1(hashTableNode *htNode, GlobalInfo *gi)
{
	AST *node = (AST *)htNode->value;
	
	switch (node->nodeKind)
	{
		case ST_GLOBAL_DECL_VAR:
		case ST_GLOBAL_DECL_VAR_ARRAY:
		{
		// load cursor position
		u64 *cursor = (u64 *)gi->buffp;
		// record offset
		node->attribute.i = cursor - (u64 *)gi->start;
		// init to 0
		*cursor = 0;
		// move cursor forward
		cursor++;
		// save new cursor position
		gi->buffp = (u8 *)cursor;
		}
		break;
		default:
		break;
	}
	return 0;
}

static u32
processGlobals2(hashTableNode *htNode, GlobalInfo *gi)
{
	AST *node = (AST *)htNode->value;
	
	switch (node->nodeKind)
	{
		case ST_GLOBAL_DECL_VAR_ARRAY:
		{
		// load cursor position
		u64 *cursor = (u64 *)gi->buffp;
		// determine the size by processing integer literals
		traverseGetSizeArray(node->children);
		// record offset of the start of array
		((u64 *)gi->start)[node->attribute.i] = cursor - (u64 *)gi->start;
		// node->children->attribute.i has the size or a failure announced
		// move cursor forward the size of the array
		cursor += node->children->attribute.i;
		// save new cursor position
		gi->buffp = (u8 *)cursor;
		}
		break;
		default:
		break;
	}
	return 0;
}

/*******************************************************************************
 * Section Grammar Tokens
*******************************************************************************/

static const char *const yyTokenName[] = { 
  /*    0 */ "$",
  /*    1 */ "COMMA",
  /*    2 */ "ASSIGN",
  /*    3 */ "EQUALS",
  /*    4 */ "NOTEQUALS",
  /*    5 */ "LT",
  /*    6 */ "GT",
  /*    7 */ "LTEQ",
  /*    8 */ "GTEQ",
  /*    9 */ "OR",
  /*   10 */ "XOR",
  /*   11 */ "AND",
  /*   12 */ "LBITSHIFT",
  /*   13 */ "RBITSHIFT",
  /*   14 */ "PLUS",
  /*   15 */ "MINUS",
  /*   16 */ "STAR",
  /*   17 */ "DIVI",
  /*   18 */ "MOD",
  /*   19 */ "NOT",
  /*   20 */ "TILDA",
  /*   21 */ "LPAREN",
  /*   22 */ "RPAREN",
  /*   23 */ "LBRACKET",
  /*   24 */ "RBRACKET",
  /*   25 */ "IDENT",
  /*   26 */ "SEMI",
  /*   27 */ "S64",
  /*   28 */ "VOID",
  /*   29 */ "LBLOCK",
  /*   30 */ "RBLOCK",
  /*   31 */ "IF",
  /*   32 */ "ELSE",
  /*   33 */ "WHILE",
  /*   34 */ "RETURN",
  /*   35 */ "INTEGER_LITERAL",
  /*   36 */ "program",
  /*   37 */ "declaration_list",
  /*   38 */ "declaration",
  /*   39 */ "var_declaration",
  /*   40 */ "fun_declaration",
  /*   41 */ "type_specifier",
  /*   42 */ "expression",
  /*   43 */ "params",
  /*   44 */ "compound_statement",
  /*   45 */ "param_list",
  /*   46 */ "param",
  /*   47 */ "statement_list",
  /*   48 */ "local_declarations",
  /*   49 */ "statement",
  /*   50 */ "expression_statement",
  /*   51 */ "selection_statement",
  /*   52 */ "iteration_statement",
  /*   53 */ "return_statement",
  /*   54 */ "assign_expression",
  /*   55 */ "var",
  /*   56 */ "call",
  /*   57 */ "arg_list",
};
#include "tool_output/tinyGram.h"
#include "tool_output/tinyLex.c"
#include "tool_output/tinyGram.c"


/*******************************************************************************
 * Section Initialization
*******************************************************************************/

static void
tinyInit(Context **c)
{
	Context *context = calloc(1, sizeof(Context));
	hashTable_init(&context->hashTable);
	stringListStack_init(&context->stringLS);
	context->scopedSymbols = malloc(256*sizeof(AST*));
	*c = context;
	return;
}

// Section Utility
#define HELP_MESSAGE \
	"tiny help:\n" \
	"--repl for read-eval-print-loop.\n" \
	"yourFileNameHere.tiny to run a file.\n"

void
printHelp(void)
{
	printf(HELP_MESSAGE);
}

static u8*
loadFile(u8 *file_name, u8 as_function)
{
	FILE *pFile;
	u8 *buffer;
	u64 fileSize, result;
	
	pFile = fopen(file_name, "rb");
	if (pFile==NULL) {
		fputs ("File error, cannot open source file\n",stderr);
		exit(1);
	}
	// obtain file size:
	fseek(pFile , 0 , SEEK_END);
	fileSize = ftell(pFile);
	rewind(pFile);
	// allocate memory to contain the whole file:
	buffer = malloc(fileSize+2);
	if (buffer == NULL) {
		fputs ("Memory error\n",stderr);
		exit(2);
	}
	// copy the file into the buffer:
	result = fread (buffer,1,fileSize,pFile);
	if (result != fileSize) {
		fputs ("Reading error\n",stderr);
		exit(3);
	}
	/* 0x00 terminate buffer, leave return in sub file */
	if (as_function){
		buffer[fileSize]=';';
		buffer[fileSize+1]=0;
	} else {
		buffer[fileSize]=0;
	}
	fclose(pFile);
	return buffer;
}


/*******************************************************************************
 * Section Command Line
*******************************************************************************/

static s32
tinyCommandLine(Context *c, s32 argc, u8 **argv)
{
	u8       buff[1024*64];
	Token    tokenData;
	LexInfo  lexInfo={0};
	void     *pEngine;     /* The LEMON-generated LALR(1) parser */
	yyParser sEngine;      /* Space to hold the Lemon-generated Parser object */
	GlobalInfo gi;
	gi.buffp = buff;
	gi.start = buff;
	
	if (argc <= 1)
	{
		printf("tiny supplied with no arguments, try --help.\n");
		return 0;
	}
	
	tokenData.s = 0;
	tokenData.lineNumber = 1;
	tokenData.attribute = 0;
	for (s32 i=1; i<argc; i+=1)
	{
		u32 x = tinyLexOptions(argv[i]);
		switch(x){
			case 0:
			return 0;
			case 1:
			printHelp();
			break;
			case 2:
			return 0;
			case 3:
			printf("target file: %s\n",argv[i]);
			sprintf(buff, "%s", argv[i]);
			c->fileName = buff;
			lexInfo.cursor = loadFile(buff, 0);
			
			#ifndef NDEBUG
			tinyParseTrace(stdout, "debug:: ");
			#endif
			// Set up parser
			pEngine = &sEngine;
			tinyParseInit(pEngine, c);
			
			printf("starting compile\n");
			// pass the code to the compiler
			do{
				// lex input, grabbing a single token
				lexInfo = tinyLex(
					lexInfo.cursor,
					&tokenData);
				
				// parse, a single token at a time
				tinyParse(
					pEngine,
					lexInfo.token,
					tokenData);
					
				// clear attribute data
				tokenData.attribute = 0;
				
				// check if finished
				if( lexInfo.token == 0){
					break;
				}
				
			} while(1);
			
			printf("Lexing and Parsing Complete\n");
			
			if (c->parseTree == 0 || c->errorPrinted)
			{
				return c->errorPrinted;
			}
			
			// at this point we have a parse tree
			//~ c->parseTree->attribute.s[c->parseTree->identLen] = 0;
			//~ printf("First Thing: %s\n\n\n", c->parseTree->attribute.s);
			
			// ready for code gen
			// set up globals first
			// first pass sets up all globals memory location.
			HASHTABLE_TRAVERSAL(c->hashTable, processGlobals1, &gi);
			// second pass lays out memory for the arrays and saves offsets
			HASHTABLE_TRAVERSAL(c->hashTable, processGlobals2, &gi);
			// each entry for the globals is populated with its address
			// now we have to code gen the executable code and output the byte
			// code
			c->programStart = gi.buffp;
			printf("size of globals in bytes: %ld.\n", gi.buffp - buff);
			
			// walk abstract syntax tree propigating types and doing type check-
			// ing while we are there we might as well generate the code?
			traverse(c, c->parseTree);
			
			if (c->stringLS->scopeIndex != 0)
			{
				printf("ERROR: Unbalanced scope detected.\n");
			}
			
			printf("size program in bytes: %ld.\n", c->programStart - gi.buffp);
			
			while(gi.buffp != c->programStart)
			{
				printf("instruction: %d\n", *gi.buffp);
				gi.buffp++;
			}
			
			Data returnData =
			VirtualMachine(
				(Data*)gi.start,
				(u8*) c->mainFunction->attribute.s,
				(Data *)((((u64)c->programStart)+7)/8*8),
				(Data *)((((u64)c->programStart)+7)/8*8),
				(Data)(s64)0  );
			printf("VirtualMachine returned %ld.\n", returnData.i);
			
			return c->errorPrinted;
		}
	}
		
	return 0;
}

/*******************************************************************************
 * Section Main
*******************************************************************************/

int main(int argc, char **argv)
{
	Context *c;
	tinyInit(&c);
	tinyCommandLine(c, argc, (u8**)argv);
	//~ printf("sizeof(YYMINORTYPE) = %ld\n", sizeof(YYMINORTYPE));
	//~ printf("sizeof(Token) = %ld\n", sizeof(Token));
	return 0;
}

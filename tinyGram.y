/*
 * grammar for a c-like language that is context free
*/

%include{
#define YYNOERRORRECOVERY 1
}
/* not active when YYNOERRORRECOVERY is defined */
/*%parse_failure {
	printf("parse_failure\n");
	p_s->error = 1;
}*/

%name tinyParse

%extra_context {Context *c}

%token_type   {Token}
%default_type {Token}


%left  COMMA.
%right ASSIGN.
%left  EQUALS NOTEQUALS LT GT LTEQ GTEQ.
%left  OR XOR AND.
%left  LBITSHIFT RBITSHIFT.
%left  PLUS MINUS.
%left  STAR DIVI MOD.
// DOLLAR ATSIGN
%right NOT TILDA.
%left  LPAREN RPAREN LBRACKET RBRACKET.

%syntax_error {
	u8 * start_of_line=TOKEN.s;
	u8 * end_of_line=TOKEN.s;
	
	if (!c->errorPrinted){
		c->errorPrinted=1;
		if ( (TOKEN.lineNumber) >1) {
			while (*start_of_line!='\n'){
				start_of_line--;
			}
			start_of_line++;
		}
		while (*end_of_line!='\n'){
				end_of_line++;
		}
		//end_of_line++;
		*end_of_line=0;
		
		// print error
		printf("%s:%d: syntax error on token %d:%s\n",
			c->fileName, (TOKEN.lineNumber), yymajor, yyTokenName[yymajor]);
		// print current stack
		printf("Current Stack:\n");
		yyStackEntry *stack_p = yypParser->yystack+1;
		while( stack_p<=yypParser->yytos ) {
			printf("[%s]",yyTokenName[stack_p->major]);
			stack_p++;
		}
		// print the offending token
		printf(">%s<",yyTokenName[yymajor]);
		printf("\n");
		// print the line of code
		printf("%s\n", start_of_line);
	}
	while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
}

program ::= declaration_list(A). {
	c->parseTree = A.ast;
	printf("parse done, input exhausted.\n");
}

declaration_list ::= declaration(A).
{
	if( (A.ast->nodeKind != ST_GLOBAL_DECL_VAR)
		&& (A.ast->nodeKind != ST_GLOBAL_DECL_VAR_ARRAY))
	{
		A.s = (u8 *)A.ast;
	} 
	else {
		AST *node = calloc(1, sizeof(AST));
		node->nodeKind  = 255;
		A.s = (u8 *)node;
	}
}
declaration_list ::= declaration_list(B) declaration(C).
{
	if( (C.ast->nodeKind != ST_GLOBAL_DECL_VAR)
		&& (C.ast->nodeKind != ST_GLOBAL_DECL_VAR_ARRAY))
	{
		// using .s field as extra spot. statement_list.ast keeps root.
		// link together as siblings
		((AST*)(B.s))->siblings = C.ast;
		// scoot over C so it will connect to following if any
		B.s = (u8 *)C.ast;
	}
}

declaration ::= var_declaration(A). // expression auto reduces
{
	// populate symbol table with global variable
	A.ast->attribute.s[A.ast->identLen] = 0;
	if(
		HashTable_insert_internal(
			c->hashTable,
			A.ast->attribute.s,
			A.ast->identLen,
			(u64)A.ast)
	)
	{
		printf("\"%s\" is being declared again on line %d.\n",
			A.ast->attribute.s, A.ast->lineNumber);
	}
	if (A.ast->nodeKind == ST_DECL_VAR)
	{
		A.ast->nodeKind = ST_GLOBAL_DECL_VAR;
	} else if (A.ast->nodeKind == ST_DECL_VAR_ARRAY) {
		A.ast->nodeKind = ST_GLOBAL_DECL_VAR_ARRAY;
	}
}
declaration ::= fun_declaration(A). // expression auto reduces
{
	// populate symbol table with global variable
	A.ast->attribute.s[A.ast->identLen] = 0;
	if (strcmp(
		(const char *)A.ast->attribute.s,
		"main") == 0)
	{
		c->mainFunction = A.ast;
	}
	if(
		HashTable_insert_internal(
			c->hashTable,
			A.ast->attribute.s,
			A.ast->identLen,
			(u64)A.ast)
	)
	{
		printf("\"%s\" is being declared again on line %d.\n",
			A.ast->attribute.s, A.ast->lineNumber);
	}
	// save off number of parameters
	A.ast->identLen = (u64)A.s;
}

var_declaration(A) ::= type_specifier(B) IDENT(C) SEMI.
{
	A.ast = statement(B.lineNumber);
	A.ast->nodeKind = ST_DECL_VAR;
	A.ast->attribute.s = C.s;
	A.ast->identLen = C.attribute;
	A.ast->exprType = B.attribute;
}
var_declaration(A) ::= type_specifier(B) IDENT(C) LBRACKET expression(D) RBRACKET SEMI.
{
	A.ast = statement(B.lineNumber);
	A.ast->children = D.ast;
	A.ast->nodeKind = ST_DECL_VAR_ARRAY;
	A.ast->attribute.s = C.s;
	A.ast->identLen = C.attribute;
	A.ast->exprType = B.attribute+2;
}

//type_specifier ::= S64|F64.// expression auto reduces
type_specifier ::= S64.// expression auto reduces

fun_declaration(A) ::= type_specifier(E) IDENT(B) LPAREN(F) params(C) RPAREN compound_statement(D).
{
	A.ast = statement(B.lineNumber);
	A.ast->children = C.ast;
	// link together as siblings
	((AST*)(C.s))->siblings = D.ast;
	A.ast->attribute.s = B.s;
	A.ast->identLen = B.attribute;
	A.ast->nodeKind = ST_FUNC_DECL;
	A.ast->exprType = E.attribute;
	D.ast->exprType = c->maxLocalStack;
	c->maxLocalStack = 0;
	A.s = (u8*)(u64)F.attribute;
}
fun_declaration(A) ::= VOID IDENT(B) LPAREN(F) params(C) RPAREN compound_statement(D).
{
	A.ast = statement(B.lineNumber);
	A.ast->children = C.ast;
	// link together as siblings
	((AST*)(C.s))->siblings = D.ast;
	A.ast->attribute.s = B.s;
	A.ast->identLen = B.attribute;
	A.ast->nodeKind = ST_FUNC_DECL;
	A.ast->exprType = VOID_TYPE;
	D.ast->exprType = c->maxLocalStack;
	c->maxLocalStack = 0;
	A.s = (u8*)(u64)F.attribute;
}

params ::= param_list.
params(A) ::= VOID(B).
{
	A.ast = statement(B.lineNumber);/*A-overwrites-B*/
	A.ast->nodeKind = ST_PARAMETER;
	A.ast->exprType = VOID_TYPE;
	A.s = (u8 *)A.ast;
	yymsp[-1].minor.yy0.attribute = 0;
}

param_list ::= param(A).
{
	A.s = (u8 *)A.ast;
	yymsp[-1].minor.yy0.attribute = 1;
}
param_list ::= param_list(B) COMMA param(C).
{
	// using .s field as extra spot. statement_list.ast keeps root.
	// link together as siblings
	((AST*)(B.s))->siblings = C.ast;
	// scoot over C so it will connect to following if any
	B.s = (u8 *)C.ast;
	yymsp[-3].minor.yy0.attribute += 1;
}

param(A) ::= type_specifier(B) IDENT(C).
{
	A.ast = statement(B.lineNumber);
	A.ast->nodeKind = ST_PARAM_VAR;
	A.ast->attribute.s = C.s;
	A.ast->identLen = C.attribute;
	A.ast->exprType = B.attribute;
}
param(A) ::= type_specifier(B) IDENT(C) LBRACKET RBRACKET.
{
	A.ast = statement(B.lineNumber);
	A.ast->nodeKind = ST_PARAM_VAR_ARRAY;
	A.ast->attribute.s = C.s;
	A.ast->identLen = C.attribute;
	A.ast->exprType = B.attribute+2;
}

compound_statement ::= LBLOCK RBLOCK.
//  not a useful parse
//~ compound_statement(A) ::= LBLOCK local_declarations(B) RBLOCK.
//~ {
	//~ A.ast = statement(B.lineNumber);
	//~ A.ast->children = B.ast;
	//~ A.ast->nodeKind = ST_COMPOUND;
//~ }
compound_statement(A) ::= LBLOCK statement_list(B) RBLOCK.
{
	A.ast = statement(B.lineNumber);
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_COMPOUND;
}
compound_statement(A) ::= LBLOCK(D) local_declarations(B) statement_list(C) RBLOCK.
{
	A.ast = statement(B.lineNumber);
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_COMPOUND;
	((AST*)(B.s))->siblings = C.ast;
	if(c->numLocalDecls > c->maxLocalStack){c->maxLocalStack=c->numLocalDecls;}
	c->numLocalDecls -= D.attribute;
}

local_declarations ::= var_declaration(A).
{
	A.s = (u8 *)A.ast;
	c->numLocalDecls += 1;
	yymsp[-1].minor.yy0.attribute += 1;
}
local_declarations ::= local_declarations(B) var_declaration(C).
{
	// using .s field as extra spot. statement_list.ast keeps root.
	// link together as siblings
	((AST*)(B.s))->siblings = C.ast;
	// scoot over C so it will connect to following if any
	B.s = (u8 *)C.ast;
	c->numLocalDecls += 1;
	yymsp[-2].minor.yy0.attribute += 1;
}

statement_list ::= statement(A).
{
	A.s = (u8 *)A.ast;
}
statement_list ::= statement_list(B) statement(C).
{
	// using .s field as extra spot. statement_list.ast keeps root.
	// link together as siblings
	((AST*)(B.s))->siblings = C.ast;
	// scoot over C so it will connect to following if any
	B.s = (u8 *)C.ast;
}

statement ::= expression_statement.  // expression auto reduces
statement ::= compound_statement.  // expression auto reduces
statement ::= selection_statement.  // expression auto reduces
statement ::= iteration_statement.  // expression auto reduces
statement ::= return_statement.  // expression auto reduces

expression_statement(A) ::= SEMI.
{
	//A.ast = expression(A.lineNumber, OP_EMPTY);
	A.ast = statement(A.lineNumber);
	A.ast->nodeKind = ST_EXPR;
}
expression_statement ::= expression(A) SEMI.  // expression auto reduces
{
	if (A.ast->nodeKind == FUNC_CALL)
	{
		A.ast->nodeKind = FUNC_CALL_RETURN_IGNORED;
	}
}
expression_statement ::= assign_expression SEMI.  // expression auto reduces

selection_statement(A) ::= IF(F) LPAREN expression(B) RPAREN compound_statement(C).
{
	A.ast = statement(F.lineNumber);
	AST *node = calloc(1, sizeof(AST));
	node->nodeKind  = ST_CJMP;
	node->siblings  = C.ast;
	B.ast->siblings = node;
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_IF;
}
selection_statement(A) ::= IF(F) LPAREN expression(B) RPAREN compound_statement(C) ELSE statement(D).
{
	A.ast = statement(F.lineNumber);
	AST *node = calloc(1, sizeof(AST));
	node->nodeKind  = ST_CJMP;
	node->siblings  = C.ast;
	B.ast->siblings = node;
	node = calloc(1, sizeof(AST));
	node->nodeKind  = ST_UJMP;
	node->siblings  = D.ast;
	C.ast->siblings = node;
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_IFELSE;
}

iteration_statement(A) ::= WHILE(F) LPAREN expression(B) RPAREN compound_statement(C).
{
	A.ast = statement(F.lineNumber);
	AST *node = calloc(1, sizeof(AST));
	node->nodeKind  = ST_CJMP;
	node->siblings  = C.ast;
	B.ast->siblings = node;
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_WHILE;
}

return_statement(A) ::= RETURN SEMI.
{
	A.ast = statement(A.lineNumber);
	A.ast->nodeKind = ST_RETURN;
}
return_statement(A) ::= RETURN expression(B) SEMI.
{
	A.ast = statement(A.lineNumber);
	A.ast->children = B.ast;
	A.ast->nodeKind = ST_RETURN;
}

assign_expression(A) ::= var(B) ASSIGN(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	// swap
	D.ast->siblings = B.ast;
	A.ast->children = D.ast;
}

expression(A) ::= expression(B) EQUALS(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) NOTEQUALS(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) LT(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) GT(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) LTEQ(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) GTEQ(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}

expression(A) ::= expression(B) OR(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) XOR(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) AND(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}

expression(A) ::= expression(B) LBITSHIFT(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) RBITSHIFT(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}

expression(A) ::= expression(B) PLUS(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) MINUS(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}

expression(A) ::= expression(B) STAR(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) DIVI(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}
expression(A) ::= expression(B) MOD(C) expression(D).
{
	A.ast = expression(C.lineNumber,yymsp[-1].major);
	B.ast->siblings = D.ast;
	A.ast->children = B.ast;
}

expression(A) ::= MINUS(C) expression. [NOT]
{
	A.ast = expression(C.lineNumber,OP_NEG); /*A-overwrites-B*/
	A.ast->children = C.ast;
}
expression(A) ::= NOT(C) expression.
{
	A.ast = expression(C.lineNumber,yymsp[-1].major); /*A-overwrites-B*/
	A.ast->children = C.ast;
}
expression(A) ::= TILDA(C) expression.
{
	A.ast = expression(C.lineNumber,yymsp[-1].major); /*A-overwrites-B*/
	A.ast->children = C.ast;
}


expression(A) ::= LPAREN expression(B) RPAREN.
{
	A=B;
}
//~ expression(A) ::= type_specifier(C) LPAREN expression(B) RPAREN.
//~ {
	//~ A.ast = expression(C.lineNumber,TYPE_CAST);
	//~ A.ast->exprType = C.attribute;
	//~ A.ast->children = B.ast;
//~ }
expression ::= var. // expression auto reduces
expression ::= call. // expression auto reduces
expression(A) ::= INTEGER_LITERAL(B).
{
	A.ast = intConstant(B, B.lineNumber); /*A-overwrites-B*/
}
//~ expression(A) ::= FLOAT_LITERAL(B).
//~ {
	//~ A.ast = floatConstant(B, B.lineNumber); /*A-overwrites-B*/
//~ }

var(A) ::= IDENT(B).
{
	A.ast = indentifierExpr(B, B.lineNumber); /*A-overwrites-B*/
}
var(A) ::= IDENT(B) INCREMENT.
{
	A.ast = indentifierIncExpr(B, B.lineNumber); /*A-overwrites-B*/
}
var(A) ::= IDENT(B) LBRACKET expression(C) RBRACKET.
{
	A.ast = indentifierArrayExpr(B, B.lineNumber); /*A-overwrites-B*/
	A.ast->children = C.ast;
}

call(A) ::= IDENT(B) LPAREN RPAREN.
{
	A.ast = indentifierFuncExpr(B, B.lineNumber); /*A-overwrites-B*/
}
call(A) ::= IDENT(B) LPAREN arg_list(C) RPAREN.
{
	A.ast = indentifierFuncExpr(B, B.lineNumber); /*A-overwrites-B*/
	A.ast->children = C.ast;
}

arg_list ::= expression(A).
{
	A.s = (u8 *)A.ast;
}
arg_list ::= arg_list(B) COMMA expression(C). // chain together
{
	// link together as siblings
	((AST*)(B.s))->siblings = C.ast;
	// scoot over C so it will connect to following if any
	B.s = (u8 *)C.ast;
}



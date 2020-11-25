/* tinyLex.re */

/*!max:re2c*/                        // directive that defines YYMAXFILL
/*!re2c                              // start of re2c block
	
	// comments and white space
	scm = "//" [^\n\x00]* "\n";
	wsp = ([ \r\n\t] | scm )+;

	// integer literals
	oct = "0" [0-7]*;
	dec = [1-9][0-9]*;
	hex = "0x" [0-9A-F]+; // a-f removed
	integer = "-"? (oct | dec | hex);

	// floating literals
	frc = [0-9]* "." [0-9]+ | [0-9]+ ".";
	exp = 'e' [+-]? [0-9]+;
	flt = "-"? (frc exp? | [0-9]+ exp) [fFlL]?;

	// string literals
	string_lit = ["] ([^"\x00] | ([\\] ["]))* ["];
	string_lit_chain = ([^"\n] | ([\\] ["]))* "\n";
	string_lit_end = ([^"\n] | ([\\] ["]))* ["];
	
	// character literals
	char_lit = ['] ([^'\\\x00] | ([\\] ['nrt\\])) ['];
	
	identifier = [a-zA-Z_][a-zA-Z_0-9]*;
	
*/                                   // end of re2c block


static s32
tinyLexOptions(u8 *YYCURSOR)
{
	u8 *YYMARKER;
	u8 *start;

	start = YYCURSOR;

	/*!re2c                            // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;

	* { printf("Invalid Option: %s, try --help.\n",start); return 0; }
	[\x00] { printf("Invalid Option: NULL try --help.\n"); return 0; }
	
	"--help" {
		return 1;
	}
	
	"--repl" {
		return 2;
	}
	
	[a-zA-Z_/0-9] [a-zA-Z_/0-9-]* ".tiny" {
		return 3;
	}
	
	*/                                  // end of re2c block
}

static void
compactStrings(u8 *YYCURSOR)
{
	//u8 * YYCURSOR;
	u8 *start;
	u64 length=0;
	u8 *out = YYCURSOR;
	//YYCURSOR = *YYCURSORp;
	//startMangledString = out;
	

loop: // label for looping within the lexxer
	start = YYCURSOR;

	/*!re2c                          // start of re2c block **/
	re2c:define:YYCTYPE = "u8";
	re2c:yyfill:enable  = 0;
	
	* { goto loop; }
	
	string_lit_chain {
		// part of multi-line string literal
		// subtract 1 from length to remove newline
		length = YYCURSOR-start-1;
		// move string into position(first part of chain is moved wastefully)
		memmove(out, start, length);
		out+=length;
		// skip starting tabs to allow formatting
		while(*YYCURSOR=='\t'){YYCURSOR++;}
		goto loop;
	}
	
	string_lit_end {
		// end of potentially multi-line string
		// if the start has been unchanged, we are regular string
		if(length==0)
		{
			// null terminate and return
			*(YYCURSOR-1) = 0;
			// length including ending null
			//length = YYCURSOR-start;
			// copy string out
			//memmove(out, start, length);
			//out+=length;
			return;
		}
		*(YYCURSOR-1) = 0;
		// length including ending null
		length = YYCURSOR-start;
		// move string into position
		memmove(out, start, length);
		out+=length;
		return;
	}

	*/                               // end of re2c block
}

// reader
// returns single token OR list of tokens
// lisp requires garbage collection, therefore a "stack" cannot be used
// as a scratchpad __attribute__ ((noinline))
static  LexInfo 
tinyLex(u8 *YYCURSOR, Token *t) 
{
	u8 *YYMARKER;
	u8 *start;
	LexInfo lexInfo;//={0};


loop: // label for looping within the lexxer

	start = YYCURSOR;

	/*!re2c                          // start of re2c block **/
	re2c:define:YYCTYPE = "u8";      //   configuration that defines YYCTYPE
	re2c:yyfill:enable  = 0;         //   configuration that turns off YYFILL

	* { 
		u8 *s=start;
		u8 *f=YYCURSOR;
		while (*s!='\n'){
			s--;
		}
		s++;
		while (*f!='\n'){
			f++;
		}
		f++;
		printf("lex failure ");
		for (u32 ss=0; ss <(f-s); ss++){
			printf("%c", s[ss]);
		}
		printf("\n");
		goto loop;
	}

	[\x00] {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = 0;
		return lexInfo;
	}

	wsp {
		u32 lineNumber = t->lineNumber;
		do {
			if(*start=='\n'){
				lineNumber++;
			}
			start++;
		} while (start!=YYCURSOR);
		t->lineNumber = lineNumber;
		goto loop;
	}
 
	integer {
		t->s = start;
		t->attribute = S64_TYPE;
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = INTEGER_LITERAL;
		return lexInfo;
	}
	
	//~ flt {
		//~ t->s = start;
		//~ t->attribute = F64_TYPE;
		//~ lexInfo.cursor = YYCURSOR;
		//~ lexInfo.token = FLOAT_LITERAL;
		//~ return lexInfo;
	//~ }
	
	//~ string_lit {
		//~ *YYCURSORp = YYCURSOR;
		//~ start++;
		//~ t->s = start;
		//~ // concatenate all multiline strings
		//~ compactStrings(start);
		//~ return STRING_LITERAL;
	//~ }

	//~ char_lit {
		//~ *YYCURSORp = YYCURSOR;
		//~ start++;
		//~ switch(*start)
		//~ {
			//~ case '\\':
			//~ switch(*(start+1))
			//~ {
				//~ case '\'':
				//~ t->l = 0x27;
				//~ break;
				//~ case 'n':
				//~ t->l = 0x0A;
				//~ break;
				//~ case 'r':
				//~ t->l = 0x0D;
				//~ break;
				//~ case 't':
				//~ t->l = 0x09;
				//~ break;
				//~ case '\\':
				//~ t->l = 0x5C;
				//~ break;
			//~ }
			//~ break;
			//~ default:
			//~ t->l = *start;
			//~ break;
		//~ }
		//~ return CHAR_LITERAL;
	//~ }

	// Operators
	"+" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = PLUS;
		return lexInfo;
	}
	
	"-" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = MINUS;
		return lexInfo;
	}
	
	"*" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = STAR;
		return lexInfo;
	}
	
	"/" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = DIVI;
		return lexInfo;
	}
	
	"%" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = MOD;
		return lexInfo;
	}

	"&" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = AND;
		return lexInfo;
	}

	"^" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = XOR;
		return lexInfo;
	}

	"|" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = OR;
		return lexInfo;
	}
	
	"<<" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LBITSHIFT;
		return lexInfo;
	}

	">>" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = RBITSHIFT;
		return lexInfo;
	}

	"~" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = TILDA;
		return lexInfo;
	}

	//~ "sizeof" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return SIZEOF;
	//~ }

	// Assignment
	"="   {lexInfo.cursor = YYCURSOR;lexInfo.token = ASSIGN;return lexInfo;}
	//~ "*="  {*YYCURSORp = YYCURSOR; return  MULASSIGN;}
	//~ "/="  {*YYCURSORp = YYCURSOR; return  DIVASSIGN;}
	//~ "%="  {*YYCURSORp = YYCURSOR; return  MODASSIGN;}
	//~ "+="  {*YYCURSORp = YYCURSOR; return  ADDASSIGN;}
	//~ "-="  {*YYCURSORp = YYCURSOR; return  SUBASSIGN;}
	//~ "<<=" {*YYCURSORp = YYCURSOR; return  LSHASSIGN;}
	//~ ">>=" {*YYCURSORp = YYCURSOR; return  RSHASSIGN;}
	//~ "&="  {*YYCURSORp = YYCURSOR; return  ANDASSIGN;}
	//~ "^="  {*YYCURSORp = YYCURSOR; return  XORASSIGN;}
	//~ "|="  {*YYCURSORp = YYCURSOR; return  ORASSIGN;}

	// Comparison
	"==" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = EQUALS;
		return lexInfo;
	}

	"!=" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = NOTEQUALS;
		return lexInfo;
	}

	">" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = GT;
		return lexInfo;
	}

	"<" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LT;
		return lexInfo;
	}

	"<=" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LTEQ;
		return lexInfo;
	}

	">=" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = GTEQ;
		return lexInfo;
	}

	// Logic
	"!" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = NOT;
		return lexInfo;
	}
	
	"++" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = INCREMENT;
		return lexInfo;
	}

	//~ "||" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return LOGIC_OR;
	//~ }

	//~ "&&" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return LOGIC_AND;
	//~ }

	
	//~ "." {
		//~ *YYCURSORp = YYCURSOR;
		//~ return DOT;
	//~ }

	"(" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LPAREN;
		return lexInfo;
	}
	
	")" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = RPAREN;
		return lexInfo;
	}
	
	"{" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LBLOCK;
		return lexInfo;
	}
	
	"}" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = RBLOCK;
		return lexInfo;
	}

	"[" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = LBRACKET;
		return lexInfo;
	}
	
	"]" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = RBRACKET;
		return lexInfo;
	}


	//~ ":" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return COLON;
	//~ }
	
	";" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = SEMI;
		return lexInfo;
	}
	
	//~ "@" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return ATSIGN;
	//~ }
	
	//~ "$" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return DOLLAR;
	//~ }

	// Control
	"if" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = IF;
		return lexInfo;
	}
	"else" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = ELSE;
		return lexInfo;
	}
	"while" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = WHILE;
		return lexInfo;
	}
	//~ "switch"   {*YYCURSORp = YYCURSOR; return SWITCH ;}
	//~ "do"       {*YYCURSORp = YYCURSOR; return DO;}
	//~ "goto"     {*YYCURSORp = YYCURSOR; return GOTO ;}
	//~ "continue" {*YYCURSORp = YYCURSOR; return CONTINUE ;}
	//~ "break"    {*YYCURSORp = YYCURSOR; return BREAK ;}
	"return" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = RETURN;
		return lexInfo;
	}

	// Types
	//~ "u8" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return U8;
	//~ }

	"s64" {
		t->attribute = S64_TYPE;
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = S64;
		return lexInfo;
	}

	//~ "f64" {
		//~ t->attribute = F64_TYPE;
		//~ lexInfo.cursor = YYCURSOR;
		//~ lexInfo.token = F64;
		//~ return lexInfo;
	//~ }

	"void" {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = VOID;
		return lexInfo;
	}
	
	//~ "inline" {*YYCURSORp = YYCURSOR; return INLINE ;}

	//~ "const" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return CONST;
	//~ }
	
	//~ "struct" {
		//~ *YYCURSORp = YYCURSOR;
		//~ return STRUCT;
	//~ }
	
	//~ "..." {*YYCURSORp = YYCURSOR;return DOTDOTDOT;}
	"," {
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = COMMA;
		return lexInfo;
	}

	identifier {
		t->s = start;
		t->attribute = (u32)(YYCURSOR - start);
		//~ if( (*start=='s') && ((*(start+1)>='A') && (*(start+1)<='Z')) )
		//~ {
			//~ return S_ID;
		//~ }
		lexInfo.cursor = YYCURSOR;
		lexInfo.token = IDENT;
		return lexInfo;
	}

	*/                               // end of re2c block
}  




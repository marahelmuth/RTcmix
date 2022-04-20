/* RTcmix  - Copyright (C) 2004  The RTcmix Development Team
   See ``AUTHORS'' for a list of contributors. See ``LICENSE'' for
   the license to this software and for a DISCLAIMER OF ALL WARRANTIES.
*/
%{
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "rename.h"
#include "bison_version.h"
#include "minc_internal.h"
#include "Node.h"
#include "Symbol.h"
#include "lex.yy.c"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef EMBEDDED
double minc_memflush();
#else
// in args.cpp
const char *lookup_token(const char *token, bool printWarning);
#endif
#ifdef __cplusplus
}
#endif
#undef MDEBUG	/* turns on yacc debugging below */

#ifdef MDEBUG
//int yydebug=1;
#define MPRINT(x) rtcmix_print("YACC: %s\n", x)
#define MPRINT1(x,y) rtcmix_print("YACC: " x "\n", y)
#define MPRINT2(x,y,z) rtcmix_print("YACC: " x "\n", y, z)
#else
#define MPRINT(x)
#define MPRINT1(x,y)
#define MPRINT2(x,y,z)
#endif

#undef YYDEBUG
#define YYDEBUG 1
#define MAXTOK_IDENTLIST 200
#define TRUE 1
#define FALSE 0

static Node *	program;
static int		idcount = 0;
static char		*idlist[MAXTOK_IDENTLIST];  
static int		flerror;		/* set if there was an error during parsing */
static int		level = 0;		/* keeps track whether we are in a sub-block */
static int		flevel = 0;		/* > 0 if we are in a function decl block */
static int      slevel = 0;     /* > 0 if we are in a struct decl block */
static int      xblock = 0;		/* 1 if we are entering a block preceeded by if(), else(), while(), or for() */
static bool     preserve_symbols = false;   /* what to do with symbol table at end of parse */
static void 	cleanup();
static void 	incrLevel();
static void		decrLevel();
static Node * declare(MincDataType type);
static Node * declareStruct(const char *typeName);
static Node * parseArgumentQuery(const char *text, int *pOutErr);
static Node * parseScoreArgument(const char *text, int *pOutErr);
static Node * go(Node * t1);

%}

%left  <ival> LOWPRIO
%left  <ival> '='
%left  <ival> TOK_PLUSPLUS
%left  <ival> TOK_MINUSMINUS
%left  <ival> TOK_MINUSEQU
%left  <ival> TOK_PLUSEQU
%left  <ival> TOK_DIVEQU
%left  <ival> TOK_MULEQU
%left  <ival> TOK_OR
%left  <ival> TOK_AND
%left  <ival> TOK_EQU TOK_UNEQU
%left  <ival> '<' '>' TOK_LESSEQU TOK_GTREQU
%left  <ival> '+' '-'
%left  <ival> '*' '/'
%left  <ival> TOK_POW
%left  <ival> CASTTOKEN
%token <ival> TOK_STRUCT_DECL
%token <ival> TOK_FLOAT_DECL
%token <ival> TOK_STRING_DECL
%token <ival> TOK_HANDLE_DECL
%token <ival> TOK_LIST_DECL
%token <ival> TOK_MAP_DECL
%token <ival> TOK_FUNC_DECL;
%token <ival> TOK_IDENT TOK_NUM TOK_ARG_QUERY TOK_ARG TOK_NOT TOK_IF TOK_ELSE TOK_FOR TOK_WHILE TOK_RETURN
%token <ival> TOK_TRUE TOK_FALSE TOK_STRING '{' '}'
%type  <node> stml stmt rstmt bexp expl exp str ret bstml obj fexp fexpl func
%type  <node> fdecl sdecl hdecl ldecl mdecl structdecl funcdecl arg argl fdef fstml fargl funcname mbr mbrl structdef
%type  <str> id structname

%destructor { MPRINT1("yydestruct deleting node %p\n", $$); delete $$; } stml stmt rstmt bexp expl exp str ret bstml fdecl sdecl hdecl ldecl mdecl structdecl funcdecl fdef fstml arg argl fargl funcname mbr mbrl structdef obj fexp fexpl

%error-verbose

%%
/* program (the "start symbol") */
prg:	| stml				{ MPRINT("prg:"); program = $1; cleanup(); return 0; }
	;
 
/* statement list */
stml:	stmt				{ MPRINT("stml:	stmt"); $$ = $1; }
	| stmt ';'				{ MPRINT("stml:	stmt;"); $$ = $1; }
	| stml stmt				{ MPRINT("stml:	stml stmt"); $$ = new NodeSeq($1, $2); }
	| stml stmt ';'			{ MPRINT("stml:	stml stmt;"); $$ = new NodeSeq($1, $2); }
	;

/* statement */
stmt: rstmt					{ MPRINT("rstmt");	$$ = go($1); }
	| fdecl
	| sdecl
	| hdecl
	| ldecl
    | mdecl
    | structdecl
    | funcdecl
	| TOK_IF level bexp stmt {	xblock = 1; MPRINT("IF bexp stmt");
								decrLevel();
								$$ = go(new NodeIf($3, $4));
								xblock = 0;
							}
	| TOK_IF level bexp stmt TOK_ELSE stmt { xblock = 1;
								decrLevel();
								$$ = go(new NodeIfElse($3, $4, $6));
								xblock = 0;
							}
	| TOK_WHILE level bexp stmt	{ xblock = 1;	MPRINT("WHILE bexp stmt");
								decrLevel();
								$$ = go(new NodeWhile($3, $4));
								xblock = 0;
							}
	| TOK_FOR level '(' stmt ';' bexp ';' stmt ')' stmt { xblock = 1;
								decrLevel();
								$$ = go(new NodeFor($4, $6, $8, $10));
								xblock = 0;
							}
	| bstml
	| fdef
	| ret
    | structdef
	| error TOK_FLOAT_DECL	{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_STRING_DECL	{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_HANDLE_DECL	{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_LIST_DECL	{ flerror = 1; $$ = new NodeNoop(); }
    | error TOK_MAP_DECL    { flerror = 1; $$ = new NodeNoop(); }
    | error TOK_FUNC_DECL    { flerror = 1; $$ = new NodeNoop(); }
	| error TOK_IF		{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_WHILE	{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_FOR	{ flerror = 1; $$ = new NodeNoop(); }
	| error '{'		{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_ELSE	{ flerror = 1; $$ = new NodeNoop(); }
	| error TOK_RETURN	{ flerror = 1; $$ = new NodeNoop(); }
	| error ';'			{ flerror = 1; $$ = new NodeNoop(); }
	;

/* block statement list
   This is tricky because we want to bump 'level' here (to avoid executing the contents of the list
   until the block is completely created) but we don't want to bump it twice for <if bexp bstml>.  So,
   if/else/while/for statements set xblock to 1 indicating the bump has already been done.  Only a 
   stand-alone block statement will do its own bump.
 */

bstml:	'{'			{ if (!xblock) incrLevel(); }
		stml
		'}'			{ 	MPRINT("bstml: { stml }"); MPRINT2("level = %d, xblock = %d", level, xblock);
									if (!xblock) { decrLevel(); }
									$$ = go(new NodeBlock($3));
								}
	/* DAS handling empty block as special case to avoid need for empty stml's */
	| '{''}'		{ 	MPRINT("bstml: {}");
								$$ = go(new NodeBlock(new NodeNoop()));
								}
	;

/* A return statement.  Only used inside functions. */

ret: TOK_RETURN exp			{	MPRINT("ret exp");
								MPRINT1("called at level %d", level);
								if (flevel == 0) {
									minc_die("return statements not allowed in main score");
									$$ = new NodeNoop();
								}
								else {
									$$ = new NodeRet($2);
								}
							}
	| TOK_RETURN exp ';'	{	MPRINT("ret exp;");
								MPRINT1("called at level %d", level);
								if (flevel == 0) {
									minc_die("return statements not allowed in main score");
									$$ = new NodeNoop();
								}
								else {
									$$ = new NodeRet($2);
								}
							}
	;

/* variable declaration lists */

fdecl:	TOK_FLOAT_DECL idl	{ 	MPRINT("fdecl");
								$$ = go(declare(MincFloatType));
								idcount = 0;
							}	// e.g., "float x, y z"
	;
sdecl:	TOK_STRING_DECL idl	{ 	MPRINT("sdecl");
								$$ = go(declare(MincStringType));
								idcount = 0;
							}
	;
hdecl:	TOK_HANDLE_DECL idl	{ 	MPRINT("hdecl");
								$$ = go(declare(MincHandleType));
								idcount = 0;
							}
	;
ldecl:	TOK_LIST_DECL idl	{ 	MPRINT("ldecl");
								$$ = go(declare(MincListType));
								idcount = 0;
							}
    ;
mdecl:    TOK_MAP_DECL idl    {     MPRINT("mdecl");
                                $$ = go(declare(MincMapType));
                                idcount = 0;
                              }
    ;
structdecl: TOK_STRUCT_DECL id idl    {     MPRINT("structdecl");
                                            $$ = go(declareStruct($2));
                                            idcount = 0;
                                        }
    ;
funcdecl:    TOK_FUNC_DECL idl    {     MPRINT("funcdecl");
                                            $$ = go(declare(MincFunctionType));
                                            idcount = 0;
                                        }
    ;

/* statement nesting level counter */
level:  /* nothing */ { incrLevel(); }
	;

/* An obj is anything that can be operator accessed via . or [] */
obj:    id                  {       MPRINT("obj: id");          $$ = new NodeLoadSym($1); }
    |   obj'.'id            {       MPRINT("obj: obj.id");      $$ = new NodeMember($1, $3);  }
    |   obj'[' exp ']'      {       MPRINT("obj: obj[exp]");    $$ = new NodeSubscriptRead($1, $3); }     // This is always non-terminal therefor read-access only
    ;

func:   id                  {       MPRINT("func: id");         $$ = new NodeLoadFuncSym($1); }
    |   obj'.'id            {       MPRINT("func: obj.id");     $$ = new NodeMember($1, $3); }
    |   obj'[' exp ']'      {       MPRINT("func: obj[exp]");   $$ = new NodeSubscriptRead($1, $3); }
    ;

/* function expression */
fexp:   exp             { MPRINT("fexp: exp"); $$ = $1; }
    |   bexp            { MPRINT("fexp: bexp"); $$ = $1; }
;

/* function expression list */
fexpl:  fexp            { MPRINT("fexpl: fexp"); $$ = new NodeListElem(new NodeEmptyListElem(), $1); }
    |   fexpl ',' fexp  {  MPRINT("fexpl: fexpl,fexp"); $$ = new NodeListElem($1, $3); }
;

/* statement returning a value: assignments, function calls, etc. */
rstmt: id '=' exp		{ MPRINT("rstmt: id = exp");		$$ = new NodeStore(new NodeAutoDeclLoadSym($1), $3); }
	| id TOK_PLUSEQU exp {		$$ = new NodeOpAssign(new NodeLoadSym($1), $3, OpPlus); }
	| id TOK_MINUSEQU exp {		$$ = new NodeOpAssign(new NodeLoadSym($1), $3, OpMinus); }
	| id TOK_MULEQU exp {		$$ = new NodeOpAssign(new NodeLoadSym($1), $3, OpMul); }
	| id TOK_DIVEQU exp {		$$ = new NodeOpAssign(new NodeLoadSym($1), $3, OpDiv); }

    | TOK_PLUSPLUS id %prec CASTTOKEN {
        $$ = new NodeOpAssign(new NodeLoadSym($2), new NodeConstf(1.0), OpPlusPlus);
    }
    | TOK_MINUSMINUS id %prec CASTTOKEN {
        $$ = new NodeOpAssign(new NodeLoadSym($2), new NodeConstf(1.0), OpMinusMinus);
    }

    | func '(' fexpl ')' {	MPRINT("rstmt: func(fexpl)");
								$$ = new NodeCall($1, $3);
							}
	| func '(' ')' {			MPRINT("rstmt: func()");
								$$ = new NodeCall($1, new NodeEmptyListElem());
							}
	| obj '[' exp ']' '=' exp {
                                MPRINT("rstmt: obj[exp] = exp");
                                $$ = new NodeSubscriptWrite($1, $3, $6);
                            }
    | obj'.'id '=' exp       {
                                MPRINT("rstmt: obj.id = exp");
                                $$ = new NodeStore(new NodeMember($1, $3), $5);
                            }
	;

/* identifier list */
idl: id					{ MPRINT("idl: id"); idlist[idcount++] = $1; }
	| idl ',' id		{ MPRINT("idl: idl,id"); idlist[idcount++] = $3; }
	;

/* identifier */
id:  TOK_IDENT			{ MPRINT("id"); $$ = strsave(yytext); }
	;

/* expression list */
expl:	exp				{ MPRINT("expl: exp"); $$ = new NodeListElem(new NodeEmptyListElem(), $1); }
	| expl ',' exp		{ MPRINT("expl: expl,exp"); $$ = new NodeListElem($1, $3); }
	;

/* string */
str:	TOK_STRING		{
								char *s = yytext + 1;
								s[strlen(s) - 1] = '\0';
								$$ = new NodeString(strsave(s));
							}
	;

/* Boolean expression, before being wrapped in () */
bexp:	exp %prec LOWPRIO	{ MPRINT("bexp"); $$ = $1; }
	| TOK_NOT bexp %prec TOK_UNEQU { MPRINT("!bexp"); $$ = new NodeNot($2); }
	| bexp TOK_AND bexp	{ $$ = new NodeAnd($1, $3); }
	| bexp TOK_OR  bexp	{ $$ = new NodeOr($1, $3); }
	| bexp TOK_EQU bexp	{ $$ = new NodeRelation(OpEqual, $1, $3); }
	| exp TOK_UNEQU exp	{ $$ = new NodeRelation(OpNotEqual, $1, $3); }
	| exp '<' exp			{ $$ = new NodeRelation(OpLess, $1, $3); }
	| exp '>' exp			{ $$ = new NodeRelation(OpGreater, $1, $3); }
	| exp TOK_LESSEQU exp { $$ = new NodeRelation(OpLessEqual, $1, $3); }
	| exp TOK_GTREQU exp	{ $$ = new NodeRelation(OpGreaterEqual, $1, $3); }
	| TOK_TRUE				{ $$ = new NodeRelation(OpEqual, new NodeConstf(1.0), new NodeConstf(1.0)); }
	| TOK_FALSE				{ $$ = new NodeRelation(OpNotEqual, new NodeConstf(1.0), new NodeConstf(1.0)); }
	;

/* expression */
exp: rstmt				{ MPRINT("exp: rstmt"); $$ = $1; }
	| exp TOK_POW exp	{ $$ = new NodeOp(OpPow, $1, $3); }
	| exp '*' exp		{ $$ = new NodeOp(OpMul, $1, $3); }
	| exp '/' exp		{ $$ = new NodeOp(OpDiv, $1, $3); }
	| exp '+' exp		{ $$ = new NodeOp(OpPlus, $1, $3); }
	| exp '-' exp		{ $$ = new NodeOp(OpMinus, $1, $3); }
	| exp '%' exp		{ $$ = new NodeOp(OpMod, $1, $3); }
	| '(' bexp ')'		{ MPRINT("exp: (bexp)"); $$ = $2; } // bexp only an expression when wrapped
	| str				{ $$ = $1; }
	| TOK_NUM			{
							double f = atof(yytext);
							$$ = new NodeConstf(f);
						}
    /* DAS THESE ARE NOW PURE RIGHT-HAND-SIDE */
    | '{' level expl '}'	{ MPRINT("exp: {expl}");	decrLevel(); $$ = new NodeList($3); }
	| '{' '}'				{ MPRINT("exp: {}");	$$ = new NodeList(new NodeEmptyListElem()); }

	| obj '[' exp ']' 	{	MPRINT("exp: obj[exp]");    $$ = new NodeSubscriptRead($1, $3); }
    | obj'.'id           {   MPRINT("exp: obj.id");    $$ = new NodeMember($1, $3); }
	| TOK_ARG_QUERY		{
                            $$ = parseArgumentQuery(yytext, &flerror);
						}
	| TOK_ARG			{
                            $$ = parseScoreArgument(yytext, &flerror);
						}
	| TOK_TRUE			{ $$ = new NodeConstf(1.0); }
	| TOK_FALSE			{ $$ = new NodeConstf(0.0); }
	| '-' exp %prec CASTTOKEN {
								/* NodeConstf is a dummy; makes exct_operator work */
								$$ = new NodeOp(OpNeg, $2, new NodeConstf(0.0));
							}
	| id					{
								MPRINT("exp: id");    $$ = new NodeLoadSym($1);
							}
	;

/* struct member declaration.
    An <mbr> is always a type followed by an <id>, like "float length". They only occur in struct definitions.
 */

mbr: TOK_FLOAT_DECL id        { MPRINT("mbr");  $$ = new NodeMemberDecl($2, MincFloatType); }
    | TOK_STRING_DECL id    { MPRINT("mbr");    $$ = new NodeMemberDecl($2, MincStringType); }
    | TOK_HANDLE_DECL id    { MPRINT("mbr");    $$ = new NodeMemberDecl($2, MincHandleType); }
    | TOK_LIST_DECL id        { MPRINT("mbr");  $$ = new NodeMemberDecl($2, MincListType); }
    | TOK_MAP_DECL id        { MPRINT("mbr");   $$ = new NodeMemberDecl($2, MincMapType); }
    | TOK_FUNC_DECL id        { MPRINT("mbr");   $$ = new NodeMemberDecl($2, MincFunctionType); }
    | structname id           { MPRINT("mbr");   $$ = new NodeMemberDecl($2, MincStructType, $1); }     // member decl for struct includes struct type
    ;

/* struct declaration */

/* a <mbrl> is one <mbr> or a series of <mbr>'s separated by commas, for a struct definition,
    e.g. "float f, float g, string s"
 */

mbrl: mbr               { MPRINT("mbrl: mbr");
            $$ = new NodeSeq(new NodeNoop(), $1); }
    | mbrl ',' mbr      { MPRINT("mbrl: mbrl,mbr");
            $$ = new NodeSeq($1, $3); }
    ;

/* "struct Foo" - here we just return the id for the struct's name */

structname: TOK_STRUCT_DECL id { MPRINT("structname"); $$ = $2; }
    ;

/* "struct Foo { <member decls> }" */

structdef: structname '{' mbrl '}' struct   { MPRINT("structdef");
        --slevel; MPRINT1("slevel => %d", slevel);
        $$ = go(new NodeStructDef($1, $3));
    }
    ;

/* struct level counter */

struct:         {    MPRINT("struct"); if (slevel > 0) { minc_die("nested struct decls not allowed"); }
                    slevel++; MPRINT1("slevel => %d", slevel);
                }
    ;

/* Everything from this point down is used to build a function declaration/definition */

/* a <arg> is always a type followed by an <id>, like "float length".  They only occur in function definitions.
    The variables declared are not visible outside of the function definition.
    TODO: CAN THESE BE DERIVED FROM SOME OTHER EXISTING RULE?
 */

arg: TOK_FLOAT_DECL id      { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincFloatType); }
    | TOK_STRING_DECL id    { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincStringType); }
    | TOK_HANDLE_DECL id    { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincHandleType); }
    | TOK_LIST_DECL id      { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincListType); }
    | TOK_MAP_DECL id      { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincMapType); }
    | TOK_STRUCT_DECL id id { MPRINT("arg");
                                    $$ = new NodeStructDecl($3, $2); }
    | TOK_FUNC_DECL id      { MPRINT("arg");
                                    $$ = new NodeDecl($2, MincFunctionType); }
    ;

/* function name, e.g. "list myfunction".  Used as first part of definition.
    TODO: This is where I would add the ability for a function to return a function.
 */

funcname: TOK_FLOAT_DECL id function { MPRINT("funcname");
									$$ = go(new NodeFuncDecl(strsave($2), MincFloatType)); }
	| TOK_STRING_DECL id function { MPRINT("funcname");
									$$ = go(new NodeFuncDecl(strsave($2), MincStringType)); }
	| TOK_HANDLE_DECL id function { MPRINT("funcname");
									$$ = go(new NodeFuncDecl(strsave($2), MincHandleType)); }
	| TOK_LIST_DECL id function { MPRINT("funcname");
									$$ = go(new NodeFuncDecl(strsave($2), MincListType)); }
    | TOK_MAP_DECL id function { MPRINT("funcname");
                                    $$ = go(new NodeFuncDecl(strsave($2), MincMapType)); }
    | TOK_STRUCT_DECL id id function { MPRINT("funcname");
                                    $$ = go(new NodeFuncDecl(strsave($3), MincStructType)); }
	;

/* a <argl> is one <arg> or a series of <arg>'s separated by commas */

argl: arg     { MPRINT("argl: arg"); $$ = new NodeArgListElem(new NodeEmptyListElem(), $1); }
    | argl ',' arg    { MPRINT("argl: argl,arg"); $$ = new NodeArgListElem($1, $3); }
    ;

/* a <fargl> is a grouped argument list for a function definition, e.g. "(float f, string s)".
   We store this list in an NodeArgList tree because it gives us the information
   about the types of each of the arguments at the point where the function is called.
   When the function is executed, each NodeArgListElem executes, which declares the argument
   variable in the function's scope, then accesses it via lookup().
 */

fargl: '(' argl ')'		{ MPRINT("fargl: (argl)"); $$ = new NodeArgList($2); }
	| '(' ')'           { MPRINT("fargl: (NULL)"); $$ = new NodeArgList(new NodeEmptyListElem()); }
	;

/* function block level counter */

function:  level {	MPRINT("function"); if (flevel > 0) {
								minc_die("nested function decls not allowed");
							}
							flevel++; MPRINT1("flevel => %d", flevel);
						 }
;

/* function statement list.  Must be a statement list ending with a return statement */

fstml:	stml ret			{	MPRINT("fstml: stml,ret");
									$$ = new NodeFuncSeq($1, $2);
							}
	| ret					{	MPRINT("fstml: ret");
									$$ = new NodeFuncSeq(new NodeEmptyListElem(), $1);
							}
	;

/* the full rule for a function definition, e.g. "list myfunction(string s) { ... }" */

fdef: funcname fargl '{' fstml '}'	{
									MPRINT("fdef");
									decrLevel();
									--flevel; MPRINT1("flevel => %d", flevel);
									$$ = go(new NodeFuncDef($1, $2, $4));
								}
	| error funcname fargl '{' stml '}'	{ minc_die("%s(): function body must end with 'return <exp>' statement", $2); flerror = 1; $$ = new NodeNoop(); }
	| error funcname fargl '{' '}'	{ minc_die("%s(): function body must end with 'return <exp>' statement", $2); flerror = 1; $$ = new NodeNoop(); }
	;

%%

static void 	incrLevel()
{
	++level; MPRINT1("level => %d", level);
}

static void		decrLevel()
{
	--level; MPRINT1("level => %d", level);
}

// N.B. Because we have no need for <id>'s to exist in our tree other than for the purpose
// of declaring variables, we shortcut here and do not rely on the recursive parser.  We
// create our own sequence of declaration nodes.

static Node * declare(MincDataType type)
{
	MPRINT2("declare(type=%d, idcount=%d)", type, idcount);
	assert(idcount > 0);
	
	Node * t = new NodeNoop();	// end of the list
	for (int i = 0; i < idcount; i++) {
		Node * decl = new NodeDecl(idlist[i], type);
		t = new NodeSeq(t, decl);
	}
	return t;
}

static Node * declareStruct(const char *typeName)
{
    MPRINT2("declareStruct(typeName='%s', idcount=%d)", typeName, idcount);
    assert(idcount > 0);
    Node * t = new NodeNoop();    // end of the list
    for (int i = 0; i < idcount; i++) {
        Node * decl = new NodeStructDecl(idlist[i], typeName);
        t = new NodeSeq(t, decl);
    }
    return t;
}

static Node * parseArgumentQuery(const char *text, int *pOutErr)
{
#ifndef EMBEDDED
    /* ?argument will return 1.0 if defined, else 0.0 */
    const char *token = strsave(text + 1);    // strip off '?'
    // returns NULL silently if not found
    const char *value = lookup_token(token, false);
    return new NodeConstf(value != NULL ? 1.0 : 0.0);
#else
    minc_warn("Argument variables not supported");
    *pOutErr = 1;
    return new NodeNoop();
#endif
}

static Node * parseScoreArgument(const char *text, int *pOutErr)
{
#ifndef EMBEDDED
    const char *token = strsave(text + 1);    // strip off '$'
    const char *value = lookup_token(token, true);        // returns NULL with warning
    if (value != NULL) {
        // We store this as a number constant if it can be coaxed into a number,
        // else we store this as a string constant.
        int i, is_number = 1;
        for(i = 0; value[i] != '\0'; ++i) {
            if ('-' == value[i] && i == 0)
            continue;    // allow initial sign
            if (! (isdigit(value[i]) || '.' == value[i]) ) {
                is_number = 0;
                break;
            }
        }
        if (is_number) {
            double f = atof(value);
            return new NodeConstf(f);
        }
        else {
            // Strip off extra "" if present
            if (value[0] == '"' && value[strlen(value)-1] == '"') {
                char *vcopy = strsave(value+1);
                *strrchr(vcopy, '"') = '\0';
                return new NodeString(vcopy);
            }
            else {
                return new NodeString(strsave(value));
            }
        }
    }
    else {
        *pOutErr = 1;
        return new NodeNoop();
    }
#else
    minc_warn("Argument variables not supported");
    *pOutErr = 1;
    return new NodeNoop();
#endif
}

static Node *
go(Node * t1)
{
	if (level == 0) {
		MPRINT1("--> go(%p)", t1);
		try {
			t1->exct();
		}
        catch(const RTException &rtex) {
            delete t1;
            t1 = NULL;
            cleanup();
            rterror("parser", "caught fatal exception: '%s' - bailing out", rtex.mesg());
            throw;
        }
		catch(...) {
			MPRINT1("caught exception - deleting node %p and cleaning up", t1);
			delete t1;
			t1 = NULL;
			cleanup();
			throw;
		}
		MPRINT1("<-- go(%p)", t1);
	}
	return t1;
}

int yywrap()
{
	return 1;
}

static void cleanup()
{
	rtcmix_debug("cleanup", "yy_init = %d", yy_init);
	MPRINT1("Freeing program tree %p", program);
    delete program;
	program = NULL;
	/* Reset all static state */
	comments = 0;	// from lex.yy.c
	cpcomments = 0;
	xblock = 0;
	idcount = 0;
	flerror = 0;
	flevel = 0;
    slevel = 0;
	level = 0;
	include_stack_index = 0;
    if (!preserve_symbols) {
        free_symbols();
    }
#ifndef EMBEDDED
	/* BGG mm -- I think this buffer gets reused, so we don't delete it */
	yy_delete_buffer(YY_CURRENT_BUFFER);
	YY_CURRENT_BUFFER_LVALUE = NULL;
#endif
}

void preserveSymbols(bool preserve)
{
    preserve_symbols = preserve;
}

void reset_parser()
{
    rtcmix_debug("reset_parser", "resetting line number");
    flerror = 0;
    // Reset the line # every time a new score buffer is received
    yyset_lineno(1);
    // Special exported function from minc.l
    yy_clear_includes();
}

#ifdef EMBEDDED

#warning DAS Make sure yylex_destroy() works
#define USE_YYLEX_DESTROY

// BGG mm -- for dynamic memory mgmt (double return for UG_INTRO() macro)
double minc_memflush()
{
	rtcmix_debug("minc_memflush", "Freeing parser memory");
	delete program;
	program = NULL;
	free_symbols();
#ifdef USE_YYLEX_DESTROY
	yylex_destroy();
#else
	yy_delete_buffer(YY_CURRENT_BUFFER);
	YY_CURRENT_BUFFER_LVALUE = NULL;
	
	yy_init = 1;    /* whether we need to initialize */
	yy_start = 0;   /* start state number */
#endif
	return 1.0;
}

#endif

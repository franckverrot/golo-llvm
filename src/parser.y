%{
  #include "src/includes/node.h"
  #include <cstdio>
  #include <cstdlib>
  #define YYERROR_VERBOSE
  #define YYDEBUG 1
  NBlock *programBlock; /* the top level root node of our final AST */
  NModule *topLevelModule; /* name of the llvm module */

  extern int yylex();
  void yyerror(const char *s) { std::printf("Error: %s\n", s);std::exit(1); }
%}

/* Represents the many different ways we can access our data */
%union {
  Node *node;
  NBlock *block;
  NExpression *expr;
  NStatement *stmt;
  NIdentifier *ident;
  NModule *module;
  NVariableDeclaration *var_decl;
  std::vector<NVariableDeclaration*> *varvec;
  std::vector<NExpression*> *exprvec;
  std::string *string;
  int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER TINTEGER TSTRING TDOUBLE TMODULE TCOMMENT_BEG
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TPIPE
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV
%token <token> TRETURN TFUNC TLET

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <ident> ident
%type <expr> numeric expr string
%type <varvec> func_decl_args
%type <exprvec> call_args
%type <block> program stmts block
%type <stmt> stmt var_decl func_decl comment
%type <token> comparison
%type <module> module

/* Operator precedence for mathematical operators */
%left TPLUS TMINUS
%left TMUL TDIV

%start program

%%

program : module stmts { topLevelModule = $1; programBlock = $2; }
        ;

module : TMODULE ident { $$ = new NModule(*$2); }
       ;

stmts : stmt { $$ = new NBlock(); $$->statements.push_back($<stmt>1); }
      | stmts stmt { $1->statements.push_back($<stmt>2); }
    ;

stmt : var_decl | func_decl
     | expr { $$ = new NExpressionStatement(*$1); }
     | TRETURN expr { $$ = new NReturnStatement(*$2); }
     | comment
     ;

comment : TCOMMENT_BEG  { $$ = new NCommentStatement(*$1); }
        ;

block : TLBRACE stmts TRBRACE { $$ = $2; }
      | TLBRACE TRBRACE { $$ = new NBlock(); }
    ;

var_decl : TLET ident { $$ = new NVariableDeclaration(*$2); }
         | TLET ident TEQUAL expr { $$ = new NVariableDeclaration(*$2, $4); }
     ;

func_decl : TFUNC ident TEQUAL TPIPE func_decl_args TPIPE block
          { $$ = new NFunctionDeclaration(*$2, *$5, *$7);}
      ;

func_decl_args : /*blank*/  { $$ = new VariableList(); }
               | ident { $$ = new VariableList(); $$->push_back(new NVariableDeclaration(*$1)); }
      | func_decl_args TCOMMA ident { $1->push_back(new NVariableDeclaration(*$3)); }
      ;

ident : TIDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
      ;

numeric : TINTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
        | TDOUBLE { $$ = new NDouble(atof($1->c_str())); delete $1; }
    ;

string : TSTRING { std::string foo = "BLAH "; $$ = new NString(foo);}
       ;

expr : ident TEQUAL expr { $$ = new NAssignment(*$<ident>1, *$3); }
     | ident TLPAREN call_args TRPAREN { $$ = new NMethodCall(*$1, *$3); }
     | ident { $<ident>$ = $1; }
     | string
     | numeric
     | expr TMUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TDIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TPLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TMINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | TLPAREN expr TRPAREN { $$ = $2; }
   ;

call_args : /*blank*/  { $$ = new ExpressionList(); }
          | expr { $$ = new ExpressionList(); $$->push_back($1); }
      | call_args TCOMMA expr  { $1->push_back($3); }
      ;

comparison : TCEQ | TCNE | TCLT | TCLE | TCGT | TCGE;
%%

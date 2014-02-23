#ifndef __NODE__H
#define __NODE__H
#include <iostream>
#include <vector>
#include <set>
#include <llvm/Value.h>

class CodeGenContext;
class NStatement;
class NExpression;
class NVariableDeclaration;
class NImport;

typedef std::vector<NStatement*> StatementList;
typedef std::vector<NExpression*> ExpressionList;
typedef std::vector<NVariableDeclaration*> VariableList;
typedef std::set<NImport*> Imports;

class Node {
  public:
    virtual ~Node() {}
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth) { return NULL; }
};

class NExpression : public Node {
};

class NStatement : public Node {
};

class NString : public NExpression {
  public:
    std::string value;
    NString(const std::string& value) : value(value) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NInteger : public NExpression {
  public:
    long long value;
    NInteger(long long value) : value(value) { };
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NDouble : public NExpression {
  public:
    double value;
    NDouble(double value) : value(value) { };
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NIdentifier : public NExpression {
  public:
    std::string name;
    NIdentifier(const std::string& name) : name(name) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NMethodCall : public NExpression {
  public:
    const NIdentifier& id;
    ExpressionList arguments;
    NMethodCall(const NIdentifier& id, ExpressionList& arguments) :
      id(id), arguments(arguments) { }
    NMethodCall(const NIdentifier& id) : id(id) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NBinaryOperator : public NExpression {
  public:
    int op;
    NExpression& lhs;
    NExpression& rhs;
    NBinaryOperator(NExpression& lhs, int op, NExpression& rhs) :
      lhs(lhs), rhs(rhs), op(op) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NAssignment : public NExpression {
  public:
    NIdentifier& lhs;
    NExpression& rhs;
    NAssignment(NIdentifier& lhs, NExpression& rhs) : 
      lhs(lhs), rhs(rhs) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NBlock : public NExpression {
  public:
    StatementList statements;
    NBlock() { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NExpressionStatement : public NStatement {
  public:
    NExpression& expression;
    NExpressionStatement(NExpression& expression) : 
      expression(expression) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NReturnStatement : public NStatement {
  public:
    NExpression& expression;
    NReturnStatement(NExpression& expression) : 
      expression(expression) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NCommentStatement : public NStatement {
  public:
    std::string& comment;
    NCommentStatement(std::string& comment) : comment(comment) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NVariableDeclaration : public NStatement {
  public:
    NIdentifier& id;
    NExpression *assignmentExpr;
    NVariableDeclaration(NIdentifier& id) : id(id) { }
    NVariableDeclaration(NIdentifier& id, NExpression *assignmentExpr) :
      id(id), assignmentExpr(assignmentExpr) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NFunctionDeclaration : public NStatement {
  public:
    const NIdentifier& id;
    VariableList arguments;
    NBlock block;
    NFunctionDeclaration(const NIdentifier& id, const VariableList& arguments, NBlock& block) :
      id(id), arguments(arguments), block(block) { }
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NModule : public NExpression {
  public:
    const NIdentifier& ident;
    NModule(const NIdentifier& ident) : ident(ident) { };
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

class NImport : public NExpression {
  public:
    const NIdentifier& ident;
    NImport(const NIdentifier& ident) : ident(ident) { };
    virtual llvm::Value* codeGen(CodeGenContext& context, int depth);
};

#endif

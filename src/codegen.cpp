#include "src/includes/codegen.hpp"
#include "build/parser.hpp"
#include <iostream>
#include <fstream>

using namespace std;

CodeGenContext::CodeGenContext(std::string moduleName) {
  module = new Module(moduleName, getGlobalContext());
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NModule& mod, NBlock& root)
{
  std::cerr << "Starting code generation..." << endl << std::flush;

  /* Create the top level interpreter function to call as entry */
  vector<Type*> argTypes;
  FunctionType *ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(argTypes), false);
  mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, mod.ident.name, module);
  BasicBlock *bblock = BasicBlock::Create(getGlobalContext(), "entry", mainFunction, 0);

  /* Push a new variable/block context */
  pushBlock(bblock);
  root.codeGen(*this, 0); /* emit bytecode for the toplevel block */
  ReturnInst::Create(getGlobalContext(), bblock);
  popBlock();

  runPasses();
  std::cerr << "Code generation is done." << endl;
}

void CodeGenContext::printModule(std::string outputFileName) {
  //module->print((raw_ostream&)OutFile, 0);
  FILE * f = fopen(outputFileName.c_str(), "w+");
  raw_fd_ostream os(fileno(f), true, false);
  module->print(os,0);
}

void CodeGenContext::runPasses() {
  PassManager pm;
  pm.add(createVerifierPass());
  //pm.add(createPrintModulePass((raw_fd_ostream&)OutFile));
  pm.run(*module);
}



/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() {
  std::cerr << "Running code...\n";
  ExecutionEngine *ee = EngineBuilder(module).create();
  vector<GenericValue> noargs;
  GenericValue v = ee->runFunction(mainFunction, noargs);
  std::cerr << "Code was run.\n";
  return v;
}

/* Returns an LLVM type based on the identifier */
static Type *typeOf(const NIdentifier& type) 
{
  Type * charType = Type::getInt8Ty(getGlobalContext());
  Type * stringType = Type::getInt8Ty(getGlobalContext());

  if (type.name.compare("int") == 0) {
    return Type::getInt64Ty(getGlobalContext());
  }
  else if (type.name.compare("double") == 0) {
    return Type::getDoubleTy(getGlobalContext());
  }
  else if (type.name.compare("[string]") == 0) {
    return ArrayType::get(charType,2);
  }
  return Type::getVoidTy(getGlobalContext());
}

class Debug 
{
  public:
    Debug& operator()(int depth) { 
      for(int i = 0; i < depth; i++) { std::cerr << '\t'; }
      std::cerr << depth << " ";
      return *this;
    }

    template<class T>
      Debug& operator<<(T t) {
        std::cerr << t;
        return *this;
      }

    Debug& operator<<(ostream& (*f)(ostream& o)) {
      std::cerr << f;
      return *this;
    };
};

/* -- Code Generation -- */

Value* NString::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating string: " << value << endl << std::flush;

  StringRef r(value);
  return ConstantDataArray::getString(getGlobalContext(), r, false);
}

Value* NInteger::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating integer: " << value << endl;
  return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating double: " << value << endl;
  return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating identifier reference: " << name << endl;
  if (context.locals().find(name) == context.locals().end()) {
    AllocaInst *alloc = new AllocaInst(typeOf(*(new NIdentifier("int"))), name.c_str(), context.currentBlock());
    context.locals()[name] = alloc;
    debug(depth) << "undeclared variable " << name << "... declaring it." << endl;
    return NULL;
  }
  return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* NModule::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating module reference: " << ident.name << endl;
  //if (context.locals().find(name) == context.locals().end()) {
  //  debug(depth) << "[ERR]" << "undeclared variable " << name << endl;
  //  return NULL;
  //}
  //return new LoadInst(context.locals()[name], "", false, context.currentBlock());
  return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 42, true);
}

Value* NMethodCall::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  std::string fname = context.module->getModuleIdentifier() + "_" + id.name;
  Function *function = context.module->getFunction(fname.c_str());
  if (function == NULL) {
    debug(depth) << "[ERR]" << "no such function " << fname << endl;
    exit(-1);
  }
  std::vector<Value*> args;
  ExpressionList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    args.push_back((**it).codeGen(context, depth + 1));
  }
  CallInst *call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
  debug(depth) << "Creating method call: " << fname << endl;
  return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating binary operation " << op << endl;
  Instruction::BinaryOps instr;
  switch (op) {
    case TPLUS:   instr = Instruction::Add; goto math;
    case TMINUS:   instr = Instruction::Sub; goto math;
    case TMUL:     instr = Instruction::Mul; goto math;
    case TDIV:     instr = Instruction::SDiv; goto math;

                  /* TODO comparison */
  }

  return NULL;
math:
  return BinaryOperator::Create(instr, lhs.codeGen(context, depth + 1), 
      rhs.codeGen(context, depth + 1), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating assignment for " << lhs.name << endl;
  // cannot really happen in fact, as we create variables on the fly
  // I think the variables should be declared though, so something's
  // required to change to have a special case for func arguments
  //FIXME: la declaration de variable devrait se faire apres l'assignment?
  //if (context.locals().find(lhs.name) == context.locals().end()) {
  //  debug(depth) << "[ERR]" << "undeclared variable " << lhs.name << endl;
  //  return NULL;
  //}
  Value * val  = rhs.codeGen(context, depth + 1);
  debug(depth) << "1Creating assignment for " << lhs.name << endl;

  Type * type = typeOf(*(new NIdentifier("int")));
  //AllocaInst *alloc = new AllocaInst(typeOf(*(new NIdentifier("int"))), lhs.name.c_str(), context.currentBlock());
  //context.locals()[lhs.name] = alloc;

  Value * addr = context.locals()[lhs.name];
  return new StoreInst(val, addr, /* volatile? */ false, /* insertAtEnd */ context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  StatementList::const_iterator it;
  Value *last = NULL;
  for (it = statements.begin(); it != statements.end(); it++) {
    debug(depth) << "Generating code for " << typeid(**it).name() << endl;
    last = (**it).codeGen(context, depth + 1);
  }
  debug(depth) << "Creating block" << endl;
  return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Generating code for " << typeid(expression).name() << endl;
  return expression.codeGen(context, depth + 1);
}

Value* NReturnStatement::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Generating return code for " << typeid(expression).name() << endl;
  Value *returnValue = expression.codeGen(context, depth + 1);
  context.setCurrentReturnValue(returnValue);
  return returnValue;
}

Value* NCommentStatement::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Generating comment code for " << typeid(comment).name() << endl;
  Value *returnValue = context.getCurrentReturnValue();
  context.setCurrentReturnValue(returnValue);
  return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  debug(depth) << "Creating variable declaration " << id.name << endl;
  Type * type = typeOf(*(new NIdentifier("int")));
  AllocaInst *alloc = new AllocaInst(type, id.name.c_str(), context.currentBlock());
  context.locals()[id.name] = alloc;
  if (assignmentExpr != NULL) {
    debug(depth + 1) << "and assign expr..." << endl;
    NAssignment assn(id, *assignmentExpr);
    Value * assgen = assn.codeGen(context, depth + 1);
    debug(depth + 1) << "[" << typeid(assignmentExpr).name() << "]" << endl;
  } else {
    debug(depth + 1) << "but without assign expr..." << endl;
  }
  return NULL;
  return context.locals()[id.name];
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context, int depth)
{
  Debug debug;
  vector<Type*> argTypes;
  VariableList::const_iterator it;
  GlobalValue::LinkageTypes linkage;
  if (externalLinkage) {
    linkage = GlobalValue::ExternalLinkage;
  }
  else {
    linkage = GlobalValue::InternalLinkage;
  }
  for (it = arguments.begin(); it != arguments.end(); it++) {
    //argTypes.push_back(typeOf((**it).type));
    argTypes.push_back(typeOf(*(new NIdentifier("int"))));
  }
  std::string fname = context.module->getModuleIdentifier() + "_" + id.name;

  NIdentifier * typeIdentifier;

  //if(fname == "main") {
  //  debug(depth) << "renaming main to _golo_entry_point " << endl;
  //  fname = "_golo_entry_point";
  //  typeIdentifier = new NIdentifier("[string]");
  //} else {
  //  typeIdentifier = new NIdentifier("int");
  //}
  //TODO: unforce
  typeIdentifier = new NIdentifier("int");

  debug(depth) << "Function " << fname.c_str() << " has " << arguments.size() << " argument(s)" << endl;
  FunctionType *ftype = FunctionType::get(typeOf(*typeIdentifier), makeArrayRef(argTypes), false);
  Function *function = Function::Create(ftype, linkage, fname.c_str(), context.module);
  BasicBlock *bblock = BasicBlock::Create(getGlobalContext(), "entry", function, 0);

  context.pushBlock(bblock);

  Function::arg_iterator argsValues = function->arg_begin();
  Value* argumentValue;

  for (it = arguments.begin(); it != arguments.end(); it++) {
    (**it).codeGen(context, depth + 1);

    argumentValue = argsValues++;
    argumentValue->setName((**it).id.name.c_str());
    StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
  }

  block.codeGen(context, depth + 1);
  ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(), bblock);

  context.popBlock();
  debug(depth) << "Creating function: " << id.name << endl;
  return function;
}

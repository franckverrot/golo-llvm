#include "src/includes/version.hpp"
#include "src/includes/golo-llvm.hpp"
#include "src/includes/codegen.hpp"

extern int yyparse(void);
extern FILE *yyin;
extern NBlock* programBlock;
extern NModule* topLevelModule;

void createCoreFunctions(CodeGenContext& context);
void parseOptions(int, char**);

char *outputFileName = NULL;
char *inputFileName  = NULL;

GoloLLVM::GoloLLVM(int argc, char **argv) {
  parseOptions(argc, argv);

  if(!inputFileName) {
    yyin = stdin;
  } else {
    yyin = fopen(inputFileName,"r");
  }
  yyparse();
  fclose(yyin);

  std::cerr << "Program block is " << programBlock << std::endl;
  // see http://comments.gmane.org/gmane.comp.compilers.llvm.devel/33877
  InitializeNativeTarget();
  CodeGenContext context(topLevelModule->ident.name, imports);
  createCoreFunctions(context);
  context.generateCode(*topLevelModule, *programBlock);
  //context.runCode();
  context.printModule(outputFileName);
}

void parseOptions(int argc, char **argv) {
  int index;
  int option;

  opterr = 0;

  while ((option = getopt (argc, argv, "c:o:")) != -1)
    switch(option)
    {
      case 'c':
        inputFileName = optarg;
        break;
      case 'o':
        outputFileName = optarg;
        break;
      case '?':
        if ((optopt == 'c') || (optopt == 'o'))
          fprintf (stderr, "You must specify a file to the -%c option.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        exit(1);
      default:
        abort ();
    }

  std::cout << "golo-llvm " << VERSION << std::endl;
  std::cout << "-- input: "<< inputFileName << std::endl;
  std::cout << "-- output: "<< outputFileName << std::endl;

  if (optind < argc) {
    printf("Options found but not recognized:\n", argc - optind);
    for (index = optind; index < argc; index++) {
      printf ("\t* %s\n", argv[index]);
      exit(1);
    }
  }
}

#define __BUILDING_CVC4LIB_UNIT_TEST

#include <string>

#include "parser/parser_builder.h"
#include "parser/parser.h"
#include "options/options.h"
#include "util/language.h"
#include "expr/expr_manager.h"
#include "expr/command.h"
#include "context/context.h"
#include "smt/smt_engine.h"
#include "smt/smt_engine_scope.h"

using namespace CVC4;
using namespace CVC4::parser;

#include "mcsat/solver.h"

using namespace std;

int main(int argc, char* argv[]) {
  
  // Create the expression manager
  Options options;
  ExprManager exprManager(options);

  // Parse the options
  vector<string> filenames = options.parseOptions(argc, argv); 

  // Output as SMT2
  Debug.getStream() << expr::ExprSetDepth(-1) << expr::ExprSetLanguage(language::output::LANG_SMTLIB_V2);
  
  // Solve the problems 
  for (unsigned i = 0; i < filenames.size(); ++ i) {
    try {
      // Make an smt engine so just it has statistics registry
      SmtEngine smtEngine(&exprManager);
   
      // Create the parser
      ParserBuilder parserBuilder(&exprManager, filenames[i], options);
      Parser* parser = parserBuilder.withInputLanguage(language::input::LANG_SMTLIB_V2).build();
  
      // Create the mcsat solver
      context::UserContext userContext;
      context::Context searchContext;

      mcsat::Solver* mcSolver = 0;
      { 
        smt::SmtScope smtScope(&smtEngine);
        mcSolver = new mcsat::Solver(&userContext, &searchContext);
      }
      
      // Parse and assert
      Command* cmd;
      while ((cmd = parser->nextCommand())) {
  
        AssertCommand* assert = dynamic_cast<AssertCommand*>(cmd);
        if (assert) {
          // Get the assertion
          Node assertion = assert->getExpr();
          // Assert to mcsat
          {
            smt::SmtScope smtScope(&smtEngine);
            mcSolver->addAssertion(assertion, true);
          }
        }
  
        delete cmd;
      }
  
      // Check the problem
      bool result = mcSolver->check();
    
      cout << (result ? "sat" : "unsat") << endl;
      
      // Get rid of the parser
      delete parser;
      // Get rid of mcSat
      { 
        smt::SmtScope smtScope(&smtEngine);
        delete mcSolver;
      }
    } catch (const Exception& e) {
      std::cerr << e << std::endl;
    }
  }
}

#include "ufo/Smt/ZExprConverter.hpp"
#include "ufo/Smt/Yices2SolverImpl.hpp"
#include "ufo/Smt/MarshalYices.hpp"

#include "llvm/Support/raw_ostream.h"

#include "doctest.h"

TEST_CASE("yices2.test") {
  using namespace std;
  using namespace expr;
  using namespace ufo;


  using namespace seahorn;
  using namespace yices;


  seahorn::solver::solver_options opts;
  expr::ExprFactory efac;

  Expr x = bind::intConst (mkTerm<string> ("x", efac));
  Expr y = bind::intConst (mkTerm<string> ("y", efac));

  Expr e1 = mk<EQ>(x, mkTerm<mpz_class>(0, efac));
  Expr e2 = mk<EQ>(y, mkTerm<mpz_class>(5, efac));
  Expr e3 = mk<GT>(x, y);
  Expr e4 = mk<GT>(y, x);

  std::vector<Expr> args({e1, e2, e4});

  Expr e = mknary<AND>(args.begin(), args.end());

  yices_impl solver(&opts, efac);
  
  
  bool success = solver.add(e);




  if (success){

    solver::Solver::result answer = solver.check();

    if(answer == solver::Solver::ERROR){

      llvm::errs() << yices::error_string() <<  "\n";

    } else {

      llvm::errs () << "yices2 is fantastic: " <<  answer <<  "\n";

      if (answer == solver::Solver::SAT){

	solver::model *model = solver.get_model();

	Expr valx = model->eval(x, false);
	Expr valy = model->eval(y, false);

	llvm::errs () << "valx = " << *valx  << "\n";
	llvm::errs () << "valy = " << *valy  << "\n";
       
	llvm::errs () << *model << "\n";

	delete model;
      }
    }

    

  } else {

    llvm::errs () << "fix your code Ian.\n";

  }


 EZ3 ctx(efac);
 ZSolver<EZ3> s(ctx);
 s.assertExpr(e);
 auto r = s.solve();

  if (r) {
    llvm::errs() << "SAT" << "\n";

    auto m = s.getModel();

    Expr xval = m.eval(x);
    Expr yval = m.eval(y);

    llvm::errs() << "xval = "  << *xval  << "\n";
    llvm::errs() << "yval = "  << *yval  << "\n";

    llvm::errs() << m <<  "\n";

  } else if (!r) {
    llvm::errs() << "UNSAT" << "\n";
  } else {
    llvm::errs() << "UNKNOWN" << "\n";
  }

  llvm::errs() << "FINISHED\n";

  CHECK(true);
}

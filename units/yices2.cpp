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

  llvm::errs () << "yices2 is fantastic" <<  "\n";

  llvm::errs() << yices::error_string() <<  "\n";
    
  CHECK(true);
}

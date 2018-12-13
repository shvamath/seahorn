#include "ufo/Smt/ZExprConverter.hpp"
#include "ufo/Smt/Yices2Solver.hpp"
#include "ufo/Smt/Yices2Impl.hpp"
#include "ufo/Smt/MarshalYices.hpp"

#include "llvm/Support/raw_ostream.h"

#include "doctest.h"

TEST_CASE("yices2.test") {
  using namespace std;
  using namespace expr;
  using namespace ufo;


  using namespace seahorn;
  using namespace yices;

  errs () << "yices2 is fantastic" <<  "\n";

  ytype_t ytp;
  ExprFactory efac;


  //marshal_yices().encode_type(sort::intTy(efac), ytp);


  CHECK(true);
}

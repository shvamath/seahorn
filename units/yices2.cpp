#include "ufo/Smt/ZExprConverter.hpp"
#include "ufo/Smt/Yices2.hpp"
#include "ufo/Smt/Yices2Impl.hpp"
#include "ufo/Smt/Yices2Impl.hpp"
#include "ufo/Smt/Yices2ExprConverter.hpp"

#include "llvm/Support/raw_ostream.h"

#include "doctest.h"

TEST_CASE("yices2.test") {
  using namespace std;
  using namespace expr;
  using namespace ufo;

  errs () << "yices2 is fantastic" <<  "\n";

  CHECK(true);
}

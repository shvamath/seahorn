#include "ufo/ExprLlvm.hpp"

namespace seahorn {
  namespace solver {


    class model {

      virtual
      bool get_value(expr::Expr exp, expr::Expr& value) = 0;



    };




  }
}

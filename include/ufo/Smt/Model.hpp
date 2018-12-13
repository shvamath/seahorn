#include "ufo/ExprLlvm.hpp"

namespace seahorn {
  namespace solver {


    class model {

      virtual
      expr::Expr eval(expr::Expr expr, bool complete) = 0;
      // Get the type/sort of expr
      // translate
      //
      // yices ignores the complete flag


      virtual
      void print(std::ostream& str) const = 0;
      
      // also need to define operator <<
      friend std::ostream& operator<<(std::ostream &OS, const model &model)
      {
	model.print(OS);
	return OS;
      }

    };




  }
}

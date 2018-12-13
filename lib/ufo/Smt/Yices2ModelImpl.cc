#ifdef WITH_YICES2

#include "yices.h"

#include "ufo/Smt/Model.hpp"

namespace seahorn {
  namespace yices {


    class model_impl : public solver::model {

      expr::Expr eval(expr::Expr expr, bool complete){
      // Get the type/sort of expr
      // translate
      //
      // yices ignores the complete flag

	return nullptr;
      }

      void print(std::ostream& str){



      }
      
    };

  }

}
#endif

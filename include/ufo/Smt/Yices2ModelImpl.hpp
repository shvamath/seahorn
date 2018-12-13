#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Model.hpp"

namespace seahorn {
  namespace yices {


    class model_impl : public solver::Model {

      expr::Expr eval(expr::Expr expr, bool complete);
      // Get the type/sort of expr
      // translate
      //
      // yices ignores the complete flag


      void print(std::ostream& str);
      
    };

  }

}
#endif

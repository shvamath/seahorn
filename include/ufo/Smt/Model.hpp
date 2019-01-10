#include "ufo/ExprLlvm.hpp"
#pragma once

namespace seahorn {
  namespace solver {


    class model {

    public:
      virtual ~model(){};

    public:
      virtual
      expr::Expr eval(expr::Expr expr, bool complete) = 0;
      // yices ignores the complete flag

    public:
      virtual
      void print(llvm::raw_ostream& o) const = 0;

      // also need to define operator <<
      friend llvm::raw_ostream& operator<<(llvm::raw_ostream &o, const model &model)
      {
        model.print(o);
        return o;
      }
      
    };


  }
}

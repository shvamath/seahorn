#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Model.hpp"
#include "Yices2SolverImpl.hpp"

namespace seahorn {
  namespace yices {


    class model_impl : public solver::model {

      /* printing defaults */
      static  uint32_t width;
      static  uint32_t height;
      static  uint32_t offset;

      /* the underlying yices model */
      model_t *d_model;

      /* the underlying context */
      yices_impl &d_solver;

      expr::ExprFactory &d_efac;


    public:
      model_impl(model_t *model, yices_impl &solver, expr::ExprFactory &efac);

      ~model_impl();

      //yices ignores the complete flag
      expr::Expr eval(expr::Expr expr, bool complete);

      void print(llvm::raw_ostream& o) const;

    };

  }

}
#endif

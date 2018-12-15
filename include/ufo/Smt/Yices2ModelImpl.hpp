#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Model.hpp"

namespace seahorn {
  namespace yices {


    class model_impl : public solver::model {

      /* printing defaults */
      static  uint32_t width;
      static  uint32_t height;
      static  uint32_t offset;

      /* the underlying yices model */
      model_t *d_model;

      std::map<expr::Expr, term_t> &d_cache;

      expr::ExprFactory &d_efac;


    public:
      model_impl(model_t *model, std::map<expr::Expr, term_t> &cache, expr::ExprFactory &efac);

      ~model_impl();

      //yices ignores the complete flag
      expr::Expr eval(expr::Expr expr, bool complete);

      void print(std::ostream& str) const;

    };

  }

}
#endif

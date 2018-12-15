#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Solver.hpp"

namespace seahorn {
  namespace yices {




    /* the yices solver; actually a yices context. */
    class yices_impl {

      ctx_config_t *d_cfg;

      /* the context */
      context_t *d_ctx;

      expr::ExprFactory &d_efac;

    public:

      /* how should we set the default logic? */
      yices_impl(std::string logic, seahorn::solver::solver_options *opts, expr::ExprFactory &efac);


      ~yices_impl();

      bool add(expr::Expr exp);

      /** Check for satisfiability */
      solver::Solver::result check();

      /** Push a context */
      void push();

      /** Pop a context */
      void pop();

      /** Get a model */
      solver::model *get_model();


    };


  }

}
#endif

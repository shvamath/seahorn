#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Solver.hpp"

namespace seahorn {
  namespace yices {

    class yices_impl;

    /* the yices solver; actually a yices context. */
    class yices : public solver::Solver {


      yices_impl *context;

    public:


      bool add(expr::Expr exp) = 0;

      /** Check for satisfiability */
      result check() = 0;

      /** Push a context */
      void push() = 0;

      /** Pop a context */
      void pop() = 0;

      /** Get a model */
      solver::model *get_model() = 0;



    };

  }

}
#endif

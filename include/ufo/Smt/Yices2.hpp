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


    };

  }

}
#endif

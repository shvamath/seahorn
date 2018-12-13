#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Solver.hpp"
#include "Yices2ExprConverter.hpp"

namespace seahorn {
  namespace yices {



    /* the yices solver; actually a yices context. */
    class yices_impl {

      ctx_config_t *d_cfg;

      /* the context */
      context_t *d_ctx;

    public:

      /* how should we set the default logic? */
      yices_impl(std::string logic, seahorn::solver::solver_options *opts);

      
      ~yices_impl();

    };


  }

}
#endif

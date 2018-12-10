#ifdef WITH_YICES2
#pragma once

#include "yices.h"

#include "Solver.hpp"
#include "Yices2ExprConverter.hpp"

namespace seahorn {
  namespace yices {


    /* flag to indicate library status; we are single threaded so we can be lazy. */
    static bool s_yices_lib_initialized = false;


    inline void yices_library_initialize(void){
      if( !s_yices_lib_initialized ){
        s_yices_lib_initialized = true;
        yices_init();
      }
    }


    /* the yices solver; actually a yices context. */
    class yices_impl {

      ctx_config_t *d_cfg;

      /* the context */
      context_t *d_ctx;

    public:

      /* how should we set the default logic? */
      yices_impl(std::string logic, seahorn::solver::solver_options *opts){
        yices_library_initialize();
	/* the yices configuration structure */
	ctx_config_t *cfg;

	if ( logic.empty() && !opts ){
	  cfg = NULL;
	} else {
	  cfg = yices_new_config();
	  if ( ! logic.empty() ){
	    int32_t errcode = yices_default_config_for_logic(cfg, logic.c_str());
	  } 
	  if ( opts ){
	    /* iterate through the opts map and set the keys */

	  }

	}

	d_ctx = yices_new_context(cfg);
        yices_free_config(cfg);
	
      }

      
      ~yices_impl(){
        yices_free_context(d_ctx);
      };





    };


  }

}
#endif

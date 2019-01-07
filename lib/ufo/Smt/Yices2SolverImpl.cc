#ifdef WITH_YICES2

#include <gmp.h>
#include "yices.h"

#include "ufo/Smt/Yices2SolverImpl.hpp"
#include "ufo/Smt/Yices2ModelImpl.hpp"
#include "ufo/Smt/MarshalYices.hpp"

using namespace expr;


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



    /* how should we set the default logic? */
    yices_impl::yices_impl(std::string logic, seahorn::solver::solver_options *opts, expr::ExprFactory &efac):
      Solver(opts),
      d_efac(efac)
    {
      yices_library_initialize();
      /* the yices configuration structure */
      ctx_config_t *cfg;

      if ( logic.empty() && opts != nullptr ){
        cfg = NULL;
      } else {
        cfg = yices_new_config();
        if ( ! logic.empty() ){
          int32_t errcode = yices_default_config_for_logic(cfg, logic.c_str());
        }
        if ( opts != nullptr ){
          /* iterate through the opts map and set the keys */
          std::map<std::string, std::string>::iterator it;
          for ( it = opts->begin(); it != opts->end(); it++ ){
            yices_set_config(cfg, it->first.c_str(), it->second.c_str());
          }
        }

      }

      d_ctx = yices_new_context(cfg);
      yices_free_config(cfg);

    }

    yices_impl::~yices_impl(){
      yices_free_context(d_ctx);
    }


    yices_impl::ycache_t& yices_impl::get_cache(void){
      return d_cache;
    }


    bool yices_impl::add(expr::Expr exp){
      term_t yt = marshal_yices::encode_term(exp, get_cache());
      if (yt == NULL_TERM){
        llvm::errs() << "yices_impl::add:  failed to encode: " << *exp << "\n";
        return false;
      }
      int32_t errcode = yices_assert_formula(d_ctx, yt);
      if (errcode == -1){
        llvm::errs() << "yices_impl::add:  yices_assert_formula failed: " << yices::error_string() << "\n";
        return false;
      }

      return true;
    }

    /** Check for satisfiability */
    solver::Solver::result yices_impl::check(){
      //could have a param_t field for this call.
      smt_status_t stat = yices_check_context(d_ctx, nullptr);
      switch(stat){
      case STATUS_UNSAT: return solver::Solver::UNSAT;
      case STATUS_SAT: return solver::Solver::SAT;
      case STATUS_UNKNOWN: return solver::Solver::UNKNOWN;
      case STATUS_INTERRUPTED: return solver::Solver::UNKNOWN;
      default:
        return solver::Solver::UNKNOWN;
      }
      return solver::Solver::UNKNOWN;
    }

    /** Push a context */
    void yices_impl::push(){
      yices_push(d_ctx);
    }

    /** Pop a context */
    void yices_impl::pop(){
      yices_pop(d_ctx);
    }

    /** Get a model   WHO FREES THE MODEL */
    solver::model* yices_impl::get_model(){
      model_t *model = yices_get_model(d_ctx, 1); //BD & JN: keep subst??
      return new model_impl(model, *this, d_efac);
    }



  }

}





#endif

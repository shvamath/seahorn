#ifdef WITH_YICES2

#include "yices.h"

#include "ufo/Smt/Model.hpp"
#include "ufo/Smt/Yices2ModelImpl.hpp"

namespace seahorn {
  namespace yices {

    /* printing defaults */
    uint32_t model_impl::width   = 80;
    uint32_t model_impl::height  = 100;
    uint32_t model_impl::offset  = 0;


    model_impl::model_impl(model_t *model, std::map<expr::Expr, term_t> &cache, expr::ExprFactory &efac):
      d_model(model),
      d_cache(cache),
      d_efac(efac)
    {  }

    model_impl::~model_impl(){
      yices_free_model(d_model);
    }

    expr::Expr model_impl::eval(expr::Expr expr, bool complete){

      return nullptr;
    }

    void model_impl::print(std::ostream& strm) const {
      char* model_as_string = yices_model_to_string(d_model, width, height, offset);
      strm << model_as_string;
      yices_free_string(model_as_string);
    }



  }

}
#endif

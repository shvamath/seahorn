#ifdef WITH_YICES2

#include "yices.h"

#include "ufo/Smt/Model.hpp"
#include "ufo/Smt/MarshalYices.hpp"
#include "ufo/Smt/Yices2ModelImpl.hpp"

namespace seahorn {
  namespace yices {

    /* printing defaults */
    uint32_t model_impl::width   = 80;
    uint32_t model_impl::height  = 100;
    uint32_t model_impl::offset  = 0;


    model_impl::model_impl(model_t *model, yices_impl &solver, expr::ExprFactory &efac):
      d_model(model),
      d_solver(solver),
      d_efac(efac)
    {  }

    model_impl::~model_impl(){
      yices_free_model(d_model);
    }

    expr::Expr model_impl::eval(expr::Expr exp, bool complete){
      return marshal_yices::eval(exp,  d_efac, d_solver.get_cache(), complete, d_model);
    }

    void model_impl::print(llvm::raw_ostream& o) const {
      char* model_as_string = yices_model_to_string(d_model, width, height, offset);
      o << model_as_string;
      yices_free_string(model_as_string);
    }



  }

}
#endif

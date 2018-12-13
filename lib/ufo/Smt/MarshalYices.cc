#ifdef WITH_YICES2

#include "ufo/Smt/MarshalYices.hpp"

using namespace expr;

namespace seahorn {
  namespace yices {


    term_t marshal_yices::encode_term(Expr exp, std::map<Expr, term_t> &cache){
      return NULL_TERM;
    }

    bool marshal_yices::encode_type(Expr exp, ytype_t &ytp){
      return false;
    }
      

    Expr marshal_yices::eval(Expr expr,  ExprFactory &efac, bool complete, model_t *model){
      return nullptr;
    }


  }
}


#endif

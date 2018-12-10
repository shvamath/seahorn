#pragma once

#include "ufo/ExprLlvm.hpp"
#include "Model.hpp"


namespace seahorn {
  namespace solver {

    typedef std::map<std::string, std::string> solver_options;

    class Solver {

    public:

      /** Result of the check */
      enum result {
                   /** Formula is satisfiable */
                   SAT,
                   /** Formula is unsatisfiable */
                   UNSAT,
                   /** The result is unknown */
                   UNKNOWN
      };


      /* assert a formula; sally includes a type here */
      virtual bool add(expr::Expr exp) = 0;

      /** Check for satisfiability */
      virtual result check() = 0;

      /** Push a context */
      virtual void push() = 0;

      /** Pop a context */
      virtual void pop() = 0;

      /** Get a model */
      virtual model get_model() = 0;



    };



  }

}

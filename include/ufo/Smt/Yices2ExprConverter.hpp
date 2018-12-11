#ifdef WITH_YICES2
#pragma once

#include <gmp.h>
#include "yices.h"

#include "ufo/ExprLlvm.hpp"
#include "Yices2.hpp"

using namespace expr;

namespace seahorn {
  namespace yices {


    class yices_impl;

    struct FailMarshal {
      template <typename C>
      static term_t marshal(Expr e, yices_impl &yices, C &cache) {
        llvm::errs() << "Cannot marshal: " << *e << "\n";
        assert(0);
        exit(1);
      }
    };

    struct FailUnmarshal {
      template <typename C>
      static Expr unmarshal(term_t yt, ExprFactory &efac, C &cache) {
        llvm::errs() << "Cannot unmarshal: " << std::string(yices_term_to_string(yt, 120, 40, 0))
                     << "\n";
        assert(0);
        exit(1);
      }
    };


    //XXX: why do I have to make this inline?
    inline void check_yices_error(int ret, const char* error_msg) {
      if (ret < 0) {
        char* error = yices_error_string();
        llvm::errs() << error_msg << ": " << error
                     << "\n";
        assert(0);
        exit(1);
      }
    }

    inline std::string get_function_name(Expr e){
      Expr fname = bind::fname(e);
      std::string sname = isOpX<STRING>(fname) ? getTerm<std::string>(fname) : lexical_cast<std::string>(*fname);
      return sname;
    }

    inline std::string get_variable_name(Expr e){
      Expr name = bind::name(e);
      std::string sname = isOpX<STRING>(name) ?
        // -- for variables with string names, use the names
        getTerm<std::string>(name)
        :
        // -- for non-string named variables use address
        "I" + lexical_cast<std::string, void *>(name.get());
      return sname;
    }


    //M is destined to be Failure!
    template <typename M> struct BasicExprMarshal {
      template <typename C>
      static term_t marshal(Expr e, yices_impl &yices, C &cache){

        assert(e);

        if (isOpX<TRUE>(e))
          return yices_true();
        if (isOpX<FALSE>(e))
          return yices_false();

        /** check the cache */
        {
          typename C::const_iterator it = cache.find(e);
          if (it != cache.end())
            return it->second;
        }

        term_t res = NULL_TERM;
        /* XXX: note we are returning type_t sometimes; so we will either need another routine; or else more clauses */
        /* IAM: I really think we should have marshalTerm and marshalType */
        if (bind::isBVar(e)) {
          // XXX:  this is destined to be bound!
          type_t vartype = marshal(bind::type(e), yices, cache);
          unsigned debruijn_index = bind::bvarId(e);
          // XXX: kosher to use the index as a name?
          std::string varname = "db_var" + std::to_string(debruijn_index);
          res = yices_new_variable(vartype);
          assert(res != NULL_TERM);
          int32_t errcode = yices_set_term_name(res, varname.c_str());
          check_yices_error(errcode, "BasicExprMarshal: bound variable case");
        }
        else if (isOpX<INT_TY>(e))
          res = yices_int_type();
        else if (isOpX<REAL_TY>(e))
          res = yices_real_type();
        else if (isOpX<BOOL_TY>(e))
          res = yices_bool_type();
        else if (isOpX<ARRAY_TY>(e)) {
          type_t domain = marshal(e->left(), yices, cache);
          type_t range = marshal(e->right(), yices, cache);
          type_t array_type = yices_function_type1(domain, range);
          // XXX: probably need to give this type a name and remember it for when we unmarshall
          // terms of this type
          // ZZZ: Another point in favor of marshalTerm and mashalType
          res = array_type;
        }
        else if (isOpX<BVSORT>(e)){
          res = yices_bv_type(bv::width(e));
          assert(res != NULL_TERM);
        }
        else if (isOpX<INT>(e)) {
          //XXX: following the Z3 case here.
          std::string sname = boost::lexical_cast<std::string>(e.get());
          res = yices_parse_float(sname.c_str());
          assert(res != NULL_TERM);

        }
        else if (isOpX<MPQ>(e)) {
          // GMP library
          const MPQ &op = dynamic_cast<const MPQ &>(e->op());
          res = yices_mpq(op.get().get_mpq_t());
          assert(res != NULL_TERM);
        }
        else if (isOpX<MPZ>(e)) {
          // GMP library
          const MPZ &op = dynamic_cast<const MPZ &>(e->op());
          res =  yices_mpz(op.get().get_mpz_t());
          assert(res != NULL_TERM);
        }
        else if (bv::is_bvnum(e)) {
          const MPZ &num = dynamic_cast<const MPZ &>(e->arg(0)->op());
          std::string val = boost::lexical_cast<std::string>(num.get());
          res = yices_parse_bvbin(val.c_str());
          assert(res != NULL_TERM);
        }
        else if (bind::isBoolVar(e) || bind::isIntVar(e) || bind::isRealVar(e)) {
          type_t var_type;
          if (bind::isBoolVar(e)){
            var_type = yices_bool_type();
          } else if (bind::isIntVar(e)) {
            var_type = yices_int_type();
          } else if (bind::isRealVar(e)) {
            var_type = yices_real_type();
          }

          std::string sname =  get_variable_name(e);
          const char* varname = sname.c_str();

          //Look and see if I have already made this variable! Though the cache should make this step redundant I guess.
          term_t var =  yices_get_term_by_name(varname);
          if (var != NULL_TERM){
            //check that we don't have name clashes
            type_t pvar_type = yices_type_of_term(var);
            assert(pvar_type == var_type);
            res = var;
          } else {
            res =  yices_new_uninterpreted_term(var_type);
            assert(res != NULL_TERM);
            //christian it
            int32_t errcode = yices_set_term_name(res, varname);
            check_yices_error(errcode, "BasicExprMarshal: variable case");
          }
        }
        /** function declaration */
        else if (bind::isFdecl(e)) {
          uint32_t arity = e->arity();

          std::vector<type_t> domain(arity);
          for (size_t i = 0; i < bind::domainSz(e); ++i) {
            type_t t_i = marshal(bind::domainTy(e, i), yices, cache);
            domain[i] = t_i;
          }

          type_t range = marshal(bind::rangeTy(e), yices, cache);

          type_t declaration_type = yices_function_type(e->arity(), &domain[0], range);
          assert(declaration_type != NULL_TYPE);

          res = yices_new_uninterpreted_term(declaration_type);
          assert(res != NULL_TERM);

          // Get the name of the function
          std::string sname = get_function_name(e);
          const char* function_name = sname.c_str();

          int32_t errcode = yices_set_term_name(res, function_name);
          check_yices_error(errcode, "BasicExprMarshal: fdecl case");

        }
        /** function application */
        else if (bind::isFapp(e)) {

          //get the name of the function
          std::string sname = get_function_name(e);
          term_t function =  yices_get_term_by_name(sname.c_str());
          assert(function != NULL_TERM);

          uint32_t arity = e->arity() - 1;
          std::vector<term_t> args(arity);
          unsigned pos = 0;
          for (ENode::args_iterator it = ++(e->args_begin()), end = e->args_end();
               it != end; ++it) {
            term_t arg_i = marshal(*it, yices, cache);
            assert(arg_i != NULL_TERM);
            args[pos++] = arg_i;
          }

          res = yices_application(function, arity, &args[0]);
          assert(res != NULL_TERM);

        }

        /** variable binders */
        else if (isOpX<FORALL>(e) || isOpX<EXISTS>(e) || isOpX<LAMBDA>(e)) {
          unsigned num_bound = bind::numBound(e);
          std::vector<term_t> bound_vars;
          bound_vars.reserve(num_bound);
          for (unsigned i = 0; i < num_bound; ++i) {
            //XXX: check this with JN
            term_t var_i = marshal(bind::decl(e, i), yices, cache);
            assert(var_i != NULL_TERM);
            bound_vars.push_back(var_i);
          }

          term_t body = marshal(bind::body(e), yices, cache);

          if (isOpX<FORALL>(e) ){
            res = yices_forall(num_bound, &bound_vars[0], body);
          } else if (isOpX<EXISTS>(e)) {
            res = yices_exists(num_bound, &bound_vars[0], body);
          } else if (isOpX<LAMBDA>(e) ) {
            res = yices_lambda(num_bound, &bound_vars[0], body);
          } else {
            assert(false);
          }

          assert(res != NULL_TERM);
        }

        // XXX: I have no idea why we are not caching composite terms.

        // -- cache the result for unmarshaling
        if (res) {
          cache.insert(typename C::value_type(e, res));
          return res;
        }

        int arity = e->arity();

        /** other terminal expressions */
        if (arity == 0) {
          // FAIL
          return M::marshal(e, yices, cache);
        }
        else if (arity == 1) {

          term_t arg = marshal(e->left(), yices, cache);

          if (isOpX<UN_MINUS>(e)) {
            return yices_neg(arg);
          }
          else if (isOpX<NEG>(e)) {
            return yices_not(arg);
          }
          else if (isOpX<ARRAY_DEFAULT>(e)) {
            //XXX: huh?
          }
          else if (isOpX<BNOT>(e)) {
            return yices_bvnot(arg);
          }
          else if (isOpX<BNEG>(e)) {
            return yices_bvneg(arg);
          }
          else if (isOpX<BREDAND>(e)) {
            return yices_redand(arg);
          }
          else if (isOpX<BREDOR>(e)) {
            return yices_redor(arg);
          }
          else {
            return M::marshal(e, yices, cache);
          }
        }
        else if (arity == 2){
          term_t t1 = marshal(e->left(), yices, cache);
          term_t t2 = marshal(e->right(), yices, cache);

          if (isOpX<AND>(e))
            res = yices_and2(t1, t2);
          else if (isOpX<OR>(e))
            res = yices_or2(t1, t2);
          else if (isOpX<IMPL>(e))
            res = yices_implies(t1, t2);
          else if (isOpX<IFF>(e))
            res = yices_iff(t1, t2);
          else if (isOpX<XOR>(e))
            res = yices_xor2(t1, t2);

          /** NumericOp */
          else if (isOpX<PLUS>(e))
            res = yices_add(t1, t2);
          else if (isOpX<MINUS>(e))
            res = yices_sub(t1, t2);
          else if (isOpX<MULT>(e))
            res = yices_mul(t1, t2);
          else if (isOpX<DIV>(e) || isOpX<IDIV>(e))
            //XXX: should idiv really be div?
            res = yices_division(t1, t2);
          else if (isOpX<MOD>(e))
            res = yices_imod(t1, t2);
          else if (isOpX<REM>(e))
            //XXX: res = yices_???div(t1, t2);  need to encode rem
            res = yices_idiv(t1, t2);
          /** Comparison Op */
          else if (isOpX<EQ>(e))
            res = yices_eq(t1, t2);
          else if (isOpX<NEQ>(e))
            res = yices_neq(t1, t2);
          else if (isOpX<LEQ>(e))
            res = yices_arith_leq_atom(t1, t2);
          else if (isOpX<GEQ>(e))
            res = yices_arith_geq_atom(t1, t2);
          else if (isOpX<LT>(e))
            res = yices_arith_lt_atom(t1, t2);
          else if (isOpX<GT>(e))
            res = yices_arith_gt_atom(t1, t2);

          /** Array Select */
          else if (isOpX<SELECT>(e))
            res = yices_application1(t1, t2);
          /** Array Const */
          else if (isOpX<CONST_ARRAY>(e)) {
            // XXX: make a lambda expression here?
            // or use update; but do we know the size of the domain?
            res = NULL_TERM;
          }

          /** Bit-Vectors */
          else if (isOpX<BSEXT>(e) || isOpX<BZEXT>(e)) {
            assert(yices_term_is_bitvector(t1));
            type_t bvt = yices_type_of_term(t1);
            uint32_t t1_sz = yices_term_bitsize(t1);
            unsigned t2_sz = bv::width(e->arg(1));
            assert(t1_sz > 0);
            assert(t1_sz < t2_sz);
            if (isOpX<BSEXT>(e)){
              yices_sign_extend(t1, t2_sz - t1_sz);
            }  else {
              yices_zero_extend(t1, t2_sz - t1_sz);
            }
          } else if (isOpX<BAND>(e))
            res = yices_bvand2(t1, t2);
          else if (isOpX<BOR>(e))
            res = yices_bvor2(t1, t2);
          else if (isOpX<BMUL>(e))
            res = yices_bvmul(t1, t2);
          else if (isOpX<BADD>(e))
            res = yices_bvadd(t1, t2);
          else if (isOpX<BSUB>(e))
            res = yices_bvsub(t1, t2);
          else if (isOpX<BSDIV>(e))
            res = yices_bvsdiv(t1, t2);
          else if (isOpX<BUDIV>(e))
            res = yices_bvdiv(t1, t2);
          else if (isOpX<BSREM>(e))
            res = yices_bvsrem(t1, t2);
          else if (isOpX<BUREM>(e))
            res = yices_bvrem(t1, t2);
          else if (isOpX<BSMOD>(e))
            res = yices_bvsmod(t1, t2);
          else if (isOpX<BULE>(e))
            res = yices_bvle_atom(t1, t2);
          else if (isOpX<BSLE>(e))
            res = yices_bvsle_atom(t1, t2);
          else if (isOpX<BUGE>(e))
            res = yices_bvge_atom(t1, t2);
          else if (isOpX<BSGE>(e))
            res = yices_bvsge_atom(t1, t2);
          else if (isOpX<BULT>(e))
            res = yices_bvlt_atom(t1, t2);
          else if (isOpX<BSLT>(e))
            res = yices_bvslt_atom(t1, t2);
          else if (isOpX<BUGT>(e))
            res = yices_bvgt_atom(t1, t2);
          else if (isOpX<BSGT>(e))
            res = yices_bvsgt_atom(t1, t2);
          else if (isOpX<BXOR>(e))
            res = yices_bvxor2(t1, t2);
          else if (isOpX<BNAND>(e))
            res = yices_bvnand(t1, t2);
          else if (isOpX<BNOR>(e))
            res = yices_bvnor(t1, t2);
          else if (isOpX<BXNOR>(e))
            res = yices_bvxnor(t1, t2);
          else if (isOpX<BCONCAT>(e))
            res = yices_bvconcat2(t1, t2);
          else if (isOpX<BSHL>(e))
            res = yices_bvshl(t1, t2);
          else if (isOpX<BLSHR>(e))
            res = yices_bvlshr(t1, t2);
          else if (isOpX<BASHR>(e))
            res = yices_bvashr(t1, t2);

          else
            return M::marshal(e, yices, cache);


        } else if (isOpX<BEXTRACT>(e)) {
          assert(bv::high(e) >= bv::low(e));
          term_t b = marshal(bv::earg(e), yices, cache);
          res = yices_bvextract(b, bv::low(e), bv::high(e));
        } else if (isOpX<AND>(e) || isOpX<OR>(e) || isOpX<ITE>(e) ||
                   isOpX<XOR>(e) || isOpX<PLUS>(e) || isOpX<MINUS>(e) ||
                   isOpX<MULT>(e) || isOpX<STORE>(e) || isOpX<ARRAY_MAP>(e)) {

          std::vector<term_t> args;
          for (ENode::args_iterator it = e->args_begin(), end = e->args_end();
               it != end; ++it) {
            term_t yt  = marshal(*it, yices, cache);
            args.push_back(yt);
          }

          if (isOp<ITE>(e)) {
            assert(e->arity() == 3);
            res = yices_ite(args[0], args[1], args[2]);
          } else if (isOp<AND>(e)) {
            res = yices_and(args.size(), &args[0]);
          }
          else if (isOp<OR>(e))
            res = yices_or(args.size(), &args[0]);
          else if (isOp<PLUS>(e))
            res = yices_sum(args.size(), &args[0]);
          else if (isOp<MINUS>(e)){
            term_t rhs = yices_sum(args.size() - 1, &args[1]);
            res = yices_sub(args[0], rhs);
          }
          else if (isOp<MULT>(e))
            res = yices_product(args.size(), &args[0]);
          else if (isOp<STORE>(e)) {
            assert(e->arity() == 3);
            res = yices_update1(args[0], args[1], args[2]);
          } else if (isOp<ARRAY_MAP>(e)) {
            //Z3_func_decl fdecl = reinterpret_cast<Z3_func_decl>(args[0]);
            //res = Z3_mk_map(ctx, fdecl, e->arity() - 1, &args[1]);
          }
        } else
          return M::marshal(e, yices, cache);

        assert(res != NULL_TERM);
        return res;
      }
    };

    template <typename M> struct BasicExprUnmarshal {
      template <typename C>
      static Expr unmarshal(term_t yt, yices_impl &yices, ExprFactory &efac, C &cache){

        term_constructor_t constructor = yices_term_constructor(yt);

        if (constructor ==  YICES_CONSTRUCTOR_ERROR){

          assert(0 && "Not a yices term");
        }

        /* atomic terms */
        switch(constructor){
        case YICES_BOOL_CONSTANT: {
          int32_t value;
          int32_t errcode = yices_bool_const_value(yt, &value);
          assert(errcode != -1);
          return value == 1 ? mk<TRUE>(efac) : mk<FALSE>(efac);
        }
        case YICES_ARITH_CONSTANT: {
          mpq_t q;
          mpq_init(q);
          int32_t errcode = yices_rational_const_value(yt, q);
          //XXX: do something with q
          mpq_clear(q);
          break;
        }
        case YICES_BV_CONSTANT: {
          uint32_t n = yices_term_bitsize(yt);
          int32_t vals[n];
          int32_t errcode = yices_bv_const_value(yt, vals);
          //XXX: do something with n and vals
          break;
        }
        case YICES_SCALAR_CONSTANT: {
          assert(0 && "Not expecting a scalar");
          break;
        }
        case YICES_VARIABLE: {

          break;
        }
        case YICES_UNINTERPRETED_TERM: {

          break;
        }
        default:
          break;
        }

        /* composite terms */


        int32_t num_children = yices_term_num_children(yt);

        if ( yices_term_is_projection(yt) ){



        } else {

          std::vector<term_t> args;
          for(int i = 0; i < num_children; i++){
            term_t yt_i = yices_term_child(yt, i);
            Expr a_i = unmarshal(yt_i, yices, efac, cache);
            args.push_back(yt_i);
          }

          if (constructor == YICES_ITE_TERM ){
          }
          else if (constructor == YICES_APP_TERM ){
          }
          else if (constructor == YICES_UPDATE_TERM ){
          }
          else if (constructor == YICES_TUPLE_TERM ){
          }
          else if (constructor == YICES_EQ_TERM ){
          }
          else if (constructor == YICES_DISTINCT_TERM ){
          }
          else if (constructor == YICES_FORALL_TERM ){
          }
          else if (constructor == YICES_LAMBDA_TERM ){
          }
          else if (constructor == YICES_NOT_TERM ){
          }
          else if (constructor == YICES_OR_TERM ){
          }
          else if (constructor == YICES_XOR_TERM ){
          }
          else if (constructor == YICES_BV_ARRAY ){
          }
          else if (constructor == YICES_BV_DIV ){
          }
          else if (constructor == YICES_BV_REM ){
          }
          else if (constructor == YICES_BV_SDIV ){
          }
          else if (constructor == YICES_BV_SREM ){
          }
          else if (constructor == YICES_BV_SMOD ){
          }
          else if (constructor == YICES_BV_SHL ){
          }
          else if (constructor == YICES_BV_LSHR ){
          }
          else if (constructor == YICES_BV_ASHR ){
          }
          else if (constructor == YICES_BV_GE_ATOM ){
          }
          else if (constructor == YICES_BV_SGE_ATOM ){
          }
          else if (constructor == YICES_ARITH_GE_ATOM ){
          }
          else if (constructor == YICES_ARITH_ROOT_ATOM ){
          }
          else if (constructor == YICES_ABS ){
          }
          else if (constructor == YICES_CEIL ){
          }
          else if (constructor == YICES_FLOOR ){
          }
          else if (constructor == YICES_RDIV ){
          }
          else if (constructor == YICES_IDIV ){
          }
          else if (constructor == YICES_IMOD ){
          }
          else if (constructor == YICES_IS_INT_ATOM ){
          }
          else if (constructor == YICES_DIVIDES_ATOM ){
          }
          else if (constructor == YICES_SELECT_TERM ){
          }
          else if (constructor == YICES_BIT_TERM ){
          }
          else if (constructor == YICES_BV_SUM ){
          }
          else if (constructor == YICES_ARITH_SUM ){
          }
          else if (constructor == YICES_POWER_PRODUCT ){

          }
          else {


          }

        }

        return 0;
      }
    };

  }
}
#endif

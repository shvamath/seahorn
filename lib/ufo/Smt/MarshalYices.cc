#ifdef WITH_YICES2

#include "ufo/Smt/MarshalYices.hpp"

using namespace expr;

namespace seahorn {
  namespace yices {

    static term_t encode_term_fail(Expr e, const char* error_msg) {
      llvm::errs() << "encode_term: failed on "
		   << *e
		   << "\n"
		   << error_msg
		   << "\n";
      assert(0);
      exit(1);
    }

    static std::string get_function_name(Expr e){
      Expr fname = bind::fname(e);
      std::string sname = isOpX<STRING>(fname) ? getTerm<std::string>(fname) : lexical_cast<std::string>(*fname);
      return sname;
    }

    static std::string get_variable_name(Expr e){
      Expr name = bind::name(e);
      std::string sname = isOpX<STRING>(name) ?
        // -- for variables with string names, use the names
        getTerm<std::string>(name)
        :
        // -- for non-string named variables use address
        "I" + lexical_cast<std::string, void *>(name.get());
      return sname;
    }


    bool marshal_yices::encode_type(Expr e, ytype_t &ytp){
      type_t res = NULL_TYPE;
      bool is_array = false;
      if (isOpX<INT_TY>(e))
	res = yices_int_type();
      else if (isOpX<REAL_TY>(e))
	res = yices_real_type();
      else if (isOpX<BOOL_TY>(e))
	res = yices_bool_type();
      else if (isOpX<ARRAY_TY>(e)) {
	ytype_t domain, range;
	if(!encode_type(e->left(), domain)){
	  return false;
	}
	if(!encode_type(e->right(), range)){
	  return false;
	}
	type_t array_type = yices_function_type1(domain.ytype, range.ytype);
	res = array_type;
	is_array = true;
      }
      else if (isOpX<BVSORT>(e)){
	res = yices_bv_type(bv::width(e));
	assert(res != NULL_TERM);
      } else {
	llvm::errs() << "Unhandled sort: " << *e << "\n";
      }

      bool success = (res != NULL_TYPE);
      
      if(success){
	ytp.ytype = res;
	ytp.is_array = is_array;
      }
      return success;
    }

    term_t marshal_yices::encode_term(Expr e, std::map<Expr, term_t> &cache){

      assert(e);

      if (isOpX<TRUE>(e))
	return yices_true();
      if (isOpX<FALSE>(e))
	return yices_false();

      /** check the cache */
      {
	auto it = cache.find(e);
	if (it != cache.end())
	  return it->second;
      }

      term_t res = NULL_TERM;
      
      if (bind::isBVar(e)) {
	ytype_t vartype;
	if(!encode_type(bind::type(e), vartype)){
	  return res;
	}
	unsigned debruijn_index = bind::bvarId(e);
	// XXX: kosher to use the index as a name? NO
	std::string varname = "db_var" + std::to_string(debruijn_index);
	res = yices_new_variable(vartype.ytype);
	assert(res != NULL_TERM);
	int32_t errcode = yices_set_term_name(res, varname.c_str());
	if(errcode < 0){
	  encode_term_fail(e, yices_error_string());
	}
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
	  if(errcode < 0){
	    encode_term_fail(e, yices_error_string());
	  }
	}
      }
      /** function declaration */
      else if (bind::isFdecl(e)) {
	uint32_t arity = e->arity();

	std::vector<type_t> domain(arity);
	for (size_t i = 0; i < bind::domainSz(e); ++i) {
	  ytype_t yt_i;
	  if(!encode_type(bind::domainTy(e, i), yt_i)){
	    encode_term_fail(e, "fdecl domain encode error");
	  }
	  domain[i] = yt_i.ytype;
	}

	ytype_t yrange;
	if(! encode_type(bind::rangeTy(e), yrange) ){
	  encode_term_fail(e, "fdecl range encode error");
	}
	type_t range = yrange.ytype;

	type_t declaration_type = yices_function_type(e->arity(), &domain[0], range);
	assert(declaration_type != NULL_TYPE);

	res = yices_new_uninterpreted_term(declaration_type);
	assert(res != NULL_TERM);

	// Get the name of the function
	std::string sname = get_function_name(e);
	const char* function_name = sname.c_str();

	int32_t errcode = yices_set_term_name(res, function_name);
	if(errcode < 0){
	  encode_term_fail(e, yices_error_string());
	}
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
	  term_t arg_i = encode_term(*it, cache);
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
	  term_t var_i = encode_term(bind::decl(e, i), cache);
	  assert(var_i != NULL_TERM);
	  bound_vars.push_back(var_i);
	}

	term_t body = encode_term(bind::body(e), cache);

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
	cache[e] = res;
	return res;
      }

      int arity = e->arity();

      /** other terminal expressions */
      if (arity == 0) {
	// FAIL
	return encode_term_fail(e, "zero arity unexpected");
      }
      else if (arity == 1) {

	term_t arg = encode_term(e->left(), cache);

	if (isOpX<UN_MINUS>(e)) {
	  return yices_neg(arg);
	}
	else if (isOpX<NEG>(e)) {
	  return yices_not(arg);
	}
	else if (isOpX<ARRAY_DEFAULT>(e)) {
	  //XXX: huh?
	  //BD and JN throw an exception here
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
	  return encode_term_fail(e, "unhandled arity 1 case");
	}
      }
      else if (arity == 2){
	term_t t1 = encode_term(e->left(), cache);
	term_t t2 = encode_term(e->right(), cache);

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
	else if (isOpX<REM>(e)){
	  //XXX: res = yices_??? div(t1, t2);  need to encode rem
	  //throw an exception for the time being.
	  //http://smtlib.cs.uiowa.edu/logics-all.shtml#QF_BV
	  llvm::errs() << "rem on integers needs to be suffered through: " << *e << "\n";
	  assert(0);
	  exit(1);
	}
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
	  return encode_term_fail(e, "unhandle binary case");


      } else if (isOpX<BEXTRACT>(e)) {
	assert(bv::high(e) >= bv::low(e));
	term_t b = encode_term(bv::earg(e), cache);
	res = yices_bvextract(b, bv::low(e), bv::high(e));
      } else if (isOpX<AND>(e) || isOpX<OR>(e) || isOpX<ITE>(e) ||
		 isOpX<XOR>(e) || isOpX<PLUS>(e) || isOpX<MINUS>(e) ||
		 isOpX<MULT>(e) || isOpX<STORE>(e) || isOpX<ARRAY_MAP>(e)) {

	std::vector<term_t> args;
	for (ENode::args_iterator it = e->args_begin(), end = e->args_end();
	     it != end; ++it) {
	  term_t yt  = encode_term(*it, cache);
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
	  //BD says can't really handle this case
	}
      } else
	return encode_term_fail(e, "Unhandled case");

      assert(res != NULL_TERM);
      return res;
    }

    //XXX: kind of ugly to have the cache here. Maybe we should push it into the yices context and pass that around.
    //     would be more uniform with the Z3 case.  WHY NOT MAKE IT STATIC SOMEWHERE?
    Expr marshal_yices::eval(Expr expr,  ExprFactory &efac, std::map<Expr, term_t> &cache, bool complete, model_t *model){

      term_t yt_var = encode_term(expr, cache);

      if(yt_var == NULL_TERM){
	return nullptr;
      }

      type_t ytyp_var =  yices_type_of_term(yt_var);

      if(ytyp_var == NULL_TYPE){
	return nullptr;
      }

      yval_t yval;
      int32_t errcode = yices_get_value(model, yt_var, &yval);

      if(errcode == -1){
	return nullptr;
      }

      
      Expr res = nullptr;
      /* atomic terms */
      switch(yval.node_tag){
      case YICES_BOOL_CONSTANT: {
	int32_t value;
	int32_t errcode = yices_val_get_bool(model, &yval, &value);
	if(errcode == -1){
	  return nullptr;
	}
	res = value == 1 ? mk<TRUE>(efac) : mk<FALSE>(efac);
      }
      case YICES_ARITH_CONSTANT: {
	mpq_t q;
	mpq_init(q);
	int32_t errcode = yices_val_get_mpq(model, &yval, q);
	mpq_class qpp(q);
	res = mkTerm(qpp, efac);
	mpq_clear(q);
	break;
      }
      case YICES_BV_CONSTANT: {
	uint32_t n = yices_val_bitsize(model, &yval);
	if (n == 0){
	  return nullptr;
	}
	int32_t vals[n];
	int32_t errcode = yices_val_get_bv(model, &yval, vals);
	char cvals[n];
	for(int32_t i = 0; i < n; i++){
	  cvals[i] = vals[i] ? '1' : '0';
	}
	std::string snum(cvals);
	res = bv::bvnum(mpz_class(snum), n, efac);
	break;
      }
      case YICES_SCALAR_CONSTANT: {
	assert(0 && "Not expecting a scalar");
	break;
      }
      case YICES_VARIABLE: {
	assert(0 && "Not expecting a scalar");
	break;
      }
      case YICES_UNINTERPRETED_TERM: {
	assert(0 && "Not expecting a scalar");
	break;
      }
      default:
	assert(0 && "Not expecting a scalar");
	break;
      }

      /* composite terms


      int32_t num_children = yices_term_num_children(yt);

      if ( yices_term_is_projection(yt) ){



      } else {

	std::vector<term_t> args;
	for(int i = 0; i < num_children; i++){
	  term_t yt_i = yices_term_child(yt, i);
	  Expr a_i = unmarshal(yt_i, yices, efac, cache, arrays);
	  args.push_back(yt_i);
	}


      }
      */
      
      return nullptr;
    }


  }
}


#endif

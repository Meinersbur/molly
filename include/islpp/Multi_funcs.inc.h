
  static inline bool plainIsEqual(const Multi<ISLPP_ELPP> &maff1, const Multi<ISLPP_ELPP> &maff2) { return isl_multi_aff_plain_is_equal(maff1.keep(), maff2.keep()); }
  static inline Multi<ISLPP_ELPP> add(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_add(maff1.take(), maff2.take())); }
  static inline Multi<ISLPP_ELPP> sub(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_sub(maff1.take(), maff2.take())); }

  static inline Multi<ISLPP_ELPP> rangeSplice(Multi<ISLPP_ELPP> &&maff1, unsigned pos, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_range_splice(maff1.take(), pos, maff2.take())); }
  static inline Multi<ISLPP_ELPP> splice(Multi<ISLPP_ELPP> &&maff1, unsigned in_pos, unsigned out_pos, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_splice(maff1.take(), in_pos, out_pos, maff2.take())); }

  static inline Multi<ISLPP_ELPP> rangeProduct(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_range_product(maff1.take(), maff2.take())); }
  static inline Multi<ISLPP_ELPP> flatRangeProduct(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_flat_range_product(maff1.take(), maff2.take())); }
  static inline Multi<ISLPP_ELPP> product(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_product(maff1.take(), maff2.take())); }

  static inline Multi<ISLPP_ELPP> pullbackMultiAff(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Multi<ISLPP_ELPP>::wrap(isl_multi_aff_pullback_multi_aff(maff1.take(), maff2.take())); }

  static inline Set lexLeSet(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Set::wrap(isl_multi_aff_lex_le_set(maff1.take(), maff2.take())); }
  static inline Set lexGeSet(Multi<ISLPP_ELPP> &&maff1, Multi<ISLPP_ELPP> &&maff2) { return Set::wrap(isl_multi_aff_lex_ge_set(maff1.take(), maff2.take())); }


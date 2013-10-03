#include "islpp_impl_common.h"
#include "islpp/Int.h"

#include <isl/deprecated/int.h>

using namespace isl;


void isl::Int::print(llvm::raw_ostream &out, int base /*= 10*/) const {
  char *s = mpz_get_str(0, base, keep());
  out << s;
  isl_int_free_str(s);
}

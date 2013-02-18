#include "islpp/Ctx.h"

#include "islpp/BasicSet.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Constraint.h"
#include "islpp/Printer.h"

#include <isl/ctx.h>
#include <isl/options.h>
#include <isl/space.h>
#include <isl/set.h>
#include <isl/printer.h>

using namespace isl;
using namespace std;


void Ctx:: operator delete(void* ptr) {
  isl_ctx *ctx = reinterpret_cast<isl_ctx*>(ptr); 
  if (ctx)
    isl_ctx_free(ctx);
}

Ctx::~Ctx() {
  return;
  isl_ctx *ctx = reinterpret_cast<isl_ctx*>(this); 
  if (ctx)
    isl_ctx_free(ctx);
}

Ctx *Ctx::create() {
  auto result = wrap(isl_ctx_alloc());
#ifndef NDEBUG
  result->setOnError(OnError::Abort);
#endif
  return result;
}



isl_error Ctx::getLastError() const {
  return isl_ctx_last_error(keep());
}


void Ctx::resetLastError() {
  isl_ctx_reset_error(keep());
}


void Ctx::setOnError(OnErrorEnum val){
  isl_options_set_on_error(keep(), val);
}


OnErrorEnum Ctx::getOnError() const {
  return static_cast<OnErrorEnum>(isl_options_get_on_error(keep()));
}

Printer Ctx::createPrinterToFile(FILE *file) {
  return Printer::wrap(isl_printer_to_file(keep(), file), false);
}

Printer Ctx:: createPrinterToFile(const char *filename) {
  FILE *file = fopen(filename, "w");
  assert(file);
  Printer result = Printer::wrap(isl_printer_to_file(keep(), file), true);
  return result;
}

Printer Ctx::createPrinterToStr(){
  return Printer::wrap(isl_printer_to_str(keep()), false);
}


Space Ctx::createSpace(unsigned nparam, unsigned dim) {
  isl_space *space = isl_space_set_alloc(keep(), nparam, dim);
  return Space::wrap(space);
}


BasicSet Ctx::createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths) {
  auto dims = lengths.size();
  Space space = createSpace(0, dims);
  BasicSet set = BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = Constraint::createInequality(space.copy());
    ge.setConstant(0);
    ge.setCoefficient(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = Constraint::createInequality(move(space));
    lt.setConstant(lengths[d]);
    lt.setCoefficient(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }
  return set;
}


BasicSet Ctx::readBasicSet(const char *str) {
  return BasicSet::wrap(isl_basic_set_read_from_str(keep(), str));
}

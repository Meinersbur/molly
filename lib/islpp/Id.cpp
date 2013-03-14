#include "islpp/Id.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"

#include <isl/id.h>
#include <llvm/Support/raw_ostream.h>

using namespace isl;


isl_id *Id::takeCopyOrNull() const {
  return isl_id_copy(id);
}


void Id::giveOrNull(isl_id *id) { 
  if (this->id && this->id!=id) 
    isl_id_free(this->id); 
  this->id = id;
}


Id::~Id() {
  if (this->id) isl_id_free(this->id);
#ifndef NDEBUG
  this->id = NULL;
#endif
}


Id Id::create(Ctx *ctx, const char *name, void *user) {
  return Id::wrap(isl_id_alloc(ctx->keep(), name, user));
}


Id Id::createAndFreeUser(Ctx *ctx, const char *name, void *user) {
  isl_id *result = isl_id_alloc(ctx->keep(), name, user);
  result = isl_id_set_free_user(result, &free);
  return Id::wrap(result);
}


void Id::print(llvm::raw_ostream &out) const {
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


std::string Id::toString() const {
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return buf;
}


void Id::dump() const {
  print(llvm::errs());
}


Ctx *Id::getCtx() const { 
  return Ctx::wrap(isl_id_get_ctx(keep()));
}


void *Id::getUser() const {
  return isl_id_get_user(keep());
}


const char *Id::getName() const { 
  return isl_id_get_name(keepOrNull()); 
}


void Id::setFreeUser( void (*freefunc)(void *))  { 
  give(isl_id_set_free_user(take(),freefunc)); 
}

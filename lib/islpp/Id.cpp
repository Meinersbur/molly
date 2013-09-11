#include "islpp_impl_common.h"
#include "islpp/Id.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"

#include <llvm/Support/raw_ostream.h>
#include <isl/id.h>

using namespace isl;
using namespace llvm;

//namespace isl {

void Id::release() {
  isl_id_free(takeOrNull());
}


Id::StructTy *Id::addref() const { 
  return isl_id_copy(keepOrNull()); 
}


Ctx *Id::getCtx() const { 
  return Ctx::enwrap(isl_id_get_ctx(keep()));
}


void Id::print(llvm::raw_ostream &out) const {
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}
void Id::dump() const { isl_id_dump(keep()); }


Id Id::create(Ctx *ctx, const char *name, void *user) {
  return Id::enwrap(isl_id_alloc(ctx->keep(), name, user));
}


const char *Id::getName() const { 
  return isl_id_get_name(keepOrNull()); 
}
void *Id::getUser() const { 
  return isl_id_get_user(keep()); 
}


Id Id::setFreeUser(void (*freefunc)(void *)) { 
  return Id::enwrap(isl_id_set_free_user(keep(), freefunc)); 
}
void Id::setFreeUser_inline(void (*freefunc)(void *)) {
  give(isl_id_set_free_user(keep(), freefunc)); 
}
Id Id::setFreeUser_consume(void (*freefunc)(void *)) { 
  return Id::enwrap(isl_id_set_free_user(take(), freefunc));
}
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
Id Id::setFreeUser(void (*freefunc)(void *)) && { 
  return Id::enwrap(isl_id_set_free_user(take(), freefunc));
}
#endif

Id isl::setFreeUser(const Id &id, void (*freefunc)(void *)) {
  return Id::enwrap(isl_id_set_free_user(id.keep(), freefunc)); 
}
Id isl::setFreeUser(Id &&id, void (*freefunc)(void *)) {
  return Id::enwrap(isl_id_set_free_user(id.take(), freefunc)); 
}


llvm::DenseMapInfo<isl::Id>::KeyInitializer::KeyInitializer() : ctx(isl::Ctx::create()) {
  empty = ctx->createId("empty", &empty);
  tombstone = ctx->createId("tombstone", &tombstone);
}

llvm::DenseMapInfo<isl::Id>::KeyInitializer::~KeyInitializer() {
  empty.reset();
  tombstone.reset();
  delete ctx;
}

llvm::DenseMapInfo<isl::Id>::KeyInitializer llvm::DenseMapInfo<isl::Id> ::keys;


//} // namespace isl

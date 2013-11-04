#include "islpp/Constraint.h"

#include "islpp/LocalSpace.h"
#include "islpp/Int.h"
#include "islpp/Aff.h"
#include "islpp/Printer.h"
#include "islpp/Ctx.h"
#include "islpp/Dim.h"
#include "islpp/BasicSet.h"

#include <isl/constraint.h>
#include <llvm/Support/ErrorHandling.h>
#include <functional>

using namespace isl;
using namespace std;

#if 0
isl_constraint *Constraint::takeCopy() const {
  return isl_constraint_copy(constraint);
}


void Constraint::give(isl_constraint *constraint) {
  if (this->constraint)
    isl_constraint_free(this->constraint); 
  this->constraint = constraint;
}
#endif

#if 0
Constraint::~Constraint() {
  if (this->constraint) 
    isl_constraint_free(this->constraint); 
}
#endif

Constraint Constraint::createEquality(LocalSpace space) {
  return Constraint::enwrap(isl_equality_alloc(space.take()));
}


Constraint Constraint::createInequality(LocalSpace space) {
  return Constraint::enwrap(isl_inequality_alloc(space.take()));
}


void Constraint::print(llvm::raw_ostream &out) const { 
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}

#if 0
std::string Constraint::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}
#endif

#if 0
void Constraint::dump() const { 
  print(llvm::errs());
}
#endif

struct indent_helper {
  int count;
  indent_helper(int count) : count(count) {}
};
static llvm::raw_ostream  &operator<<(llvm::raw_ostream &os, const indent_helper &helper) {
  for (int i = 0; i < helper.count; i+=1) {
    os << "  ";
  }
  return os;
}

static indent_helper indent(int count) {
  return indent_helper(count);
}


static const char * dimtype2str(isl_dim_type type, bool isSet = false) {
  switch (type) {
  case isl_dim_cst:
    return "isl_dim_cst";
  case isl_dim_param:
    return "isl_dim_param";
  case isl_dim_in:
    return "isl_dim_in";
  case isl_dim_out:
    return isSet ? "isl_dim_set" : "isl_dim_out";
  case isl_dim_div:
    return "isl_dim_div";
  default:
    llvm_unreachable("No such isl_dimt_type");
  }
}


static void printPerDim(llvm::raw_ostream &out, int indentcount, const char *propname, LocalSpace space, std::function<void(llvm::raw_ostream &, isl_dim_type, int)> &&fn) {
  out <<  indent(indentcount) << propname << " = [\n";

  {
    auto type = isl_dim_cst;
    auto len = space.dim(type);
    if (len > 0) {
      out << indent(indentcount+1) << "[cst] = [";
      for (auto pos = len-len; pos < len; pos+=1) {
        if (pos!=0)
          out << ", ";
        fn(out, type, pos);
      }
      out << indent(indentcount+1) << "]\n";
    }
  }

  {
    auto type = isl_dim_param;
    auto len = space.dim(type);
    if (len > 0) {
      out << indent(indentcount+1) << "[param] = [";
      for (auto pos = len-len; pos < len; pos+=1) {
        if (pos!=0)
          out << ", ";
        fn(out, type, pos);
      }
      out << indent(indentcount+1) << "]\n";
    }
  }

  {
    auto type = isl_dim_in;
    auto len = space.dim(type);
    if (len > 0) {
      out << indent(indentcount+1) << "[in] = [";
      for (auto pos = len-len; pos < len; pos+=1) {
        if (pos!=0)
          out << ", ";
        fn(out, type, pos);
      }
      out << indent(indentcount+1) << "]\n";
    }
  }

  {
    auto type = isl_dim_out;
    auto len = space.dim(type);
    if (len > 0) {
      out << indent(indentcount+1) << "[out] = [";
      for (auto pos = len-len; pos < len; pos+=1) {
        if (pos!=0)
          out << ", ";
        fn(out, type, pos);
      }
      out << indent(indentcount+1) << "]\n";
    }
  }

  {
    auto type = isl_dim_div;
    auto len = space.dim(type);
    if (len > 0) {
      out << indent(indentcount+1) << "[div] = [";
      for (auto pos = len-len; pos < len; pos+=1) {
        if (pos!=0)
          out << ", ";
        fn(out, type, pos);
      }
      out << indent(indentcount+1) << "]\n";
    }
  }

  out <<  indent(indentcount) << "]\n";
}


BasicSet Constraint::toBasicSet() const {
  auto result = getLocalSpace().universeBasicSet();
  result.addConstraint_inplace(*this);
  return result;
}


void Constraint::printProperties(llvm::raw_ostream &out, int depth, int indentcount) const {
  if (depth > 0) {
    auto localSpace = getLocalSpace();
    // auto self = this;

    out << "(Constraint) \"";
    print(out);
    out << "\"\n";
    out << indent(indentcount) << "localSpace = {...}\n";
    out << indent(indentcount) << "isEquality = " << isEquality() << "\n";
    printPerDim(out, indentcount, "isLowerBound", localSpace, [this](llvm::raw_ostream &out, isl_dim_type type, int pos) { out << this->isLowerBound(type, pos); } );
    printPerDim(out, indentcount, "isUpperBound", localSpace, [this](llvm::raw_ostream &out, isl_dim_type type, int pos) { out << this->isUpperBound(type, pos); } );
    out << indent(indentcount) << "Constant = " << getConstant() << "\n";
    printPerDim(out, indentcount, "Coefficient", localSpace, [this](llvm::raw_ostream &out, isl_dim_type type, int pos) { out << this->getCoefficient(type, pos); } );
    printPerDim(out, indentcount, "InvolvesDims", localSpace, [this](llvm::raw_ostream &out, isl_dim_type type, int pos) { out << this->involvesDims(type, pos, 1); } );
    auto divdims = localSpace.dim(isl_dim_div);
    for (auto pos = divdims-divdims; pos<divdims;pos+=1) {
      out << indent(indentcount) << "Div[" << pos << "] = {\n";
      getDiv(pos).printProperties(out, depth-1, indentcount+1);
      out << indent(indentcount) << "}\n";
    }
    //printPerDim(out, indentcount, "Bound", localSpace, [this](llvm::raw_ostream &out, isl_dim_type type, int pos) { this->getBound(type, pos); });
    out << indent(indentcount) << "Aff = {\n";
    getAff().printProperties(out, depth-1,indentcount+1);
    out << indent(indentcount) << "}\n";
  } else {
    out << "...";
  }
}

#if 0
Ctx *Constraint::getCtx() const {
  return Ctx::wrap(isl_constraint_get_ctx(keep()));
}
#endif

LocalSpace Constraint::getLocalSpace() const {
  return LocalSpace::enwrap(isl_constraint_get_local_space(keep()));
}

void  Constraint::setConstant_inplace(const Int &v) ISLPP_INPLACE_FUNCTION {
  give(isl_constraint_set_constant(take(), v.keep() ) );
}

void Constraint::setConstant_inplace(int v) ISLPP_INPLACE_FUNCTION {
  give(isl_constraint_set_constant_si(take(), v));
}


void Constraint::setCoefficient_inplace(isl_dim_type type, int pos, const Int & v) ISLPP_INPLACE_FUNCTION {
  give(isl_constraint_set_coefficient(take(), type, pos, v.keep()));
}


void Constraint::setCoefficient_inplace( isl_dim_type type, int pos, int v) ISLPP_INPLACE_FUNCTION {
  give(isl_constraint_set_coefficient_si(take(), type, pos, v));
}


bool Constraint::isEquality() const{
  return isl_constraint_is_equality(keep());
}
bool Constraint::isLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_constraint_is_lower_bound(keep(), type, pos);
}
bool Constraint::isUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_constraint_is_upper_bound(keep(), type, pos);
}
Int Constraint::getConstant() const {
  Int result;
  isl_constraint_get_constant(keep(), result.change());
  return result;
}
Int Constraint::getCoefficient(isl_dim_type type, int pos) const {
  Int result;
  isl_constraint_get_coefficient(keep(), type, pos, result.change());
  return result;
}
bool Constraint::involvesDims(isl_dim_type type, unsigned first, unsigned n) const {
  return isl_constraint_involves_dims(keep(), type, first, n);
}
Aff Constraint::getDiv(int pos) const {
  return Aff::enwrap(isl_constraint_get_div(keep(), pos));
}
const char *Constraint::getDimName(isl_dim_type type, unsigned pos) const {
  return isl_constraint_get_dim_name(keep(), type, pos); 
}
Aff Constraint::getBound(isl_dim_type type, int pos) const{
  return Aff::enwrap(isl_constraint_get_bound(keep(), type, pos));
}
Aff Constraint::getAff() const {
  return Aff::enwrap(isl_constraint_get_aff(keep()));
}

Constraint isl::makeLtConstaint(const Constraint &lhs, const Constraint &rhs) {
  return makeGeConstraint( rhs, lhs);
}

Constraint isl::makeLrConstaint(const Constraint &lhs, const Constraint &rhs) {
  return makeGtConstraint( rhs, lhs);
}

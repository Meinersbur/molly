#include "islpp_impl_common.h"
#include "islpp/Expr.h"

#include "islpp/Int.h"
#include "islpp/Id.h"
#include "islpp/LocalSpace.h"
#include "islpp/Dim.h"
#include "islpp/Constraint.h"
#include <isl/space.h>
//#include <llvm/ADT/SmallVector.h>
#include <vector>

using namespace std;

namespace isl {

  class ExprImpl {
  private:
    //Ctx *ctx;
    LocalSpace space;
    unsigned refs;

    //TODO: malloc ExprImpl + the coeffs together
    vector<Int> coeffs; // support Val?

  private:
    ~ExprImpl() {}

  public:
    explicit ExprImpl(LocalSpace &&ls) : space(std::move(ls)), refs(1), coeffs(this->space.getAllDimCount()/*ls has been moved from*/) {}
    explicit ExprImpl(const LocalSpace &ls) : space(space), refs(1), coeffs(ls.getAllDimCount()) {}
    /* implicit */ ExprImpl(const ExprImpl &that) : space(that.space), refs(1), coeffs(that.coeffs) {}

    void release() {
      assert(this);
      assert(refs > 0);

      refs -= 1;
      if (refs == 0) 
        delete this;
    }

    void addRef() {
      assert(this);
      assert(refs >= 0);
      assert(refs < std::numeric_limits<decltype(refs)>::max());

      refs += 1;
    }

    ExprImpl *cow() {
      assert(this);
      assert(refs > 0);
      if (refs == 1)
        return this;

      auto result = new ExprImpl(*this);
      refs -= 1;
      return result;
    }

    void modifying() {
      assert(refs == 1);
    }

    Ctx *getCtx() const { return space.getCtx(); }
    LocalSpace getLocalSpace() const { return space; } 

    Int getCoefficient(isl_dim_type type, unsigned pos) const {
      auto offset = dimToIdx(type, pos);
      return coeffs[offset];
    }

    unsigned dimToIdx(isl_dim_type type, unsigned pos) const {
      if (type==isl_dim_cst) {
        assert(pos==0);
        return 0;
      }

      unsigned result = 1;
      assert(pos < space.dim(type));
      for (int i = isl_dim_param; i < type; i+=1) {
        result += space.dim((isl_dim_type)i);
      }
      result += pos;
      return result;
    }

    void idxToDim(unsigned idx, isl_dim_type &type, unsigned &pos) const {
      if (idx==0) {
        type = isl_dim_cst;
        pos = 0;
        return;
      }

      type = isl_dim_param;
      pos = idx-1;
      while (true) {
        assert(type < isl_dim_all);
        auto dims = space.dim(type);
        if (pos < dims) 
          return;

        type = (isl_dim_type)(type + 1);
        pos -= dims;
      }
    }

    void print (raw_ostream &os) const {
      auto space = getLocalSpace();

      bool first = true;
      for (auto it = space.dim_begin(), end = space.dim_end(); it!=end; ++it) {
        auto dim = *it;
        auto type = dim.getType();
        if (type == isl_dim_cst)
          continue;
        auto pos = dim.getPos();
        auto coeff = getCoefficient(type,pos);

        if (coeff == 0)
          continue;

        if (!first) {
          os << " + ";
        }

        if (coeff != 1) {
          os << coeff;
          //if (type != isl_dim_cst)
          os << "*";
        }

        switch (type) {
          //case isl_dim_cst:
          //  break;
        case isl_dim_param: {
          auto name = dim.getName();
          if (name) {
            os << name;
          } else {
            os << "p" << pos;
          }
          break;
                            }
        case isl_dim_in:
          os << "i" << pos;
          break;
        case isl_dim_out:
          os << "o" << pos;
          break;
        case isl_dim_div:
          os << "d" << pos;
          break;
        default:
          os << "(" << type << "," << pos << ")";
          break;
        }

        first = false;
      }

      auto cst = getCoefficient(isl_dim_cst,0);
      if (cst == 0) {
        if (first)
          os << '0';
      } else {
        if (!first) 
          os << " + ";
        os << cst;
      }
    }

    void add(const Int &val) {
      assert(dimToIdx(isl_dim_cst, 0) == 0);
      this->coeffs[0] += val;
    }

    void add(isl_dim_type type, unsigned pos, const Int &coeff) {
      modifying();
      coeffs[dimToIdx(type, pos)] += coeff;
    }

    void add(const ExprImpl *that) {
      assert(isEqual(this->space , that->space));
      assert(this->coeffs.size() == that->coeffs.size());
      modifying();

      auto dims = coeffs.size();
      for (auto i = 0; i < dims; i+=1) {
        this->coeffs[i] += that->coeffs[i];
      }
    }

    void sub(const ExprImpl *that) {
      assert(isEqual(this->space , that->space));
      assert(this->coeffs.size() == that->coeffs.size());
      modifying();

      auto dims = coeffs.size();
      for (auto i = 0; i < dims; i+=1) {
        this->coeffs[i] -= that->coeffs[i];
      }
    }

  }; // class ExprImpl
} // namespace isl


void Expr::release() {
  auto pimpl = keepOrNull();
  if (pimpl)
    pimpl->release();
}


Expr::StructTy *Expr::addref() const {
  auto pimpl = keepOrNull();
  if (pimpl)
    pimpl->addRef();
  return pimpl;
}


Ctx *Expr::getCtx() const {
  return keep()->getCtx();
}


void Expr::print(llvm::raw_ostream &out) const {
  keep()->print(out);
}


Expr Expr::createZero(LocalSpace &&space) {
  return Expr::enwrap(new ExprImpl(space.move()));
}


Expr Expr::createConstant(LocalSpace &&space, int v) {
  auto pimpl = new ExprImpl(space.move());
  pimpl->add(v);
  return Expr::enwrap(pimpl); 
}


Expr Expr::createConstant(LocalSpace &&space, const Int &v) {
  auto pimpl = new ExprImpl(space.move());
  pimpl->add(v);
  return Expr::enwrap(pimpl); 
}


Expr Expr::createVar(LocalSpace &&space, isl_dim_type type, unsigned pos) {
  auto pimpl = new ExprImpl(space.move());
  pimpl->add(type, pos, 1);
  return Expr::enwrap(pimpl); 
}


LocalSpace Expr::getLocalSpace() const {
  return keep()->getLocalSpace();
}


Int Expr::getCoefficient(isl_dim_type type, unsigned pos) const {
  auto pimpl = keep();
  return pimpl->getCoefficient(type, pos);
}


Int Expr::getConstant() const {
  return keep()->getCoefficient(isl_dim_cst, 0);
}


Expr &Expr::operator-=(const Expr &that) {
  auto pimpl = this->take()->cow();
  pimpl->sub(that.keep());
  this->give(pimpl);
  return *this;
}


Expr isl::add(Expr &&lhs, const Expr &rhs) {
  auto pimpl = lhs.take()->cow();
  pimpl->add(rhs.keep());
  return Expr::enwrap(pimpl);
}


Expr isl::sub(Expr &&lhs, const Expr &rhs) {
  auto pimpl = lhs.take()->cow();
  pimpl->sub(rhs.keep());
  return Expr::enwrap(pimpl);
}


Constraint isl::operator==(const Expr &lhs, const Expr &rhs) {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));

  auto space = lhs.getLocalSpace();
  auto result = space.createEqualityConstraint();

  for (auto it = space.dim_begin(), end = space.dim_end(); it!=end; ++it) {
    auto dim = *it;
    auto type = dim.getType();
    auto pos = dim.getPos();

    auto coeff = lhs.getCoefficient(type,pos) - rhs.getCoefficient(type,pos);
    result.setCoefficient_inplace(type, pos, std::move(coeff));
    //result = setCoefficient(std::move(result), type, pos, std::move(coeff)); 
  }
  auto cst = lhs.getConstant() - rhs.getConstant();
  result.setConstant_inplace(cst);

  return result;
}

Constraint isl::operator==(const Expr &lhs, int rhs) { 
  return isl::operator==(lhs, Expr::createConstant( lhs.getLocalSpace() ,rhs) ); 
}

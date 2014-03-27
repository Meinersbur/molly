#include "islpp/Point.h"

#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Int.h"
#include "islpp/BasicSet.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include "islpp/Map.h"

#include <isl/point.h>
#include <isl/set.h>

using namespace isl;
using namespace std;


void Point::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


static void printFormatted_recursive(llvm::raw_ostream &out, const Point &point, isl_dim_type type, pos_t first, Space subspace, bool printTupleNames) {
  if (subspace.isMapSpace()) {
    out << '[';
    printFormatted_recursive(out, point, type, first, subspace.getDomainSpace(), printTupleNames);
    out << " -> ";
    printFormatted_recursive(out, point, type, first + subspace.getInDimCount(), subspace.getRangeSpace(), printTupleNames);
    out << ']';
    return;
  }

  assert(subspace.isSetSpace());
  if (printTupleNames && subspace.hasSetTupleName()) {
    out << subspace.getSetTupleName();
  }

  if (subspace.isWrapping()) {
    printFormatted_recursive(out, point, type, first, subspace.unwrap(), printTupleNames);
  } else {
    out << '[';
    auto nDims = subspace.getSetDimCount();
    for (auto i = nDims - nDims; i < nDims; i += 1) {
      if (i>0)
        out << ",";
      out << point.getCoordinate(type, first + i);
    }
    out << ']';
  }
}


void Point::printFormatted(llvm::raw_ostream &out, bool printTupleNames) const {
  auto space = getSpace();

  auto nParamDims = space.getParamDimCount();
  if (nParamDims > 0) {
    out << '[';
    for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
      if (i>0)
        out << ",";
      out << this->getCoordinate(isl_dim_param, i);
    }
    out << "] -> ";
  }

  if (space.isMapSpace()) {
    //out << '[';
    printFormatted_recursive(out, *this, isl_dim_in, 0, space, printTupleNames);
    out << " -> ";
    printFormatted_recursive(out, *this, isl_dim_out, 0, space.move(), printTupleNames);
    //out << ']';
  } else if (space.isSetSpace()) {
    printFormatted_recursive(out, *this, isl_dim_set, 0, space.move(), printTupleNames);
  }
}


Point Point::createZero(Space space) {
  return Point::enwrap(isl_point_zero(space.take()));
}


Space Point::getSpace() const {
  return Space::enwrap(isl_point_get_space(keep()));
}


BasicSet Point::toBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_from_point(takeCopy()));
}
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
BasicSet Point::toBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_from_point(take()));
}
#endif

Set Point::toSet() const {
  return Set::enwrap(isl_set_from_point(takeCopy()));
}
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
Set Point::toSet() const {
  return Set::enwrap(isl_set_from_point(take()));
}
#endif

bool Point::isVoid() const{
  return isl_point_is_void(keep());
}


Int Point::getCoordinate(isl_dim_type type, int pos) const {
  Int result;
  auto retval = isl_point_get_coordinate(keep(), type, pos, result.change());
  assert(retval == 0);
  return result;
}


void Point::setCoordinate(isl_dim_type type, int pos, const Int &v) {
  give(isl_point_set_coordinate(take(), type, pos, v.keep()));
}


void Point::add(isl_dim_type type, int pos, unsigned val) {
  give(isl_point_add_ui(take(), type, pos, val));
}


void Point::sub(isl_dim_type type, int pos, unsigned val) {
  give(isl_point_sub_ui(take(), type, pos, val));
}


Set Point::apply(const Map &map) const {
  return Set::enwrap(isl_set_apply(isl_set_from_point(takeCopy()), map.takeCopy()));
}

#include "islpp_impl_common.h"
#include "islpp/Set.h"

#include "islpp/BasicSet.h"
#include "cstdiofile.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Ctx.h"
#include "islpp/Constraint.h"
#include "islpp/Id.h"
#include "islpp/Aff.h"
#include "islpp/MultiAff.h"
#include "islpp/PwAff.h"
#include "islpp/MultiPwAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/Map.h"
#include "islpp/Point.h"
#include "islpp/PwQPolynomialFold.h"
#include "islpp/Dim.h"
#include "islpp/DimRange.h"
#include "islpp/UnionSet.h"

#include <llvm/Support/raw_ostream.h>

#include <isl/set.h>
#include <isl/lp.h>
#include <isl/union_map.h>
#include <isl/map.h>
#include <isl/aff.h>
#include <isl/polynomial.h>

#include <utility>

using namespace isl;
using namespace std;


Set Set::createFromParams(Set &&set) {
  return Set::enwrap(isl_set_from_params(set.take()));
}


Set Set::createFromPwAff(PwAff &&aff) {
  return Set::enwrap(isl_set_from_pw_aff(aff.take()));
}


Set Set::createFromPwMultiAff(PwMultiAff &&aff) {
  return Set::enwrap(isl_set_from_pw_multi_aff(aff.take()));
}


Set Set::createFromPoint(Point &&point) {
  return Set::enwrap(isl_set_from_point(point.take()));
}


Set Set::createBoxFromPoints(Point &&pnt1, Point &&pnt2) {
  return Set::enwrap(isl_set_box_from_points(pnt1.take(), pnt2.take()));
}


Set Set::readFrom(Ctx *ctx, FILE *input) {
  return Set::enwrap(isl_set_read_from_file(ctx->keep(), input));
}


Set Set::readFrom(Ctx *ctx, const char *str) {
  return Set::enwrap(isl_set_read_from_str(ctx->keep(), str));
}


void Set::print(llvm::raw_ostream &out) const {
  molly::CstdioFile tmp;
  isl_set_print(this->keep(), tmp.getFileDescriptor(), 0, ISL_FORMAT_ISL);
  out << tmp;
}


void Set::dump() const {
  if (isNull()) {
    llvm::errs() << "NULL";
    return;
  }
  isl_set_dump(keep());
}


static const char *povray_prologue =
"// Generated from an Integer Set Library set\n"
"#include \"colors.inc\"\n"
"#include \"screen.inc\"\n"
"\n"
"#macro Point3D(xx,yy,zz)\n"
"#ifndef (Min_X) #declare Min_X = xx; #elseif (xx < Min_X) #declare Min_X = xx; #end\n"
"#ifndef (Max_X) #declare Max_X = xx; #elseif (xx > Max_X) #declare Max_X = xx; #end\n"
"#ifndef (Min_Y) #declare Min_Y = yy; #elseif (xx < Min_Y) #declare Min_Y = yy; #end\n"
"#ifndef (Max_Y) #declare Max_Y = yy; #elseif (xx > Max_Y) #declare Max_Y = yy; #end\n"
"#ifndef (Min_Z) #declare Min_Z = zz; #elseif (zz < Min_Y) #declare Min_Z = zz; #end\n"
"#ifndef (Max_Z) #declare Max_Z = zz; #elseif (zz > Max_Y) #declare Max_Z = zz; #end\n"
"object {\n"
"  sphere { <0,0,0>, 0.25\n"
"    pigment { color Black }\n"
"    finish { specular 0.3 }\n"
"  }\n"
"  translate<xx,yy,zz>\n"
"}\n"
"#end\n"
"\n"
"#macro Point2D(xx,yy)\n"
"  Point3D(xx,yy,0)\n"
"#end\n"
"\n"
"#macro Point1D(xx)\n"
"  Point2D(xx,0)\n"
"#end\n"
"\n"
"#macro Space1D()\n"
"  plane { <0,1,0,>, 0.5  }\n"
"  plane { <0,-1,0>, 0.5 }\n"
"  plane { <0,0,1>, 0.5 }\n"
"  plane { <0,0,-1>, 0.5  }\n"
"#end \n"
"\n"
"#macro CondIneq1D(c, xx)\n"
"  #local vec = <xx,0,0>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro CondEq1D(c, xx)\n"
"#local vec = <xx,0,0>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"  plane { -vec, (c-0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro Space2D()\n"
"  plane { <0,0,1>, 0.5 }\n"
"  plane { <0,0,-1>, 0.5  }\n"
"#end\n"
"\n"
"#macro CondIneq2D(c, xx, yy)\n"
"  #local vec = <xx,yy,0>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro CondEq2D(c, xx, yy)\n"
"#local vec = <xx,yy,0>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"  plane { -vec, (c-0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro Space3D()\n"
"#end\n"
"\n"
"#macro CondIneq3D(c, xx, yy, zz)\n"
"  #local vec = <xx,yy,0>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro CondEq3D(c, xx, yy, zz)\n"
"#local vec = <xx,yy,zz>;\n"
"  plane { -vec, (c+0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"  plane { -vec, (c-0.5*vlength(vec))/(vlength(vec)*vlength(vec)) }\n"
"#end\n"
"\n"
"#macro Polytope()\n"
"  texture { pigment { color Red transmit 0.5 } }\n"
"  finish { diffuse 0.9 phong 0.5 }\n"
"#end\n"
"\n"
;

static const char *povray_epilogue =
"\n"
"\n"
"#declare Midpoint = <(Min_X+Max_X)/2,(Min_Y+Max_Y)/2(Min_Y+Max_Y)/2>;\n"
"\n"
"Set_Camera_Location(<1,1,1>*vlength(Midpoint)*1.1)\n"
"Set_Camera_Look_At(Midpoint)\n"
"Set_Camera_Aspect(image_width,image_height)\n"
"\n"
"background {\n"
"  color White\n"
"}\n"
"\n"
"light_source {\n"
"  Camera_Location*0.7\n"
"  color 1.5\n"
"}\n"
"\n"
"#macro Arrow(dst,c)\n"
"union {\n"
"  union {\n"
"    cylinder{<0,0,0>, dst, 0.1 }\n"
"    cone { dst,0.4,dst+vnormalize(dst)*1.5,0 }\n"
"    pigment { c }\n"
"  }\n"
"  #declare K = 0;\n"
"  #while (K <= vlength(dst))\n"
"        cylinder{ vnormalize(dst)*K, vnormalize(dst)*K + vnormalize(vnormalize(dst) - <1,1,1>)*1.3 , 0.05 }\n"
"        text { ttf \"crystal.ttf\", str(K,0,0), 0.01, 0\n"
"          translate <0,-0.5,0>\n"
"          scale 0.8\n"
"          transform { Camera_Transform }\n"
"          translate -Camera_Location\n"
"          translate vnormalize(dst)*K + vnormalize(vnormalize(dst) - <1,1,1>)*1.5\n"
"        }\n"
"        #declare K = K + 1;\n"
"   #end \n"
"   pigment { Black }\n"
"}\n"
"#end\n"
"\n"
"Arrow(<7,0,0>, rgb<1,0,0>)\n"
"Arrow(<0,5,0>, rgb<0,1,0>)\n"
"Arrow(<0,0,3>, rgb<0,0,1>)\n"
;

void Set::printPovray(llvm::raw_ostream &out) const {
  out << povray_prologue;

  // Find what we take a X,Y,Z coordinates
  auto space = getSpace();
  auto params = space.getParamDimCount();
  assert(params == 0);
  auto dims = space.getSetDimCount();
  assert(0 < dims && dims <= 3);
  //Int min[3] = { Int(0), Int(0), Int(0) };
  //Int max[3] = {0,0,0};
  bool firstCoord = true;

  foreachPoint([&, dims](Point point) -> bool {
    out << "Point" << dims << "D(";

    bool first = true;
    for (auto d = dims - dims; d < dims; d += 1) {
      if (!first) {
        out << ", ";
      }
      auto coord = point.getCoordinate(isl_dim_set, d);
      if (d < 3) {
        if (firstCoord) {
          //min[d] = coord;
          //max[d] = coord;
        } else {
          //if (coord < min[d]) {
          //min[d] = coord;
          //}
          //if (coord > max[d]) {
          // max[d] = coord;
          //}
        }
      }
      out << coord;
      first = false;
    }

    out << ")\n";
    firstCoord = false;
    return false;
  });

  assert(!firstCoord); // firstCoord means empty set 
  //Int len[3];
  //for (auto d = dims-dims; d < std::min(dims,3u); d+=1) {
  //  len[d] = (max[d] - min[d])/2;
  //}
  out << "\n\n";

  foreachBasicSet([&](BasicSet bset) -> bool {
    out << "intersection {\n";
    auto lspace = bset.getLocalSpace();
    auto dims = lspace.dim(isl_dim_set);
    out << "  Space" << dims << "D()\n";

    bset.foreachConstraint([&](Constraint constraint) -> bool {
      //constraint.printProperties(out, 2);

      auto space = constraint.getLocalSpace();
      auto dims = space.dim(isl_dim_set);

      auto offset = constraint.getConstant();
      bool anyNonzero = false;
      for (auto i = dims - dims; i < dims; i += 1) {
        auto coeff = constraint.getCoefficient(isl_dim_set, i);
        if (!coeff.isZero()) {
          anyNonzero = true;
          break;
        }
      }
      if (!anyNonzero)
        return false;

      anyNonzero = false;
      auto divdims = space.dim(isl_dim_div);
      for (auto i = divdims - divdims; i < divdims; i += 1) {
        auto coeff = constraint.getCoefficient(isl_dim_div, i);
        if (!coeff.isZero()) {
          return false; // Div dimensionsions do not appear in diagram
        }
      }

      out << (constraint.isEquality() ? "  CondEq" : "  CondIneq") << dims << "D(" << offset;
      for (auto i = dims - dims; i < dims; i += 1) {
        out << ", ";
        auto coeff = constraint.getCoefficient(isl_dim_set, i);
        out << coeff;
      }
      out << ")\n";

      return false;
    });

    out << "  Polytope()\n}\n";
    return false;
  });

  out << povray_epilogue;
}

#if 0
std::string Set::toString() const {
  if (!keep())
    return string();  
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return stream.str();
}
#endif

#if 0
void Set::dump() const { 
  print(llvm::errs());
}


Space Set::getSpace() const {
  return Space::wrap(isl_set_get_space(keep()));
}
#endif

bool Set::foreachBasicSet(BasicSetCallback fn, void *user) const {
  return isl_set_foreach_basic_set(keep(), fn, user);
}

static int basicsetcallback(__isl_take isl_basic_set *bset, void *user) {
  auto &fn = *static_cast<std::function<bool(BasicSet)>*>(user);
  auto result = fn(BasicSet::wrap(bset));
  return result ? -1 : 0;
}
bool Set::foreachBasicSet(std::function<bool(BasicSet/*rvalue ref?*/)> fn) const {
  auto retval = isl_set_foreach_basic_set(keep(), basicsetcallback, &fn);
  return retval != 0;
}


bool Set::foreachPoint(PointCallback fn, void *user) const {
  return isl_set_foreach_point(keep(), fn, user);
}




static int pointcallback(__isl_take isl_point *pnt, void *user) {
  auto fn = *static_cast<std::function<bool(Point)>*>(user);
  auto result = fn(Point::enwrap(pnt));
  return result ? -1 : 0;
}
bool Set::foreachPoint(std::function<bool(Point)> fn) const {
  auto retval = isl_set_foreach_point(keep(), pointcallback, &fn);
  return retval != 0;
}


std::vector<Point> Set::getPoints() const {
  std::vector<Point> result;
  foreachPoint([&](Point point) -> bool {
    result.push_back(std::move(point));
    return false;
  });
  return result;
}


int Set::getBasicSetCount() const {
  return isl_set_n_basic_set(keep());
}

#if 0
unsigned Set::dim(isl_dim_type type) const {
  return isl_set_dim(keep(), type);
}
#endif

//unsigned Set::getSetDimCount() const { return dim(isl_dim_set); }

int Set::getInvolvedDims(isl_dim_type type, unsigned first, unsigned n) const {
  return isl_set_involves_dims(keep(), type, first, n);
}


bool Set::dimHasAnyLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_any_lower_bound(keep(), type, pos);
}

bool Set::dimHasAnyUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_any_upper_bound(keep(), type, pos);
}

bool Set::dimHasLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_lower_bound(keep(), type, pos);
}

bool Set::dimHasUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_upper_bound(keep(), type, pos);
}
#if 0
void Set::setTupleId(Id &&id) {
  give(isl_set_set_tuple_id(take(), id.take()));
}


void Set::resetTupleId() {
  give(isl_set_reset_tuple_id(take()));
}

bool Set::hasTupleId() const {
  return isl_set_has_tuple_id(keep());
}

Id Set::getTupleId() const {
  return Id::enwrap(isl_set_get_tuple_id(keep()));
}

bool Set::hasTupleName() const {
  return isl_set_has_tuple_name(keep());
}

const char *Set::getTupleName() const {
  return isl_set_get_tuple_name(keep());
}

void Set::setDimId(isl_dim_type type, unsigned pos, Id &&id) {
  give(isl_set_set_dim_id(take(), type, pos, id.take()));
}


bool Set::hasDimId(isl_dim_type type, unsigned pos) const {
  return isl_set_has_dim_id(keep(), type, pos);
}


Id Set::getDimId(isl_dim_type type, unsigned pos) const {
  return Id::enwrap(isl_set_get_dim_id(keep(), type, pos));
}

int Set::findDimById(isl_dim_type type, const Id &id) const {
  return isl_set_find_dim_by_id(keep(), type, id.keep());
}

int Set::findDimByName(isl_dim_type type, const char *name) const {
  return isl_set_find_dim_by_name(keep(), type, name);
}

bool Set:: dimHasName(isl_dim_type type, unsigned pos) const {
  return isl_set_has_dim_name(keep(), type, pos);
}


const char *Set::getDimName(enum isl_dim_type type, unsigned pos) const {
  return isl_set_get_dim_name(keep(), type, pos);
}
#endif
bool Set::plainIsEmpty() const {
  return isl_set_plain_is_empty(keep());
}
bool Set::isEmpty() const {
  return isl_set_is_empty(keep());
}

bool Set::plainIsUniverse() const {
  return isl_set_plain_is_universe(keep());
}


bool Set::plainIsFixed(isl_dim_type type, unsigned pos, Int &val) const {
  return isl_set_plain_is_fixed(keep(), type, pos, val.change());
}


void Set::eliminate(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_set_eliminate(take(), type, first, n));
}

void Set::fix(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_fix(take(), type, pos, value.keep()));
}

void Set::fix(isl_dim_type type, unsigned pos, int value) {
  give(isl_set_fix_si(take(), type, pos, value));
}

void Set::lowerBound(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_lower_bound(take(), type, pos, value.keep()));
}

void Set::lowerBound(isl_dim_type type, unsigned pos, signed long value) {
  give(isl_set_lower_bound_si(take(), type, pos, value));
}

void Set::upperBound(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_upper_bound(take(), type, pos, value.keep()));
}

void Set::upperBound(isl_dim_type type, unsigned pos, signed long value) {
  give(isl_set_upper_bound_si(take(), type, pos, value));
}


void Set::equate(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) {
  give(isl_set_equate(take(), type1, pos1, type2, pos2));
}


BasicSet isl::convexHull(Set&&set) {
  return BasicSet::wrap(isl_set_convex_hull(set.take()));
}
BasicSet isl::unshiftedSimpleHull(Set&&set){
  return  BasicSet::wrap(isl_set_unshifted_simple_hull(set.take()));
}

#if 0
BasicSet isl::simpleHull(Set&&set){
  return BasicSet::wrap(isl_set_simple_hull(set.take()));
}

BasicSet isl::affineHull(Set &&set) {
  return   BasicSet::wrap(isl_set_affine_hull(set.take()));
}


BasicSet isl::polyhedralHull(Set &&set) {
  return BasicSet::wrap(isl_set_polyhedral_hull(set.take()));
}
#endif

void Set::dropContraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_set_drop_constraints_involving_dims(take(), type, first, n));
}


BasicSet isl::sample(Set &&set) {
  return  BasicSet::wrap(isl_set_sample(set.take()));
}

PwAff isl::dimMin(Set &&set, int pos) {
  return PwAff::enwrap(isl_set_dim_min(set.take(), pos));
}
PwAff isl::dimMax(Set &&set, int pos) {
  return PwAff::enwrap(isl_set_dim_max(set.take(), pos));
}


BasicSet isl::coefficients(Set &&set) {
  return BasicSet::wrap(isl_set_coefficients(set.take()));
}

BasicSet isl::solutions(Set &&set){
  return BasicSet::wrap(isl_set_solutions(set.take()));
}


//  Map isl::unwrap(Set &&set) {
//     return Map::wrap(isl_set_unwrap(set.take()));
//  }

void Set::flatten() {
  give(isl_set_flatten(take()));
}


Map isl::flattenMap(Set &&set) {
  return Map::enwrap(isl_set_flatten_map(set.take()));
}


void Set::lift() {
  give(isl_set_lift(take()));
}


PwAff Set::dimMin(int pos) const {
  return PwAff::enwrap(isl_set_dim_min(takeCopy(), pos));
}


PwAff Set::dimMin(const Dim &dim) const {
  assert(dim.getType() == isl_dim_set);
  return PwAff::enwrap(isl_set_dim_min(takeCopy(), dim.getPos()));
}


PwAff Set::dimMax(int pos) const {
  return PwAff::enwrap(isl_set_dim_max(takeCopy(), pos));
}


PwAff Set::dimMax(const Dim &dim) const {
  assert(dim.getType() == isl_dim_set);
  return PwAff::enwrap(isl_set_dim_max(takeCopy(), dim.getPos()));
}


void Set::apply_inplace(Map &&map) ISLPP_INPLACE_FUNCTION{ give(isl_set_apply(take(), map.take())); }
void Set::apply_inplace(const Map &map) ISLPP_INPLACE_FUNCTION{ give(isl_set_apply(take(), map.takeCopy())); }
Set Set::apply(Map &&map) const { return Set::enwrap(isl_set_apply(takeCopy(), map.take())); }
Set Set::apply(const Map &map) const { return Set::enwrap(isl_set_apply(takeCopy(), map.takeCopy())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
Set Set::apply(Map &&map) && { return Set::enwrap(isl_set_apply(take(), map.take())); }
Set Set::apply(const Map &map) && { return Set::enwrap(isl_set_apply(take(), map.takeCopy())); }
#endif


PwMultiAff Set::lexminPwMultiAff() const { return PwMultiAff::enwrap(isl_set_lexmin_pw_multi_aff(takeCopy())); }
PwMultiAff Set::lexmaxPwMultiAff() const { return PwMultiAff::enwrap(isl_set_lexmax_pw_multi_aff(takeCopy())); }


Map Set::unwrap() const {
  return Map::enwrap(isl_set_unwrap(takeCopy()));
}


Map Set::chain(const Map &map) const{
  return map.intersectDomain(*this);
}

#if 0
Map Set::chainNested(const Map &map) const {
  auto domainTuple = map.getInTupleId();
  auto space = getSpace();

  unsigned firstTupleDim;
  unsigned tupleDimCount;
  auto success = space.findTuple(isl_dim_set, domainTuple, firstTupleDim, tupleDimCount);
  assert(success);
  assert(map.getInDimCount() == tupleDimCount);
  assert(space.extractNestedTupleSpace(isl_dim_set, domainTuple).matchesSetSpace(map.getDomainSpace()));

  auto equator = Space::createMapFromDomainAndRange(space, map.getDomainSpace()).equalBasicMap(isl_dim_in, firstTupleDim, tupleDimCount, isl_dim_out, 0); 
  return chain(equator).applyRange(map);
}
#endif

Map Set::chainSubspace(const Map &map) const {
  return copy().chainSubspace_consume(map);
}


Map Set::chainSubspace_consume(const Map &map) {
  auto myspace = this->getSpace();
  assert(myspace.isSetSpace());
  auto subspace = map.getDomainSpace();

  auto range = myspace.findSubspace(isl_dim_set, subspace);
  assert(range.isValid());

  auto equator = Space::createMapFromDomainAndRange(myspace.move(), subspace.move()).equalBasicMap(isl_dim_in, range.getBeginPos(), range.getCount(), isl_dim_out, 0);
  return chain(equator.move()).applyRange(map);
}


Map Set::chainNested(const Map &map, unsigned tuplePos) const {
  auto domainTuple = map.getInTupleId();
  auto space = getSpace();
  unsigned tupleDimCount = map.getInDimCount();
  assert(space.extractNestedTupleSpace(isl_dim_set, space.findTuplePos(isl_dim_set, domainTuple)) == map.getDomainSpace());

  auto equator = Space::createMapFromDomainAndRange(space, map.getDomainSpace()).equalBasicMap(isl_dim_in, tuplePos, tupleDimCount, isl_dim_out, 0);
  return chain(equator).applyDomain(map);
}


void Set::permuteDims_inplace(llvm::ArrayRef<unsigned> order) ISLPP_INPLACE_FUNCTION{
  llvm::SmallVector<unsigned, 4> poss(order.begin(), order.end());
  auto nDims = getSetDimCount();

  auto nextDst = 0;
  auto size = order.size();
  for (auto i = size - size; i < size; i += 1) {
    auto origPos = order[i];
    auto next = nDims;
    for (auto j = size - size; j < size; j += 1) {
      if (j == i)
        continue;
      if (order[j] < origPos)
        continue;
      next = std::min(next, order[j]);
    }
    auto len = next - origPos;
    auto srcPos = poss[i];

    moveDims_inplace(isl_dim_set, nextDst, isl_dim_set, srcPos, len);

    for (auto j = i + 1; j < size; j += 1) {
      if (poss[j] < srcPos)
        continue;
      assert(poss[j] >= srcPos + len);
      poss[j] += len; // Dims has been moved in front
    }
    nextDst += len;
  }
}



#if 0
Map Set::unwrapTuple_internal(unsigned tuplePos) ISLPP_INTERNAL_QUALIFIER {
  auto space = getSpace();
  auto nestedSpace = space.getNested();
  // assert(nestedSpace.isValid() && "Cannot unwrap something that is not wrapped (would result in a Map with zero-dimensional domain which isl does not really allow)");
  if (nestedSpace.isNull()) {
    assert(tuplePos==0);
    auto result = Map::createFromRange(move());
    result.setOutTupleId_inplace(space.getTupleId());
    return result;
  }

  unsigned pos,n;
  Id id;
  bool found = findNestedTuple(space, tuplePos, pos, n, id);
  assert(found);

  auto result = Map::createFromDomain(move());
  result.moveDims_inplace(isl_dim_out, 0, isl_dim_in, pos, n);
  result.cast_inplace();
  return result;
}


Map Set:: unwrapTuple_internal(const Id &tupleId) ISLPP_INTERNAL_QUALIFIER {
}
#endif


Set isl::alignParams(Set &&set, Space &&model) {
  return Set::enwrap(isl_set_align_params(set.take(), model.take()));
}


Set isl::addDims(Set &&set, isl_dim_type type, unsigned n) {
  return Set::enwrap(isl_set_add_dims(set.take(), type, n));
}

Set isl::insertDims(Set &&set, isl_dim_type type, unsigned pos, unsigned n){
  return Set::enwrap(isl_set_insert_dims(set.take(), type, pos, n));
}
Set isl::moveDims(Set &&set, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n){
  return Set::enwrap(isl_set_move_dims(set.take(), dst_type, dst_pos, src_type, src_pos, n));
}


Set isl::intersectParams(Set &&set, Set &&params){
  return Set::enwrap(isl_set_intersect_params(set.take(), params.take()));
}
Set isl::intersect(Set set1, Set set2){
  return Set::enwrap(isl_set_intersect(set1.take(), set2.take()));
}


Set isl::apply(Set &&set, Map &&map){
  return Set::enwrap(isl_set_apply(set.take(), map.take()));
}
Set isl::apply(const Set &set, Map &&map){
  return Set::enwrap(isl_set_apply(set.takeCopy(), map.take()));
}
Set isl::apply(Set &&set, const Map &map){
  return Set::enwrap(isl_set_apply(set.take(), map.takeCopy()));
}
Set isl::apply(const Set &set, const Map &map){
  return Set::enwrap(isl_set_apply(set.takeCopy(), map.takeCopy()));
}


Set isl::preimage(Set &&set, MultiAff &&ma){
  return Set::enwrap(isl_set_preimage_multi_aff(set.take(), ma.take()));
}
Set isl::preimage(Set &&set, PwMultiAff &&ma){
  return Set::enwrap(isl_set_preimage_pw_multi_aff(set.take(), ma.take()));
}

#if 0
Set isl::product(Set &&set1, Set &&set2){
  return Set::enwrap(isl_set_product(set1.take(), set2.take()));
}
Set isl::flatProduct(Set &&set1,Set &&set2){
  return Set::enwrap(isl_set_flat_product(set1.take(), set2.take()));
}
#endif

Set isl::gist(Set &&set, Set &&context){
  return Set::enwrap(isl_set_gist(set.take(), context.take()));
}
Set isl::gistParams(Set &&set, Set &&context){
  return Set::enwrap(isl_set_gist_params(set.take(), context.take()));
}
Set isl::partialLexmin(Set &&set, Set &&dom, Set &empty){
  return Set::enwrap(isl_set_partial_lexmin(set.take(), dom.take(), empty.change()));
}
Set isl::partialLexmax(Set &&set, Set &&dom, Set &empty){
  return Set::enwrap(isl_set_partial_lexmax(set.take(), dom.take(), empty.change()));
}
Set isl::lexmin(Set &&set){
  return Set::enwrap(isl_set_lexmin(set.take()));
}
Set isl::lexmax(Set &&set){
  return Set::enwrap(isl_set_lexmax(set.take()));
}
PwAff isl::indicatorFunction(Set &&set){
  return PwAff::enwrap(isl_set_indicator_function(set.take()));
}

Point isl::samplePoint(Set &&set) {
  return Point::enwrap(isl_set_sample_point(set.take()));
}

PwQPolynomialFold isl::apply(Set &&set, PwQPolynomialFold &&pwf, bool &tight) {
  int intTight;
  auto result = isl_set_apply_pw_qpolynomial_fold(set.take(), pwf.take(), &intTight);
  tight = intTight;
  return PwQPolynomialFold::wrap(result);
}


Set Set::complement() const {
  return Set::enwrap(isl_set_complement(takeCopy()));
}


Set isl::complement(Set &&set) {
  return Set::enwrap(isl_set_complement(set.take()));
}

Set Set::projectOut(isl_dim_type type, unsigned first, unsigned n) const {
  return Set::enwrap(isl_set_project_out(takeCopy(), type, first, n));
}
Set isl::projectOut(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::enwrap(isl_set_project_out(set.take(), type, first, n));
}


Set Set::params() const {
  return Set::enwrap(isl_set_params(takeCopy()));
}


Map Set::unwrapSubspace(const Space &subspace) const {
  auto myspace = getSpace();
  auto range = myspace.findSubspace(isl_dim_set, subspace);
  assert(range.isValid());

  auto mapToDomainSpace = Space::createMapFromDomainAndRange(myspace, myspace.removeSubspace(subspace));
  auto mapToRangeSpace = Space::createMapFromDomainAndRange(myspace, subspace);

  auto mapToDomain = mapToDomainSpace.equalBasicMap(isl_dim_in, 0, range.getBeginPos(), isl_dim_out, 0);
  mapToDomain.intersect(mapToDomainSpace.equalBasicMap(isl_dim_in, range.getBeginPos() + range.getCount(), mapToDomainSpace.getOutDimCount() - range.getBeginPos(), isl_dim_out, range.getBeginPos()));

  auto mapToRange = mapToRangeSpace.equalBasicMap(isl_dim_in, range.getBeginPos(), range.getCount(), isl_dim_out, 0);

  auto result = chain(mapToRange);
  result.applyDomain_inplace(mapToDomain);
  return result;
}


Map Set::subspaceMap(const Space &subspace) const {
  auto myspace = getSpace();
  auto range = myspace.findSubspace(isl_dim_set, subspace);
  assert(range.isValid());

  auto equator = Space::createMapFromDomainAndRange(myspace, subspace).equalBasicMap(isl_dim_in, range.getBeginPos(), range.getCount(), isl_dim_out, 0);
  return chain(equator.move());
}


Map Set::subrangeMap(unsigned first, unsigned count) const {
  auto subspace = getCtx()->createSetSpace(0, count);
  auto equator = Space::createMapFromDomainAndRange(getSpace(), subspace).equalBasicMap(isl_dim_in, first, count, isl_dim_out, 0);
  return chain(equator.move());
}


Map Set::reorganizeSubspaceList(llvm::ArrayRef<Space> domainSubspaces, llvm::ArrayRef<Space> rangeSubspaces) {
  auto space = getSpace();

  auto nTotalDomainDims = 0;
  Space domainSpace;
  auto nDomainSubspaces = domainSubspaces.size();
  for (auto i = nDomainSubspaces - nDomainSubspaces; i < nDomainSubspaces; i += 1) {
    auto subspace = domainSubspaces[i];
    domainSpace = combineSpaces(domainSpace, subspace);
    nTotalDomainDims += subspace.dim(isl_dim_in) + subspace.dim(isl_dim_out);
  }
  if (domainSpace.isNull())
    domainSpace = getParamsSpace().createSetSpace(0);

  auto nTotalRangeDims = 0;
  Space rangeSpace;
  auto nRangeSubspaces = rangeSubspaces.size();
  for (auto i = nRangeSubspaces - nRangeSubspaces; i < nRangeSubspaces; i += 1) {
    auto subspace = rangeSubspaces[i];
    rangeSpace = combineSpaces(rangeSpace, subspace);
    nTotalRangeDims += subspace.dim(isl_dim_in) + subspace.dim(isl_dim_out);
  }
  if (rangeSpace.isNull())
    rangeSpace = getParamsSpace().createSetSpace(0);

  auto resultSpace = Space::createMapFromDomainAndRange(domainSpace, rangeSpace);
  auto result = resultSpace.createUniverseMap();


  auto domainMapSpace = Space::createMapFromDomainAndRange(space, resultSpace.getDomainSpace());
  auto domainMap = domainMapSpace.universeBasicMap();
  unsigned pos = 0;
  for (auto i = nDomainSubspaces - nDomainSubspaces; i < nDomainSubspaces; i += 1) {
    auto subspace = domainSubspaces[i];
    auto dimrange = space.findSubspace(isl_dim_set, subspace);
    assert(dimrange.isValid());
    domainMap.intersect(domainMapSpace.equalBasicMap(isl_dim_in, dimrange.getBeginPos(), dimrange.getCount(), isl_dim_out, pos));
    pos += dimrange.getCount();
  }

  auto rangeMapSpace = Space::createMapFromDomainAndRange(space, resultSpace.getRangeSpace());
  auto rangeMap = domainMapSpace.universeBasicMap();
  pos = 0;
  for (auto i = nRangeSubspaces - nRangeSubspaces; i < nRangeSubspaces; i += 1) {
    auto subspace = rangeSubspaces[i];
    auto dimrange = space.findSubspace(isl_dim_set, subspace);
    assert(dimrange.isValid());
    rangeMap.intersect(domainMapSpace.equalBasicMap(isl_dim_in, dimrange.getBeginPos(), dimrange.getCount(), isl_dim_out, pos));
    pos += dimrange.getCount();
  }

  return rangeMap.intersectDomain(*this).applyDomain(domainMap);
}


static void reorganizeSubspaces_recursive(const Space &parentSpace, /*out*/BasicMap &map, const Space &subspace, /*out*/unsigned &offset, bool mustExist) {
  assert(map.isValid());

  if (subspace.isSetSpace() && subspace.hasTupleId(isl_dim_set)) {
    auto dimrange = parentSpace.findSubspace(isl_dim_set, subspace);
    if (dimrange.isValid()) {
      map.intersect_inplace(map.getSpace().equalBasicMap(isl_dim_in, dimrange.getBeginPos(), dimrange.getCount(), isl_dim_out, offset));
      offset += dimrange.getCount();
      return;
    }
  }

  if (subspace.isWrapping()) {
    reorganizeSubspaces_recursive(parentSpace, map, subspace.unwrap(), offset, mustExist);
  } else if (subspace.isMapSpace()) {
    reorganizeSubspaces_recursive(parentSpace, map, subspace.getDomainSpace(), offset, mustExist);
    if (map.isNull()) {
      assert(mustExist);
      return;
    }
    reorganizeSubspaces_recursive(parentSpace, map, subspace.getRangeSpace(), offset, mustExist);
  } else {
    // Recursion leaf
    // If we get there this means that some subspace has no source in parentSpace. Hence, the BasicMap map will not contain any restrictions for this subspace

    if (mustExist) {
      // Error out
      llvm_unreachable("Must exist flag set and subspace not found");
      map.reset();
      return;
    }

    //auto dimrange = parentSpace.findSubspace(isl_dim_set, subspace);
    //assert(dimrange.isValid());
    //map.intersect(map.getSpace().equalBasicMap(isl_dim_in, dimrange.getBeginPos(), dimrange.getCount(), isl_dim_out, offset));
    //offset += subspace.getSetDimCount();
    //llvm_unreachable("subspace to put next not found in set's space");
    offset += subspace.getSetDimCount();
  }
}


Map Set::reorganizeSubspaces(const Space &domainSpace_, const Space &rangeSpace_, bool mustExist) const {
  auto space = getSpace();
  auto domainSpace = domainSpace_.normalizeWrapped();
  auto rangeSpace = rangeSpace_.normalizeWrapped();

  auto domainMapSpace = Space::createMapFromDomainAndRange(space, domainSpace);
  auto domainMap = domainMapSpace.universeBasicMap();
  unsigned domainPos = 0;
  reorganizeSubspaces_recursive(space, domainMap, domainSpace, domainPos, mustExist);
  if (domainMap.isNull())
    return Map();
  assert(domainPos == domainSpace.getSetDimCount());

  auto rangeMapSpace = Space::createMapFromDomainAndRange(space, rangeSpace);
  auto rangeMap = rangeMapSpace.universeBasicMap();
  unsigned rangePos = 0;
  reorganizeSubspaces_recursive(space, rangeMap, rangeSpace, rangePos, mustExist);
  if (rangeMap.isNull())
    return Map();
  assert(rangePos == rangeSpace.getSetDimCount());

  return rangeMap.intersectDomain(*this).applyDomain(domainMap);
}


Set Set::reorganizeSubspaces(const Space &space, bool mustExist) ISLPP_EXSITU_FUNCTION{
  auto myspace = getSpace();
  auto targetspace = space.normalizeWrapped();

  auto mapSpace = Space::createMapFromDomainAndRange(myspace, targetspace);
  auto map = mapSpace.universeBasicMap();
  unsigned pos = 0;
  reorganizeSubspaces_recursive(myspace, map, targetspace, pos, mustExist);
  assert(pos == targetspace.getSetDimCount());

  return apply(map);
}


Set Set::cast(Space space) ISLPP_EXSITU_FUNCTION{
  assert(space.getSetDimCount() == this->getDimCount());

  // No inbuilt cast is ISL
  auto translateSpace = getSpace().mapsTo(space);
  auto translate = translateSpace.equalBasicMap(getDimCount());
  return apply(translate);
}


ISLPP_EXSITU_ATTRS Map isl::Set::reorderSubspaces(const Space &domainSpace, const Space &rangeSpace) ISLPP_EXSITU_FUNCTION{
  return reorganizeSubspaces(std::move(domainSpace), std::move(rangeSpace), true);
}


void isl::Set::dumpExplicit(int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted) const {
  printExplicit(llvm::dbgs(), maxElts, newlines, formatted, sorted);
  llvm::dbgs() << "\n";
}


void isl::Set::dumpExplicit() const {
  dumpExplicit(8, false, false, true);
}


void isl::Set::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted) const {
  auto nParamDims = getParamDimCount();
  auto nDims = getDimCount();

  int count = 0;
  auto lambda = [&count, maxElts, &os, newlines, formatted](Point p) -> bool {
    if (count >= maxElts)
      return true;

    if (count == 0) {
      os << (newlines ? "{\n  " : "{ ");
    } else {
      os << (newlines ? ",\n  " : ", ");
    }
    if (formatted)
      p.printFormatted(os);
    else
      os << p;

    count += 1;
    return false;
  };

  bool omittedsome;
  if (sorted) {
    std::vector<Point> points;
    points.reserve(maxElts);
    omittedsome = foreachPoint([&points, maxElts](Point p) -> bool {
      if (points.size() >= maxElts)
        return true;
      points.push_back(p);
      return false;
    });
    std::sort(points.begin(), points.end(), [nParamDims, nDims](const Point &p1, const Point &p2) -> bool {
      for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
        auto c1 = p1.getCoordinate(isl_dim_param, i);
        auto c2 = p2.getCoordinate(isl_dim_param, i);
        auto cmp = isl_int_cmp(c1.keep(), c2.keep());
        if (!cmp)
          continue;
        return cmp < 0;
      }
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto c1 = p1.getCoordinate(isl_dim_set, i);
        auto c2 = p2.getCoordinate(isl_dim_set, i);
        auto cmp = isl_int_cmp(c1.keep(), c2.keep());
        if (!cmp)
          continue;
        return cmp < 0;
      }
      return 0;
    });
    for (const auto &p : points) {
      if (lambda(p))
        break;
    }
  } else {
    omittedsome = this->foreachPoint(lambda);
  }

  if (count == 0) {
    if (omittedsome) {
      os << "{ ... }";
    } else {
      os << "{ }";
    }
  } else {
    if (omittedsome) {
      os << (newlines ? ",\n  ...\n}" : ", ... }");
    } else {
      os << (newlines ? "\n}" : " }");
    }
  }
}

std::string isl::Set::toStringExplicit(int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted) {
  std::string str;
  llvm::raw_string_ostream os(str);
  printExplicit(os, maxElts, newlines, sorted);
  os.flush();
  return str; // NRVO
}


std::string isl::Set::toString() const {
  return ObjBaseTy::toString();
}


void isl::Set::intersect_inplace(UnionSet that) ISLPP_INPLACE_FUNCTION{
  intersect_inplace(that.extractSet(getSpace()));
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::Set::chainSubspace(PwMultiAff pma) ISLPP_EXSITU_FUNCTION{
  auto domainSpace = getSpace();
  auto middleSpace = pma.getDomainSpace();
  auto id = MultiAff::createIdentity(domainSpace.mapsToItself());

  isl::Space subspace;
  auto range = domainSpace.findSubspace(isl_dim_set, middleSpace); assert(range.isValid());
  id.removeDims_inplace(isl_dim_out, range.getEnd(), id.getOutDimCount() - range.getEnd());
  id.removeDims_inplace(isl_dim_out, 0, range.getFirst());
  auto restricted = id.castRange(middleSpace).intersectDomain(copy());
  return restricted.applyRange(std::move(pma));
}


ISLPP_INPLACE_ATTRS void isl::Set::coalesce_inplace() ISLPP_INPLACE_FUNCTION{
  give(isl_set_coalesce(take()));
}

ISLPP_PROJECTION_ATTRS isl::HasBounds isl::Set::getDimBounds(isl_dim_type type, pos_t pos, Int &lowerBound, Int &upperBound)ISLPP_PROJECTION_FUNCTION{
  HasBounds result = HasBounds::Empty;
  int cnt = 0;
  for (auto bset : getBasicSets()) {
    if (cnt == 0) {
      result = bset.getDimBounds(type, pos, lowerBound, upperBound);
    } else {
      Int bsetLowerBound;
      Int bsetUpperBound;
      auto bsetHasBounds = bset.getDimBounds(type, pos, bsetLowerBound, bsetUpperBound);
      if (!flags(bsetHasBounds & HasBounds::LowerBound)) {
        result &= ~HasBounds::LowerBound;
      }
      if (flags(bsetHasBounds & HasBounds::LowerBound) && flags(result & HasBounds::LowerBound)) {
        lowerBound = isl::min(lowerBound, bsetLowerBound);
      }
      if (!flags(bsetHasBounds & HasBounds::UpperBound)) {
        result &= ~HasBounds::UpperBound;
      }
      if (flags(bsetHasBounds & HasBounds::UpperBound) && flags(result & HasBounds::UpperBound)) {
        upperBound = isl::min(lowerBound, bsetLowerBound);
      }
    }
    cnt += 1;
  }
  if (cnt >= 2) {
    if (flags(result & HasBounds::LowerBound) && flags(result & HasBounds::UpperBound)) {
      if (lowerBound == upperBound)
        return HasBounds::Fixed;
      if (lowerBound > upperBound)
        return HasBounds::Impossible;
    }
  }
  return result;
}



ISLPP_CONSUME_ATTRS PwMultiAff Set::chainSubspace_consume(PwMultiAff pma) ISLPP_CONSUME_FUNCTION{
  return chainSubspace(std::move(pma));
}


static int enumBasicSetsCallback(__isl_take isl_basic_set *set, void *user) {
  auto list = static_cast<std::vector<BasicSet> *>(user);
  list->push_back(BasicSet::enwrap(set));
  return 0;
}
ISLPP_EXSITU_ATTRS std::vector<BasicSet> Set::getBasicSets() ISLPP_EXSITU_FUNCTION{
  std::vector<BasicSet> result;
  auto retval = isl_set_foreach_basic_set(keep(), enumBasicSetsCallback, &result);
  assert(retval == 0);
  return result;
}


Set isl::params(Set &&set){
  return Set::enwrap(isl_set_params(set.take()));
}


Set isl::addContraint(Set &&set, Constraint &&constraint) {
  return Set::enwrap(isl_set_add_constraint(set.take(), constraint.take()));
}


Set isl::computeDivs(Set &&set) {
  return Set::enwrap(isl_set_compute_divs(set.take()));
}


Set isl::alignDivs(Set &&set) {
  return Set::enwrap(isl_set_align_divs(set.take()));
}


Set isl::removeDivs(Set &&set) {
  return Set::enwrap(isl_set_remove_divs(set.take()));
}


Set isl::removeDivsInvolvingDims(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::enwrap(isl_set_remove_divs_involving_dims(set.take(), type, first, n));
}

Set isl::removeUnknownDivs(Set &&set) {
  return Set::enwrap(isl_set_remove_unknown_divs(set.take()));
}

Set isl::makeDisjoint(Set &&set) {
  return Set::enwrap(isl_set_make_disjoint(set.take()));
}


bool isl::plainsIsEqual(const Set &left, const Set &right){
  return isl_set_plain_is_equal(left.keep(), right.keep());
}


bool isl::isEqual(const Set &left, const Set &right) {
  return isl_set_is_equal(left.keep(), right.keep());
}


bool isl::plainIsDisjoint(const Set &lhs, const Set &rhs) {
  return isl_set_plain_is_disjoint(lhs.keep(), rhs.keep());
}


bool isl::isDisjoint(const Set &lhs, const Set &rhs) {
  return isl_set_is_disjoint(lhs.keep(), rhs.keep());
}


bool isl::isSubset(const Set &lhs, const Set &rhs) {
  return isl_set_is_subset(lhs.keep(), rhs.keep());
}


bool isl::isStrictSubset(const Set &lhs, const Set &rhs) {
  return isl_set_is_strict_subset(lhs.keep(), rhs.keep());
}


int isl::plainCmp(const Set &lhs, const Set &rhs) {
  return isl_set_plain_cmp(lhs.keep(), rhs.keep());
}


isl::Map isl::alltoall(Set domainSet, Set rangeSet) {
  return product(domainSet, rangeSet).unwrap();
}


isl::Map isl::alltoall(Space domainUniverse, Set rangeSet) {
  return alltoall(domainUniverse.universeBasicSet(), std::move(rangeSet));
}


ISLPP_PROJECTION_ATTRS uint64_t Set::getComplexity() ISLPP_PROJECTION_FUNCTION{
  uint32_t pieces = 0;
  uint32_t basic = 0;

  for (const auto &bset : getBasicSets()) {
    pieces += bset.getComplexity();
    basic += 1; // logical or
  }

  uint64_t result = pieces;
  result <<= 32;
  result |= basic;

  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t Set::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  uint64_t complexity = 0;

  for (const auto &bset : getBasicSets()) {
    complexity += bset.getComplexity();
    complexity += 1; // logical or
  }

  if (complexity == 0)
    return 0; // No basic sets: set is empty

  complexity -= 1; // logical or only between basic sets, so we added one too much

  return complexity;
}


AllBounds Set::getAllBounds() const {
  AllBounds bounds;
  int cnt = 0;

  for (const auto &bset : getBasicSets()) {
    if (cnt == 0) {
      bounds.init(bset.getLocalSpace());
      bounds.searchAllBounds(bset);
      bounds.scrapDivDims();
    } else {
      auto bsetBounds = bset.getAllBounds();
      bsetBounds.scrapDivDims();
      bounds.disjunctiveMerge(bsetBounds);
    }
    cnt += 1;
  }

  return bounds; // NRVO
}

#if 0
static void dimToTypePos(size_t d, count_t nParamDims, count_t nDims, count_t  isl_dim_type &type, pos_t &pos) {
  if (d < nParamDims) {
    type = isl_dim_param;
    pos = d;
    return;
  }
  d -= nParamDims;

  if (d < nDims) {
    type = isl_dim_set;

  }
}
#endif

static Aff makeAffFromRow(const LocalSpace &ls, const Mat &eqs, int row, int d) {
  auto aff = ls.createConstantAff(eqs[row][0]);
  auto nDims = eqs.cols() - 1;
  assert(ls.getParamDimCount() + ls.getSetDimCount() == nDims);

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    if (i == d)
      continue;

    isl_dim_type type;
    pos_t pos;
    ls.matrixTypePos(i + 1, type, pos);
    Int coeff = eqs[row][i + 1];
    aff.setCoefficient_inplace(type, pos, coeff);
  }

  Int coeff = eqs[row][d + 1];
  if (coeff >= 0) {
    aff.neg_inplace();
    aff.setDenominator_inplace(coeff);
  } else {
    aff.setDenominator_inplace(-coeff);
  }
  return aff;
}


/// Check if makeAffFromRow(eqs, j, jd) == makeAffFromRow(eqs, i, jd)
static bool areAreSameConditionsForDimenstions(const Mat &eqs, size_t i, size_t j, pos_t id, pos_t jd) {
  assert(i != j);
  assert(id != jd);
  auto cols = eqs.cols();

  if (!eqs[i][jd].isZero())
    return false;
  if (!eqs[j][id].isZero())
    return false;

  auto idCoeff = eqs[i][id];
  auto jdCoeff = eqs[j][jd];
  if (!isAbsEqual(idCoeff, jdCoeff))
    return false; // One could still be a multiple of the other, but we assume this has been normalized away
  auto factor = idCoeff.sgn()*jdCoeff.sgn();

  for (auto d = cols - cols; d < cols; d += 1) {
    if (d == id)
      continue;
    if (d == jd)
      continue;

    auto iCoeff = eqs[i][d];
    auto jCoeff = eqs[j][d];
    auto factor = iCoeff.sgn()*jCoeff.sgn();

    iCoeff *= factor;
    if (iCoeff != jCoeff)
      return false;
  }
  return true;
}


static void findEqualDimensions(SmallVectorImpl<Aff> &list, const LocalSpace &commonls, const LocalSpace &ls, const Mat &eqs, pos_t i, pos_t id) {
  auto neqs = eqs.rows();
  auto nDims = eqs.cols() - 1;

  const Int &iCoeff = eqs[i][id];
  assert(!iCoeff.isZero());

  int candidate = -1;
  for (auto j = neqs - neqs; j < neqs; j += 1) {
    if (j == i)
      continue;

    for (auto jd = 1; id < nDims + 1; id += 1) {
      if (jd == id)
        continue;

      if (areAreSameConditionsForDimenstions(eqs, i, j, id, jd)) {
        isl_dim_type type;
        pos_t pos;
        ls.matrixTypePos(jd, type, pos);
        auto aff = ls.createVarAff(type, pos);
        aff.alignDivs_inplace(commonls);
        list.push_back(aff);
      }
    }
  }
}


static void collectEquals(SmallVectorImpl<Aff> &list, const LocalSpace &commonls, const LocalSpace &ls, const Mat &eqs, pos_t d) {
  auto neqs = eqs.rows();
  auto cols = eqs.cols();

  for (auto ik = neqs - neqs; ik < neqs; ik += 1) {
    Int coeff = eqs[ik][d];
    if (!coeff.isAbsOne())
      continue;

    auto aff = makeAffFromRow(ls, eqs, ik, d);
    aff.alignDivs_inplace(commonls);
    list.push_back(aff);

    findEqualDimensions(list, commonls, ls, eqs, ik, d);
  }
}


static Aff findCommonAffForDim(const LocalSpace &ils, const Mat &ieqs, const LocalSpace &jls, const Mat &jeqs, size_t d){
  auto ineqs = ieqs.rows();
  auto jneqs = jeqs.rows();
  auto commonls = isl::intersect(ils, jls);
  //auto nDims = commonls.getParamDimCount()+commonls.getInDimCount();
  auto nParamDims = commonls.getParamDimCount();
  auto iDims = ieqs.cols();
  auto jDims = jeqs.cols();

  // Stuff dimension d equals to in their BasicSet
  SmallVector<Aff, 4> iEquals;
  collectEquals(iEquals, commonls, ils, ieqs, d);

  SmallVector<Aff, 4> jEquals;
  collectEquals(jEquals, commonls, jls, jeqs, d);


  for (const auto &iaff : iEquals) {
    for (const auto &jaff : jEquals) {
      if (iaff == jaff) {
        return iaff;
      }
    }
  }


#if 0
  for (auto ik = ineqs - ineqs; ik < ineqs; ik += 1) {
    Int icoeff = ieqs[ik][d + 1];
    if (!icoeff.isAbsOne())
      continue;
    auto iaff = makeAffFromRow(ils, ieqs, ik, d);
    iaff.alignDivs_inplace(commonls);

    for (auto jk = jneqs - jneqs; jk < jneqs; jk += 1) {
      Int jcoeff = ieqs[ik][d + 1];
      if (!jcoeff.isAbsOne())
        continue;
      auto jaff = makeAffFromRow(jls, jeqs, jk, d);
      jaff.alignDivs_inplace(commonls);

      if (iaff == jaff) 
        return iaff;

#pragma region X
      for (auto il = ineqs - ineqs; il < ineqs; il += 1) {
        if (il == ik)
          continue;

        int iCandidate = -1;
        bool condidneg ;
        for (auto id = iDims-iDims;id<iDims;id+=1) {
          if (ieqs[])
        }

      }
#pragma endregion

    }
  }

  //auto iBounds = ibset.getAllBounds();
  //auto jBounds = jbset.getAllBounds();

  for (auto ik = ineqs - ineqs; ik < ineqs; ik += 1) {
    Int icoeff = ieqs[ik][d + 1];
    if (!icoeff.isAbsOne())
      continue;
    auto iaff = makeAffFromRow(ils, ieqs, ik, d);

    for (auto il = ik+1; il < ineqs; il += 1) {
    }
  }
#endif

  return Aff();
}


static BasicSet tryCoalesceBSets(const BasicSet &origibset, const BasicSet &origjbset) {
  auto origspace = origibset.getSpace();
  assert(origspace == origjbset.getSpace());
  auto islctx = origibset.getCtx();

  auto ibset = origibset.moveDims(isl_dim_in, 0, isl_dim_param, 0, origibset.getParamDimCount());
  auto jbset = origjbset.moveDims(isl_dim_in, 0, isl_dim_param, 0, origjbset.getParamDimCount());
  auto restore = ibset.getSpace().mapsToItself().createIdentityMultiAff(); // { reduced[] -> space[] }

  auto ils = ibset.getLocalSpace();
  auto ieqs = ibset.equalitiesMatrix();

  auto jls = jbset.getLocalSpace();
  auto jeqs = jbset.equalitiesMatrix();

  auto nDims = ibset.getDimCount();
  assert(nDims == jbset.getDimCount());
  bool somereduction = false;
  auto red = 0;
  auto nred = nDims;
  while (red < nred) {
    auto replacement = findCommonAffForDim(ils, ieqs, jls, jeqs, red);
    if (replacement.isValid()) {
      somereduction = true;
      assert(replacement.getCoefficient(isl_dim_in, red).isZero());
      replacement.removeDims_inplace(isl_dim_in, red, 1);
      ibset.projectOut_inplace(isl_dim_set, red, 1);
      jbset.projectOut_inplace(isl_dim_set, red, 1);
      auto restspace = islctx->createMapSpace(nred - 1, nred);
      auto restsinglepace = restspace.getDomainSpace();
      auto rest = restspace.createZeroMultiAff();
      // auto restsingle = restspace.getDomainSpace().createZeroAff();
      for (auto k = red - red; k < red; k += 1) {
        auto aff = restsinglepace.createAffOnVar(red);
        rest.setAff_inplace(k, std::move(aff));
      }
      rest.setAff_inplace(red, replacement);
      for (auto k = red + 1; k < nred; k += 1) {
        auto aff = restsinglepace.createAffOnVar(red - 1);
        rest.setAff_inplace(k, std::move(aff));
      }
      restore.pullback_inplace(rest);

      ieqs = ibset.equalitiesMatrix();
      ils = ibset.getLocalSpace();
      jeqs = jbset.equalitiesMatrix();
      jls = jbset.getLocalSpace();

      nred -= 1;
    } else {
      red += 1;
    }
  }

  BasicSet result;
  Set redset;
  if (!somereduction) {
    redset = isl::unite(origibset, origjbset);
  } else {
    redset = isl::unite(ibset, jbset);
  }

  redset.coalesce_inplace();
  if (redset.getBasicSetCount() == 1) {
    result = redset.anyBasicSet();
    if (somereduction) {
      result.preimage_inplace(restore);
      result.moveDims_inplace(isl_dim_param, 0, isl_dim_set, 0, origspace.getParamDimCount());
      result.cast_inplace(origspace);
    }
  }
  return result; // NRVO
}

#if 0
ISLPP_EXSITU_ATTRS Set Set::coalesceEx() ISLPP_EXSITU_FUNCTION{
  if (getBasicSetCount() <= 1)
  return *this;

  auto space = getSpace();
  auto bsets = detectEqualities().getBasicSets();
  auto n = bsets.size();
  //auto cmp =  coalesce();

  auto i = n - 2;
  while (i < n) {
    auto j = i + 1;

    while (j < n) {
      auto &ibset = bsets[i];
      const auto &jbset = bsets[j];

      auto coalesced = tryCoalesceBSets(ibset, jbset);
      if (coalesced.isValid()) {
        ibset = coalesced;
        bsets.erase(bsets.begin() + j);
        n -= 1;
        j = i + 1;
      } else {
        j += 1;
      }
    }

    i -= 1;
  }

  Set result = space.emptySet();
  for (auto i = n - n; i < n; i += 1) {
    const BasicSet &bset = bsets[i];
    result.unite_inplace(bset);
  }

  return result;
}
#endif


ISLPP_EXSITU_ATTRS BasicSet Set::anyBasicSet() ISLPP_EXSITU_FUNCTION{
  BasicSet result;
  foreachBasicSet([&result](BasicSet bset) ->bool {
    result = bset;
    return true;
  });
  return result;
}

static bool hasDependent(const Mat &eqs, size_t offset) {
  auto nEqs = eqs.rows();
  auto cols = eqs.cols();
  for (auto i = nEqs - nEqs; i < nEqs; i += 1) {
    Int coeff = eqs[i][offset];
    if (coeff.isZero())
      continue;

    if (!coeff.isAbsOne())
      return false; // Some div conditions we do not handle

    for (auto d = 1; d < cols; d += 1) {
      if (d == offset)
        continue;
      Int c = eqs[i][d];
      if (c.isZero())
        continue;
      return false;
    }

    //involvingConditions += 1;
  }
}

static bool isIndependentDimension(const BasicSet &bset, isl_dim_type type, pos_t pos) {
  auto offset = bset.matrixOffset(type, pos);

  auto eqs = bset.equalitiesMatrix();
  if (hasDependent(eqs, offset))
    return false;

  auto ineqs = bset.inequalitiesMatrix();
  if (hasDependent(ineqs, offset))
    return false;

  return true;
}


static void extractAdjacentDims(BasicSet &ibset, const AllBounds &iBounds, BasicSet &jbset, const AllBounds &jBounds, isl_dim_type type, BasicSet &intervals, int &numAdjacent) {
  auto nDims = ibset.dim(type);
  auto space = ibset.getSpace();

  for (auto d = nDims - nDims; d < nDims; d += 1) {
    auto iInterval = iBounds.getInterval(type, d);
    auto jInterval = jBounds.getInterval(type, d);
    if (!unitable(iInterval, jInterval))
      continue;
    // We now know that there is no gap between the dimensions (.i.a are overlapping or adjacent)

    if (!isIndependentDimension(ibset, type, d))
      continue;
    if (!isIndependentDimension(jbset, type, d))
      continue;

    // We now know that those dimensions are independent of the other dimensions
    // Also, are not involved in any divs

    auto interval = unite(iInterval, jInterval);
    if (interval.hasLower) {
      auto c = space.createGeConstraint(space.createVarAff(type, d), space.createConstantAff(interval.lower));
      intervals.addConstraint_inplace(c);
    }
    if (interval.hasUpper) {
      auto c = space.createLeConstraint(space.createVarAff(type, d), space.createConstantAff(interval.upper));
      intervals.addConstraint_inplace(c);
    }

    // Remove the conditions from the sets
    // Do this by removing the dimension and adding it again
    ibset.removeDims_inplace(type, d, 1);
    if (type != isl_dim_param) { // Params jest ge re-align again when necessary
      ibset.insertDims_inplace(type, d, 1);
    }

    jbset.removeDims_inplace(type, d, 1);
    if (type != isl_dim_param) {
      jbset.insertDims_inplace(type, d, 1);
    }

    numAdjacent += 1;
  }
}


static BasicSet tryCoalesce(BasicSet ibset, const AllBounds &iBounds, BasicSet jbset, const AllBounds &jBounds) {
  int numAdjacent = 0;

  auto space = ibset.getSpace();
  auto intervals = space.universeBasicSet();

  extractAdjacentDims(ibset, iBounds, jbset, jBounds, isl_dim_param, intervals, numAdjacent);
  extractAdjacentDims(ibset, iBounds, jbset, jBounds, isl_dim_set, intervals, numAdjacent);

  // isl_basic_set can handle zero or one adjacent dimensions itself
  if (numAdjacent <= 1)
    return BasicSet();

  auto both = isl::unite(ibset, jbset);
  both.coalesce_inplace();

  if (both.getBasicSetCount() == 1) {
    auto result = both.anyBasicSet();

    // re-add the interval constraints
    result.intersect_inplace(intervals);
    result.cast_inplace(space);
    return result;
  }

  return BasicSet();
}

#if 0
ISLPP_EXSITU_ATTRS Set Set::coalesceEx() ISLPP_EXSITU_FUNCTION{ 
  // Handle a special case isl_set_coalesce cannot handle:
  // Two dimensions that are adjacent to each other
  // We can only handle the case when those dimensions do not influence the other dims
  auto space = getSpace();
  auto bsets = getBasicSets();
  auto n = bsets.size();

  SmallVector<AllBounds,4> bounds;
  bounds.reserve(n);
  for (auto i = n - n; i < n; i += 1) {
    const auto &bset = bsets[i];
    bounds.push_back(bset.getAllBounds());
  }

  auto i = n - 2;
  while (i < n) {
    auto j = i + 1;

    while (j < n) {
      auto &ibset = bsets[i];
      const auto &jbset = bsets[j];

      auto coalesced = tryCoalesce(ibset, bounds[i] ,jbset, bounds[j]);
      if (coalesced.isValid()) {
        ibset = coalesced;
        bounds[i].disjunctiveMerge(bounds[j]);
        bsets.erase(bsets.begin() + j);
        bounds.erase(bounds.begin()+j);
        n -= 1;
        j = i + 1;
      } else {
        j += 1;
      }
    }

    i -= 1;
  }

  Set result = space.emptySet();
  for (i = n - n; i < n; i += 1) {
    auto bset = bsets[i];
    result.unite_inplace(bset);
  }

  // coalesce all the rest
  result.coalesce_inplace();
  return result; // NRVO
}
#endif
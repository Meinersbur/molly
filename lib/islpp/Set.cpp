#include "islpp/Set.h"

#include "islpp/BasicSet.h"
#include "islpp/cstdiofile.h"
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


isl_set *Set::takeCopy() const {
  assert(set); 
  return isl_set_copy(this->set);
}


void Set::give(isl_set *set) { 
  if (set)
    isl_set_free(set);
  this->set = set; 
}


Set::~Set() { 
  if (set) 
    isl_set_free(set);
}


Set::Set(BasicSet &&that) : set(isl_set_from_basic_set(that.take())) {
}


const Set &Set::operator=(BasicSet &&that) {
  give(isl_set_from_basic_set(that.take()));
  return *this;
}


Set::Set(const BasicSet &that) : set(isl_set_from_basic_set(that.takeCopy())) {
}


const Set &Set::operator=(const BasicSet &that){
  give(isl_set_from_basic_set(that.takeCopy()));
  return *this;
}


Set Set::createFromParams(Set &&set) {
  return Set::wrap(isl_set_from_params(set.take()));
}

Set Set::createFromPoint(Point &&point) {
  return Set::wrap(isl_set_from_point(point.take()));
}

Set Set:: createBocFromPoints(Point &&pnt1, Point &&pnt2) {
  return Set::wrap(isl_set_box_from_points(pnt1.take(), pnt2.take()));
}

Set Set::readFrom(const Ctx &ctx, FILE *input) {
  return Set::wrap(isl_set_read_from_file(ctx.keep(), input));
}

Set Set::readFrom(const Ctx &ctx, const char *str) {
  return Set::wrap(isl_set_read_from_str(ctx.keep(), str));
}


void Set::print(llvm::raw_ostream &out) const { 
  molly::CstdioFile tmp;
  isl_set_print(this->keep(), tmp.getFileDescriptor(), 0, ISL_FORMAT_ISL);
  out << tmp;
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
  auto params = space.getParamDims();
  assert(params == 0);
  auto dims = space.getSetDims();
  assert(0 < dims && dims <= 3);
  //Int min[3] = { Int(0), Int(0), Int(0) };
  //Int max[3] = {0,0,0};
  bool firstCoord = true;

  foreachPoint([&,dims] (Point point) -> bool {
    out << "Point" << dims << "D(";

    bool first = true;
    for (auto d = dims-dims; d < dims; d+=1) {
      if (!first) {
        out<<", ";
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

  foreachBasicSet([&] (BasicSet bset) -> bool {
    out << "intersection {\n";
    auto lspace = bset.getLocalSpace();
    auto dims = lspace.dim(isl_dim_set);
    out << "  Space" << dims << "D()\n";

    bset.foreachConstraint([&] (Constraint constraint) -> bool {
      //constraint.printProperties(out, 2);

      auto space = constraint.getLocalSpace();
      auto dims = space.dim(isl_dim_set);

      auto offset = constraint.getConstant();
      bool anyNonzero = false;
      for (auto i = dims-dims; i < dims;i+=1) {
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
      for (auto i = divdims-divdims; i < divdims;i+=1) {
        auto coeff = constraint.getCoefficient(isl_dim_div, i);
        if (!coeff.isZero()) {
          return false; // Div dimensionsions do not appear in diagram
        }
      }

      out << (constraint.isEquality() ? "  CondEq" : "  CondIneq") << dims << "D(" << offset;
      for (auto i = dims-dims; i < dims;i+=1) {
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


std::string Set::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}


void Set::dump() const { 
  print(llvm::errs());
}


Space Set::getSpace() const {
  return Space::wrap(isl_set_get_space(keep()));
}


bool Set::foreachBasicSet(BasicSetCallback fn, void *user) const {
  return isl_set_foreach_basic_set(keep(), fn, user);
}

static int basicsetcallback(__isl_take isl_basic_set *bset, void *user) {
  auto fn = *static_cast<std::function<bool(BasicSet)>*>(user);
  auto result = fn(BasicSet::wrap(bset));
  return result ? -1 : 0;
}
bool Set::foreachBasicSet(std::function<bool(BasicSet/*rvalue ref?*/)> fn) const {
  auto retval = isl_set_foreach_basic_set(keep(), basicsetcallback, &fn);
  return retval!=0;
}


bool Set::foreachPoint(PointCallback fn, void *user) const {
  return isl_set_foreach_point(keep(), fn, user);
}




static int pointcallback(__isl_take isl_point *pnt, void *user) {
  auto fn = *static_cast<std::function<bool(Point)>*>(user);
  auto result = fn(Point::wrap(pnt));
  return result ? -1 : 0;
}
bool Set::foreachPoint(std::function<bool(Point)> fn) const {
  auto retval = isl_set_foreach_point(keep(), pointcallback, &fn);
  return retval!=0;
}


std::vector<Point> Set::getPoints() const {
  std::vector<Point> result;
  foreachPoint([&] (Point point) -> bool { 
    result.push_back(std::move(point));  
    return false;
  });
  return result;
}


int Set::getBasicSetCount() const {
  return isl_set_n_basic_set(keep());
}

unsigned Set::getDim(isl_dim_type type) const {
  return isl_set_dim(keep(), type);
}


int Set::getInvolvedDims(isl_dim_type type,unsigned first, unsigned n) const {
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
  return Id::wrap(isl_set_get_tuple_id(keep()));
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
  return Id::wrap(isl_set_get_dim_id(keep(), type, pos));
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


void Set::eliminate( isl_dim_type type, unsigned first, unsigned n) {
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

void Set::coalesce() {
  give(isl_set_coalesce(take()));
}

void Set::detectEqualities() {
  give(isl_set_detect_equalities(take()));
}





void Set::removeRedundancies() {
  give(isl_set_remove_redundancies(take()));
}


BasicSet isl::convexHull(Set&&set) {
  return BasicSet::wrap(isl_set_convex_hull(set.take()));
}
BasicSet isl::unshiftedSimpleHull(Set&&set){
  return  BasicSet::wrap(isl_set_unshifted_simple_hull(set.take()));
}
BasicSet isl::simpleHull(Set&&set){
  return BasicSet::wrap(isl_set_simple_hull(set.take()));
}

BasicSet isl::affineHull(Set &&set) {
  return   BasicSet::wrap(isl_set_affine_hull(set.take()));
}


BasicSet isl::polyhedralHull(Set &&set) {
  return BasicSet::wrap(isl_set_polyhedral_hull(set.take()));
}


void Set::dropContraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_set_drop_constraints_involving_dims(take(), type, first, n));
}


BasicSet isl::sample(Set &&set) {
  return  BasicSet::wrap(isl_set_sample(set.take()));
}

PwAff isl::dimMin(Set &&set, int pos) {
  return PwAff::wrap(isl_set_dim_min(set.take(), pos));
}
PwAff isl::dimMax(Set &&set, int pos) {
  return PwAff::wrap(isl_set_dim_max(set.take(), pos));
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
  return Map::wrap(isl_set_flatten_map( set.take()));
}


void Set::lift() {
  give(isl_set_lift(take()));
}

Set isl::alignParams(Set &&set, Space &&model) {
  return Set::wrap(isl_set_align_params(set.take(), model.take()));
}


Set isl::addDims(Set &&set,isl_dim_type type, unsigned n) {
  return Set::wrap(isl_set_add_dims(set.take(),type,n));
}

Set isl::insertDims(Set &&set, isl_dim_type type, unsigned pos, unsigned n){
  return Set::wrap(isl_set_insert_dims(set.take(), type, pos, n));
}
Set isl::moveDims(Set &&set, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos,  unsigned n){
  return Set::wrap(isl_set_move_dims(set.take(), dst_type, dst_pos, src_type, src_pos, n));
}





Set isl::intersectParams(Set &&set, Set &&params){
  return Set::wrap(isl_set_intersect_params(set.take(), params.take()));
}
Set isl::intersect(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_intersect(set1.take(), set2.take()));
}
Set isl::union_(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_union(set1.take(), set2.take()));
}
Set isl::subtract(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_subtract (set1.take(), set2.take()));
}
Set isl::apply(Set &&set, Map &&map){
  return Set::wrap(isl_set_apply (set.take(), map.take()));
}
Set isl::preimage(Set &&set, MultiAff &&ma){
  return Set::wrap(isl_set_preimage_multi_aff (set.take(), ma.take()));
}
Set isl::preimage(Set &&set, PwMultiAff &&ma){
  return Set::wrap(isl_set_preimage_pw_multi_aff(set.take(), ma.take()));
}

Set isl::product(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_product(set1.take(), set2.take()));
}
Set isl::flatProduct(Set &&set1,Set &&set2){
  return Set::wrap(isl_set_flat_product(set1.take(), set2.take()));
}
Set isl::gist(Set &&set, Set &&context){
  return Set::wrap(isl_set_gist(set.take(), context.take()));
}
Set isl::gistParams(Set &&set, Set &&context){
  return Set::wrap(isl_set_gist_params(set.take(), context.take()));
}
Set isl::partialLexmin(Set &&set, Set &&dom, Set &empty){
  return Set::wrap(isl_set_partial_lexmin(set.take(), dom.take(), empty.change()));
}
Set isl::partialLexmax(Set &&set, Set &&dom, Set &empty){
  return Set::wrap(isl_set_partial_lexmax(set.take(), dom.take(), empty.change()));
}
Set isl::lexmin(Set &&set){
  return Set::wrap(isl_set_lexmin(set.take()));
}
Set isl::lexmax(Set &&set){
  return Set::wrap(isl_set_lexmax(set.take()));
}
PwAff isl::indicatorFunction(Set &&set){
  return PwAff::wrap(isl_set_indicator_function(set.take()));
}

Point isl::samplePoint(Set &&set) {
  return Point::wrap(isl_set_sample_point(set.take()));
}

PwQPolynomialFold isl::apply(Set &&set, PwQPolynomialFold &&pwf, bool &tight) {
  int intTight;
  auto result = isl_set_apply_pw_qpolynomial_fold(set.take(), pwf.take(), &intTight);
  tight = intTight;
  return PwQPolynomialFold::wrap(result);
}


Set Set::complement() const {
  return Set::wrap(isl_set_complement(takeCopy()));
}




Set isl::complement(Set &&set) {
  return Set::wrap(isl_set_complement(set.take()));
}

Set Set::projectOut(isl_dim_type type, unsigned first, unsigned n) const {
  return Set::wrap(isl_set_project_out(takeCopy(), type, first, n));
}
Set isl::projectOut(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::wrap(isl_set_project_out(set.take(), type, first, n));
}


Set Set::params() const {
  return Set::wrap(isl_set_params(takeCopy()));
}


Set isl::params(Set &&set){
  return Set::wrap(isl_set_params(set.take()));
}


Set isl::addContraint(Set &&set, Constraint &&constraint) {
  return Set::wrap(isl_set_add_constraint(set.take(), constraint.take()));
}

Set isl::computeDivs(Set &&set) {
  return Set::wrap(isl_set_compute_divs(set.take()));
}

Set isl::alignDivs(Set &&set) {
  return Set::wrap(isl_set_align_divs(set.take()));
}

Set isl::removeDivs(Set &&set) {
  return Set::wrap(isl_set_remove_divs(set.take()));
}

Set isl::removeDivsInvolvingDims(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::wrap(isl_set_remove_divs_involving_dims(set.take(), type, first, n));
}

Set isl::removeUnknownDivs(Set &&set) {
  return Set::wrap(isl_set_remove_unknown_divs(set.take()));
}

Set isl::makeDisjoint(Set &&set) {
  return Set::wrap(isl_set_make_disjoint(set.take()));
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

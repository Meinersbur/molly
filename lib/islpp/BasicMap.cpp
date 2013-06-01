#include "islpp/BasicMap.h"

#include "islpp/Map.h"
#include <isl/map.h>


using namespace isl;


isl_basic_map *BasicMap::takeCopy() const {
  assert(map);
  return isl_basic_map_copy(map);
}


void BasicMap::give(isl_basic_map *map) {
  assert(map);
  assert(this->map != map|| !map);
  if (this->map)
    isl_basic_map_free(this->map);
  this->map = map;
}


BasicMap::~BasicMap(){
  if (map)
    isl_basic_map_free(map);
#ifndef NDEBUG
  this->map = nullptr;
#endif
}


BasicMap BasicMap::createEmptyLikeMap(Map &&model) { 
  return wrap(isl_basic_map_empty_like_map(model.take())); 
}


BasicMap BasicMap::createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type c5) {
  return wrap(isl_basic_map_from_constraint_matrices(space.take(), eq.take(), ineq.take(), c1,c2,c3,c4,c5));
}


void BasicMap::dump() const { 
  isl_basic_map_dump(keep());
}


Map isl::partialLexmax(BasicMap &&bmap, BasicSet &&dom, Set &empty) { 
  isl_set *rawempty = 0;
  auto result = Map::wrap(isl_basic_map_partial_lexmax(bmap.take(), dom.take(), &rawempty)); 
  empty = Set::wrap(rawempty);
  return result;
}


Map isl::partialLexmin(BasicMap &&bmap, BasicSet &&dom, Set &empty) { 
  isl_set *rawempty = 0;
  auto result = Map::wrap(isl_basic_map_partial_lexmin(bmap.take(), dom.take(), &rawempty)); 
  empty = Set::wrap(rawempty);
  return result;
}


Map isl::lexmin(BasicMap &&bmap) { 
  return Map::wrap(isl_basic_map_lexmin(bmap.take()));
} 


Map isl::lexmax(BasicMap &&bmap) { 
  return Map::wrap(isl_basic_map_lexmax(bmap.take())); 
} 

   PwMultiAff isl::partialLexminPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
    isl_set *rawempty = 0;
    auto result = enwrap(isl_basic_map_partial_lexmin_pw_multi_aff(bmap.take(), dom.take(), &rawempty)); 
    empty = Set::wrap(rawempty);
    return result;
  }
   PwMultiAff isl::partialLexmaxPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
    isl_set *rawempty = 0;
    auto result = enwrap(isl_basic_map_partial_lexmax_pw_multi_aff(bmap.take(), dom.take(), &rawempty)); 
    empty = Set::wrap(rawempty);
    return result;
  }


      PwMultiAff isl::lexminPwMultiAff(BasicMap &&bmap) { return enwrap(isl_basic_map_lexmin_pw_multi_aff(bmap.take())); }
 PwMultiAff isl::lexmaxPwMultiAff(BasicMap &&bmap) { return enwrap(isl_basic_map_lexmax_pw_multi_aff(bmap.take())); }


Map isl::union_(BasicMap &&bmap1, BasicMap &&bmap2) { 
  return Map::wrap(isl_basic_map_union(bmap1.take(), bmap2.take())); 
}


Map isl::computeDivs(BasicMap &&bmap) {
  return enwrap(isl_basic_map_compute_divs(bmap.take()));
}

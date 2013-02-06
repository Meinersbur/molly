#include "islpp/BasicMap.h"

#include <isl/map.h>

using namespace isl;


isl_basic_map *BasicMap::takeCopy() const {
  return isl_basic_map_copy(map);
}


void BasicMap::give(isl_basic_map *vertices) {
  assert(this->map != map|| !map);
  if (this->map)
    isl_basic_map_free(this->map);
  this->map = map;
}


BasicMap::~BasicMap(void){
  if (map)
    isl_basic_map_free(map);
}

#include "islpp/Vertices.h"

#include <isl/vertices.h>

// Missing in <isl/vertices.h>
extern "C" __isl_give isl_vertices *isl_vertices_copy(__isl_keep isl_vertices *vertices);

using namespace isl;


isl_vertices *Vertices::takeCopy() const {
  return isl_vertices_copy(vertices);
}


void Vertices::give(isl_vertices *vertices) {
  assert(this->vertices != vertices|| !vertices);
  if (this->vertices)
    isl_vertices_free(this->vertices);
  this->vertices = vertices;
}


Vertices::~Vertices(void){
  if (vertices)
    isl_vertices_free(vertices);
}
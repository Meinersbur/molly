#ifndef ISLPP_VERTICES_H
#define ISLPP_VERTICES_H

#include "islpp_common.h"
#include <cassert>

struct isl_vertices;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class Vertices {
#pragma region Low-level
  private:
    isl_vertices *vertices;

  public: // Public because otherwise we had to add a lot of friends
    isl_vertices *take() { assert(vertices); isl_vertices *result = vertices; vertices = nullptr; return result; }
    isl_vertices *takeCopy() const;
    isl_vertices *keep() const { return vertices; }
  protected:
    void give(isl_vertices *vertices);

  public:
    static Vertices wrap(isl_vertices *vertices) { Vertices result; result.give(vertices); return result; }
#pragma endregion

  public:
    Vertices(void) : vertices(nullptr) {}
    Vertices(const Vertices &that) : vertices(that.takeCopy()) {}
    Vertices(Vertices &&that) : vertices(that.take()) { }
    ~Vertices(void);

    const Vertices &operator=(const Vertices &that) { give(that.takeCopy()); return *this; }
    const Vertices &operator=(Vertices &&that) { give(that.take()); return *this; }

  }; // class Vertices
} // namespace isl
#endif /* ISLPP_VERTICES_H */

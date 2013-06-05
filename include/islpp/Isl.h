#ifndef ISLPP_ISL_H
#define ISLPP_ISL_H

#include "islpp_common.h"

// Forward-declare all types of islpp
namespace isl {
  class Ctx;

  class Int;
  class Val;
  class Id;

  class Space;
  class LocalSpace;

  class Constraint;
  class Aff;

  class BasicSet;
  class Set;
  class BasicMap;
  class Map;

  class Printer;
} // namespace isl

#include "List.h"
#include "Multi.h"
#include "Union.h"
#include "Pw.h"

#endif /* ISLPP_ISL_H */
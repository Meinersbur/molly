#include "islpp/UnionMap.h"

#include "islpp/Printer.h"
#include <isl/union_map.h>
#include <llvm/ADT/DenseMap.h>

using namespace llvm;
using namespace isl;



void UnionMap::print(llvm::raw_ostream &out) const {
  if (isNull())
    return;

  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void UnionMap::dump() const {
  isl_union_map_dump(keep());
}


static int foreachMapCallback(__isl_take isl_map *map, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Map)>*>(user);
  auto retval = func(Map::enwrap(map));
  return retval ? -1 : 0;
}
bool UnionMap::foreachMap(const std::function<bool(isl::Map)> &func) const {
  auto retval = isl_union_map_foreach_map(keep(), foreachMapCallback, (void*)&func);
  return retval!=0;
}


static int enumMapCallback(__isl_take isl_map *map, void *user) {
  std::vector<Map> * list = static_cast< std::vector<Map> *>(user);
  list->push_back(Map::enwrap(map));
  return 0;
}
std::vector<Map> UnionMap::getMaps() const {
  std::vector<Map> result;
  result.reserve(isl_union_map_n_map(keep()));
  auto retval = isl_union_map_foreach_map(keep(), enumMapCallback, &result);
  assert(retval==0);
  return result;
}


void isl::simpleFlow(const UnionMap &sinks, const UnionMap &sources, const UnionMap &schedule, UnionMap *depPtr, UnionMap *nosrcPtr) {
  auto sourcesMaps = sources.getMaps();
  auto islctx = schedule.getCtx();

  if (depPtr)
    *depPtr = islctx->createEmptyUnionMap();
  if (nosrcPtr)
    *nosrcPtr = islctx->createEmptyUnionMap();

  // Unfortunately, UnionMap requires us to know the complete space to query it 
  llvm::DenseMap<const isl_id*, isl::Map> idToScatter;
  for (auto scatter : schedule.getMaps()) {
    auto id = scatter.getInTupleId();
    assert(!idToScatter.count(id.keep()) && "Just one map scatter per stmt");
    idToScatter[id.keep()] = scatter;
  }

  for (auto sink : sinks.getMaps()) {
    // Search the source(s) for this sink

    auto sinkDomain = sink.getDomain(); // { sink[domain] }
    auto sinkScatter = idToScatter[sinkDomain.getTupleId().keep()].intersectDomain(sinkDomain); // { sink[domain] -> var[index] }
    assert(sinkScatter.isValid());
    auto scatterSpace = sinkScatter.getRangeSpace();

    auto notYetAccessed = sink; // { sink[domain] -> var[index] }
    auto depMaps = islctx->createEmptyUnionMap(); // { (sink[domain], source[domain]) -> var[index] }

    for (auto source : sourcesMaps) {
      auto sourceDomain = source.getDomain(); // { source[domain] }
      auto sourceScatter = idToScatter[sourceDomain.getTupleId().keep()].intersectDomain(sourceDomain); // { source[domain] -> var[index] }
      assert(sourceScatter.isValid());

      if (notYetAccessed.getOutTupleId() != source.getOutTupleId())
        continue; // No accessing the same array

      // The elements that both statements access
      auto common = sink.domainProduct(source); // { (sink[domain], source[domain]) -> var[index] }

      // The source is only viable if happing before sink
      auto lexorder = scatterSpace.lexGtMap(); // { sink[scatter] -> source[scatter] }
      auto lexorderDomain = lexorder.applyDomain(sinkScatter.reverse()).applyRange(sourceScatter.reverse()); // { sink[domain] -> source[domain] }
      common.intersectDomain_inplace(lexorderDomain.wrap());
      if (common.isEmpty(isl::Accuracy::Plain).isTrue())
        continue;

      // dependence maps for the next iteration
      auto newDepMaps = depMaps.getSpace().createEmptyUnionMap();

      // Get those accesses for which there is no previous access
      auto notYetCommon = notYetAccessed.domainProduct(source).intersect(common); // { (sink[domain], source[domain]) -> var[index] }
      notYetAccessed.subtract_inplace(notYetCommon.projectOutSubspace(isl_dim_in, sourceDomain.getSpace()));
      //notYetCommon.coalesce_inplace();
      notYetAccessed.coalesce_inplace();
      if (notYetCommon.isEmpty(isl::Accuracy::Plain).maybeFalse())
        newDepMaps.addMap_inplace(notYetCommon);

      // For the others, find which of the two accesses the index first
      for (auto prevDep : depMaps.getMaps()) { // prevDep: { (sink[domain], prev[domain]) -> var[index] }
        auto prevDomain = prevDep.getDomain().unwrap().getRange(); // { prev[domain] }
        auto prevScatter = idToScatter[prevDomain.getTupleId().keep()].intersectDomain(prevDomain); // { prev[domain] -> last[scatter] }
        auto leMap = scatterSpace.lexLeMap();

        auto prevLaterDomain = leMap.applyDomain(sourceScatter.reverse()).applyRange(prevScatter.reverse()); // { source[domain] -> prev[domain] }
        auto prevFlow = common.applyNested(isl_dim_in, prevLaterDomain).intersect(prevDep);
        prevFlow.coalesce_inplace();
        if (prevFlow.isEmpty(isl::Accuracy::Plain).maybeFalse())
          newDepMaps.addMap_inplace(prevFlow);

        auto sourceLaterDomain = leMap.applyDomain(prevScatter.reverse()).applyRange(sourceScatter.reverse()); // { prev[domain] -> source[domain] }
        auto sourceFlow = prevDep.applyNested(isl_dim_in, sourceLaterDomain).intersect(common); // { (sink[domain], source[domain]) -> var[index] }
        sourceFlow.coalesce_inplace();
        if (sourceFlow.isEmpty(isl::Accuracy::Plain).maybeFalse())
          newDepMaps.addMap_inplace(sourceFlow);
      }

      depMaps = newDepMaps.move();
    }

    if (depPtr) {
      for (auto dep : depMaps.getMaps()) {
        depPtr->addMap_inplace(dep.getDomain().unwrap().reverse());
      }
    }
    if (nosrcPtr) {
      nosrcPtr->addMap_inplace(notYetAccessed);
    }
  }
}

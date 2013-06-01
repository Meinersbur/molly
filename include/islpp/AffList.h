#ifndef ISLPP_AFFLIST_H
#define ISLPP_AFFLIST_H

#include "Obj.h"
#include "List.h"
#include "Aff.h"
#include "Ctx.h"
#include "Printer.h"

#include <isl/printer.h>
#include <isl/list.h>

extern "C" {
ISL_DECLARE_LIST_TYPE(aff)
ISL_DECLARE_LIST_FN(aff)
}

//__isl_give isl_aff_list *isl_aff_list_copy(__isl_keep isl_aff_list *);
//void *isl_aff_list_free(__isl_take isl_aff_list *);
//isl_ctx *isl_aff_list_get_ctx(__isl_keep isl_aff_list *);

#include <functional>


namespace isl {

  template<>
  class List<Aff> : public Obj {
#pragma region Low-level
  private:
    isl_aff_list *list;

  public: // Public because otherwise we had to add a lot of friends
    isl_aff_list *take() { assert(list); isl_aff_list *result = list; list = nullptr; return result; }
    isl_aff_list *takeCopy() const { return isl_aff_list_copy(list); }
    isl_aff_list *keep() const { return list; }
  protected:
    void give(isl_aff_list *list) { if (this->list) isl_aff_list_free(list);  this->list = list; }

    explicit List(isl_aff_list *list) : list(list) { }
  public:
    static List<Aff> wrap(isl_aff_list *list) { return List<Aff>(list); }
#pragma endregion

  public:
    List<Aff>() : list(nullptr) {}
    List(const List<Aff> &that) : list(that.takeCopy()) {}
    List(List<Aff> &&that) : list(that.take()) {}
    ~List() {
  if (this->list)
    isl_aff_list_free(this->list);
#ifndef NDEBUG
  this->list = nullptr;
#endif
}

    const List<Aff> &operator=(const List<Aff> &that) { give(that.takeCopy()); return *this; }
    const List<Aff> &operator=(List<Aff> &&that) { give(that.take()); return *this; }

    List<Aff> copy() const { return wrap(takeCopy()); }
    Ctx *getCtx() const { return Ctx::wrap(isl_aff_list_get_ctx(list)); }

#pragma region Creational
    static List<Aff> create(Ctx *ctx, int n) { assert(n >= 0); return wrap(isl_aff_list_alloc(ctx->keep(), n)); }
#pragma endregion

   void add(Aff &&el) { give(isl_aff_list_add(take(), el.take())); }
   void insert(unsigned pos, Aff &&el) { give(isl_aff_list_insert(take(), pos, el.take())); }
   void drop(unsigned first, unsigned n) { give(isl_aff_list_drop(take(), first, n)); }

   Aff getAff(int index) const { return Aff::wrap(isl_aff_list_get_aff(keep(), index)); }
   void setAff(int index, Aff &&el) { give(isl_aff_list_set_aff(take(), index, el.take())); }

   bool foreach(const std::function<bool(Aff &&)> &);
   bool foreachScc(const std::function<bool(const Aff &, const Aff &)> &, const std::function<bool(List<Aff> &&)> &);
   void sort(const std::function<int(const Aff&, const Aff&)> &);


#pragma region Printing
   void printTo(isl::Printer &printer) {
     printer.give(isl_printer_print_aff_list(printer.take(), keep()));
   }
   void dump() const { isl_aff_list_dump(keep()); }
#pragma endregion

  }; // class List<Aff>

  static inline List<Aff> concat(List<Aff> &&list1, List<Aff> &&list2) { return List<Aff>::wrap(isl_aff_list_concat(list1.take(), list2.take())); }

} // namespace isl
#endif /* ISLPP_AFFLIST_H */

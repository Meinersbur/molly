#ifndef MOLLY_FIELDVARIABLE_H
#define MOLLY_FIELDVARIABLE_H

#include "islpp/Islfwd.h"
#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"


namespace molly {
  class IslBasicSet;
  class FieldType;


  class FieldVariable {
  private:
    llvm::GlobalVariable *variable;
    
    FieldType *fieldTy;
    FieldLayout *defaultLayout;

  protected:
    FieldVariable(llvm::GlobalVariable *variable, FieldType *fieldTy);

    isl::Ctx *getIslContext() const;

  public:
    static FieldVariable *create(llvm::GlobalVariable *variable, FieldType *fieldTy) {
      return new FieldVariable(variable, fieldTy);
    }

    void dump();

    llvm::GlobalVariable *getVariable() const { return variable; }
    FieldType *getFieldType() const { return fieldTy; }
    

    isl::Id getTupleId() const;
    isl::Space getAccessSpace() const;

    llvm::Type *getEltType();
    llvm::Type *getEltPtrType();

    //isl::PwMultiAff getHomeAff();

    isl::Map getPhysicalNode() const; // { fvar[domain] -> node[cluster] }
    isl::PwMultiAff getPrimaryPhysicalNode() const; // { fvar[domain] -> node[cluster] }

    // TODO: In future implementations, variables may have a range of layouts
    FieldLayout *getLayout();
    FieldLayout *getDefaultLayout() const;
    void setDefaultLayout(FieldLayout *layout) {
      this->defaultLayout = layout;
    }

  }; // class FieldVariable

} /* namespace molly */
#endif /* MOLLY_FIELDVARIABLE_H */

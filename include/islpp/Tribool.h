#ifndef TRIBOOL_H
#define TRIBOOL_H

//#include <exception>
#include <llvm/Support/ErrorHandling.h>

// TODO: What namespace?

class Tribool {
public:
  enum Consts {
    False,
    True,
    Indeterminate
  };
private:
  Consts state : 2;
public:
  Tribool() { 
#ifndef NDEBUG
    state = (Consts)3; /* non-valid */
#else
    /* state is undefined */ 
#endif
  }
  Tribool(const Tribool &val) : state(val.state) { }
  Tribool(bool val) : state((Consts)val) { }
  Tribool(Consts val) : state(val) { }

  Tribool &operator=(Tribool that) { this->state = that.state; return *this; }
  Tribool &operator=(bool that) { this->state = (Consts)that; return *this; }
  Tribool &operator=(Consts that) { this->state = that; return *this; }

#ifndef NDEBUG
private:
  bool isValid() { return state==False || state==True || state==Indeterminate; }
public:
#endif
  bool isIndeterminate() {  assert(isValid()); return state == Indeterminate; }
  bool isFalse () {assert(isValid()); return state == False; }
  bool isTrue() { assert(isValid());return state==True; }
  bool maybeTrue() { assert(isValid());return !isFalse(); }
  bool maybeFalse() { assert(isValid());return !isTrue(); }
  bool isDetermined() {assert(isValid()); return !isIndeterminate(); }

  bool asBool() { 
    assert(isValid());  
    if (isIndeterminate()) {
      llvm_unreachable("Indetermined boolean");
      //throw new std::exception("Indetermined boolean");
      abort();
    }
    return (bool)state;
  } 

  friend Tribool operator==(Tribool lhs, Tribool rhs);
  friend Tribool operator!=(Tribool lhs, Tribool rhs);
  friend Tribool operator!(Tribool arg);

  // Note that shortcut evaluation is not possible with these
  friend Tribool operator&&(Tribool lhs, Tribool rhs);
  friend Tribool operator||(Tribool lhs, Tribool rhs);
};

inline Tribool operator==(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.isIndeterminate() || rhs.isIndeterminate())
    return Tribool::Indeterminate;
  return lhs.state == rhs.state;
}

inline Tribool operator!=(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.state == Tribool::Indeterminate || rhs.state == Tribool::Indeterminate)
    return Tribool::Indeterminate;
  return lhs.state != rhs.state;
}

inline Tribool operator!(Tribool arg) {
  assert(arg.isValid());
  if (arg.state == Tribool::Indeterminate)
    return Tribool::Indeterminate;
  return !arg.state;
}

inline Tribool operator&&(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.state == Tribool::False || rhs.state == Tribool::False)
    return Tribool::False;
  if (lhs.state == Tribool::True && rhs.state == Tribool::True)
    return Tribool::True;
  return Tribool::Indeterminate;
}

inline Tribool operator||(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.state == Tribool::True || rhs.state == Tribool::True)
    return Tribool::True;
  if (lhs.state == Tribool::False && rhs.state == Tribool::False)
    return Tribool::False;
  return Tribool::Indeterminate;
}

#endif /* TRIBOOL_H */

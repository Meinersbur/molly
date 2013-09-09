#ifndef TRIBOOL_H
#define TRIBOOL_H

//#include <exception>
#include <llvm/Support/ErrorHandling.h>

// TODO: What namespace?

// C++  does not allow declare a friend function with static qualifier -- have to declare it before so friend knows it is static (inline)
class Tribool;
static Tribool operator==(Tribool lhs, Tribool rhs);
static Tribool operator!=(Tribool lhs, Tribool rhs);
static Tribool operator!(Tribool arg);
static Tribool operator&&(Tribool lhs, Tribool rhs);
static inline Tribool operator||(Tribool lhs, Tribool rhs);


class Tribool {
public:
  enum Consts {
    False = 0,
    True = 1,
    Indeterminate = 2
  };
private:
  Consts state;
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

  static Tribool maybeFalsePositive(bool that) { return that ? Indeterminate : False; }
  static Tribool maybeFalseNegative(bool that) { return that ? True : Indeterminate; }

#ifndef NDEBUG
private:
  bool isValid() const { return state==False || state==True || state==Indeterminate; }
public:
#endif
  bool isIndeterminate() const { assert(isValid()); return state==Indeterminate; }
  bool isFalse () const { assert(isValid()); return state==False; }
  bool isTrue() const { assert(isValid()); return state==True; }
  bool maybeTrue() const { assert(isValid()); return !isFalse(); }
  bool maybeFalse() const { assert(isValid()); return !isTrue(); }
  bool isDetermined() const { assert(isValid()); return !isIndeterminate(); }

  bool asBool() const { 
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

static inline Tribool operator==(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.isIndeterminate() || rhs.isIndeterminate())
    return Tribool::Indeterminate;
  return lhs.state == rhs.state;
}

static inline Tribool operator!=(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.isIndeterminate() || rhs.isIndeterminate())
    return Tribool::Indeterminate;
  return lhs.state != rhs.state;
}

static inline Tribool operator!(Tribool arg) {
  assert(arg.isValid());
  if (arg.isIndeterminate())
    return Tribool::Indeterminate;
  return !arg.state;
}

static inline Tribool operator&&(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.state==Tribool::False || rhs.state==Tribool::False)
    return Tribool::False;
  if (lhs.state==Tribool::True && rhs.state==Tribool::True)
    return Tribool::True;
  return Tribool::Indeterminate;
}

static inline Tribool operator||(Tribool lhs, Tribool rhs) {
  assert(lhs.isValid());
  assert(rhs.isValid());
  if (lhs.state==Tribool::True || rhs.state==Tribool::True)
    return Tribool::True;
  if (lhs.state==Tribool::False && rhs.state==Tribool::False)
    return Tribool::False;
  return Tribool::Indeterminate;
}

#endif /* TRIBOOL_H */

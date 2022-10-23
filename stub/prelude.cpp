// place holder file
#include "sel/engine.hpp"
#include "sel/visitors.hpp"
#include "prelude.hpp"

namespace sel {

  Val* lookup_name(Environment& env, std::string const& name) {
    return new Add2();
  }

  Fun* lookup_unary(Environment& env, std::string const& name) {
    return new Abs1();
  }

  Fun* lookup_binary(Environment& env, std::string const& name) {
    return new Abs1();
  }

  Val* Abs1::operator()(Environment& env, Val* arg) {
    return new Abs0(this, coerse<Num>(arg));
  }
  void Abs1::accept(Visitor& v) const {
    v.visitAbs1(type());
  }

  double Abs0::value() {
    return std::abs(arg->value());
  }
  void Abs0::accept(Visitor& v) const {
    v.visitAbs0(type(), base, arg);
  }

  Val* Add2::operator()(Environment& env, Val* arg) {
    return new Add1(this, coerse<Num>(arg));
  }
  void Add2::accept(Visitor& v) const {
    v.visitAdd2(type());
  }

  Val* Add1::operator()(Environment& env, Val* arg) {
    return new Add0(this, coerse<Num>(arg));
  }
  void Add1::accept(Visitor& v) const {
    v.visitAdd1(type(), base, arg);
  }

  double Add0::value() {
    return base->arg->value() + arg->value();
  }
  void Add0::accept(Visitor& v) const {
    v.visitAdd0(type(), base, arg);
  }

  Val* Join2::operator()(Environment& env, Val* arg) {
    return new Join1(this, coerse<Str>(arg));
  }
  void Join2::accept(Visitor& v) const { }

  Val* Join1::operator()(Environment& env, Val* arg) {
    return new Join0(this, coerse<Lst>(arg)); // TODO: as expected, coerse to lst will have to take more info
  }
  void Join1::accept(Visitor& v) const { }

  std::ostream& Join0::stream(std::ostream& out) {
    Str& sep = *base->arg;
    Lst& lst = *arg;
    if (!beginning) out << sep;
    return ((Str*)*(++lst))->full(out);
  }
  bool Join0::end() const {
    Lst& lst = *arg;
    return lst.end();
  }
  void Join0::rewind() {
    Lst& lst = *arg;
    lst.rewind();
    beginning = true;
  }
  std::ostream& Join0::full(std::ostream& out) {
    Str& sep = *base->arg;
    Lst& lst = *arg;
    if (lst.end()) return out;
    out << *lst;
    while (!lst.end()) {
      sep.rewind();
      out << sep << *(Str*)*(++lst);
    }
    return out;
  }
  void Join0::accept(Visitor& v) const { }

  void Map2::accept(Visitor& v) const { }
  Val* Map2::operator()(Environment& env, Val* arg) {
    return new Map1(this, coerse<Fun>(arg));
  }

  void Map1::accept(Visitor& v) const { }
  Val* Map1::operator()(Environment& env, Val* arg) {
    return new Map0(this, coerse<Lst>(arg));
  }

  Val* Map0::operator*() {
    Fun& fun = *base->arg;
    Lst& lst = *arg;
    return fun(env, lst)
  }
  Lst& Map0::operator++() {
  }
  bool Map0::end() const {
  }
  void Map0::rewind() {
  }
  size_t Map0::count() {
  }
  void Map0::accept(Visitor& v) const { }

} // namespace sel

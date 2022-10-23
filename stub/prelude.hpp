// place holder file
#ifndef SEL_PRELUDE_HPP
#define SEL_PRELUDE_HPP

#include <string>

#include "sel/utils.hpp"
#include "sel/engine.hpp"

namespace sel {

  // return may be nullptr
  Val* lookup_name(Environment& env, std::string const& name);
  Fun* lookup_unary(Environment& env, std::string const& name);
  Fun* lookup_binary(Environment& env, std::string const& name);

  struct Abs1 : Fun {
    Abs1()
      : Fun(numType(), numType())
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Abs0 : Num {
    Abs1* base;
    Num* arg;
    Abs0(Abs1* base, Num* arg)
      : base(base)
      , arg(arg)
    { }
    double value() override;
    void accept(Visitor& v) const override;
  };

  struct Add2 : Fun {
    Add2()
      : Fun(numType(), funType(new Type(numType()), new Type(numType())))
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Add1 : Fun {
    Add2* base;
    Num* arg;
    Add1(Add2* base, Num* arg)
      : Fun(numType(), numType())
      , base(base)
      , arg(arg)
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Add0 : Num {
    Add1* base;
    Num* arg;
    Add0(Add1* base, Num* arg)
      : base(base)
      , arg(arg)
    { }
    double value() override;
    void accept(Visitor& v) const override;
  };

  // join :: Str -> [Str]* -> Str*
  struct Join2 : Fun {
    Join2()
      : Fun(
          strType(TyFlag::IS_FIN),
          funType(
            new Type(lstType(new Type(strType(TyFlag::IS_FIN)), TyFlag::IS_INF)),
            new Type(strType(TyFlag::IS_INF))
          )
        )
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Join1 : Fun {
    Join2* base;
    Str* arg;
    Join1(Join2* base, Str* arg)
      : Fun(
        lstType(new Type(strType(TyFlag::IS_FIN)), TyFlag::IS_INF),
        strType(TyFlag::IS_INF)
      )
      , base(base)
      , arg(arg)
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Join0 : Str {
    Join1* base;
    Lst* arg;
    bool beginning;
    Join0(Join1* base, Lst* arg)
      : Str((TyFlag)arg->type().flags)
      , base(base)
      , arg(arg)
      , beginning(true)
    { }
    std::ostream& stream(std::ostream& out) override;
    bool end() const override;
    void rewind() override;
    std::ostream& full(std::ostream& out) override;
    void accept(Visitor& v) const override;
  };

  // map :: (a -> b) -> [a]* -> [b]*
  struct Map2 : Fun {
    Environment* env;
    Map2()
      : Fun(
          funType(
            new Type(unkType(new std::string("a"))),
            new Type(unkType(new std::string("b")))
          ),
          funType(
            new Type(lstType(new Type(unkType(new std::string("a"))), TyFlag::IS_INF)),
            new Type(lstType(new Type(unkType(new std::string("b"))), TyFlag::IS_INF))
          )
        )
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Map1 : Fun {
    Map2* base;
    Fun* arg;
    Map1(Map2* base, Fun* arg)
      : Fun(
          lstType(new Type(arg->type().from()), TyFlag::IS_INF),
          lstType(new Type(arg->type().to()), TyFlag::IS_INF)
        )
      , base(base)
      , arg(arg)
    { }
    Val* operator()(Environment& env, Val* arg) override;
    void accept(Visitor& v) const override;
  };
  struct Map0 : Lst {
    Map1* base;
    Lst* arg;
    Map0(Map1* base, Lst* arg)
      : Lst(Type(arg->type()), (TyFlag)arg->type().flags)
      , base(base)
      , arg(arg)
    { }
    Val* operator*() override;
    Lst& operator++() override;
    bool end() const override;
    void rewind() override;
    size_t count() override;
    void accept(Visitor& v) const override;
  };

}

#endif // SEL_PRELUDE_HPP

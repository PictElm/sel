#include "common.hpp"

TEST(parseType) {
  char const* source = "fn :: (Num -> Str*) -> Num -> ([Str]*, [Str*])";

  auto tpl = new std::vector<Type*>();
  tpl->push_back(new Type(lstType(new Type(strType(TyFlag::IS_FIN)), TyFlag::IS_INF)));
  tpl->push_back(new Type(lstType(new Type(strType(TyFlag::IS_INF)), TyFlag::IS_FIN)));
  Type const expect = funType(
    new Type(funType(
      new Type(numType()),
      new Type(strType(TyFlag::IS_INF))
    )),
    new Type(funType(
      new Type(numType()),
      new Type(lstType(tpl, TyFlag::IS_FIN, TyFlag::IS_TPL))
    ))
  );

  istringstream iss(source);
  Type result;
  string name;
  parseType(iss, &name, result);

  cout
    << "  source: " << source << endl
    << "  expect: fn :: " << expect << endl
    << "  result: " << name << " :: " << result << endl
    << "compares equal: " << (expect == result) << endl
  ;

  assert_eq(expect, result);

  ostringstream oss;
  oss << name << " :: " << result;
  string a = source;
  string b = oss.str();
  assert_eq(a, b);
}

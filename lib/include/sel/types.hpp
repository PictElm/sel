#ifndef SEL_TYPE_HPP
#define SEL_TYPE_HPP

/**
 * Parsing and representation of types, as well as a set
 * of helper functions to deal with the type system.
 */

#include <ostream>
#include <istream>
#include <string>
#include <vector>
#include <iterator>

namespace sel {

  enum class Ty {
    UNK,
    NUM,
    STR,
    LST,
    FUN,
  };
  enum TyFlag {
    IS_FIN = 0,
    IS_INF = 1,
    IS_TPL = 2,
  };

  /**
   * Represents a type (yey). Use the [..]Type function
   * to construct/parse.
   */
  struct Type {
    Ty base = Ty::UNK; // YYY: meh
    union P {
      std::string* name;
      std::vector<Type*>* box_has;
      Type* box_pair[2];
    } p = {.name=nullptr};
    uint8_t flags = TyFlag::IS_FIN;

    Type();
    Type(Ty base, Type::P p, uint8_t flags);
    Type(Type const& ty);
    Type(Type&& ty) noexcept;
    ~Type();

    bool operator==(Type const& other) const;
    bool operator!=(Type const& other) const;

    std::vector<Type*> const& has() { return *p.box_has; }
    Type const& from() const { return *p.box_pair[0]; }
    Type const& to() const { return *p.box_pair[1]; }

    bool isInfinite() { return TyFlag::IS_INF & flags; }
    bool isTuple() { return TyFlag::IS_TPL & flags; }
  };

  Type unkType(std::string* name);
  Type numType();
  Type strType(TyFlag is_inf);

  Type lstType(Ty has, TyFlag is_inf); //
  Type lstType(Type* has, TyFlag is_inf);
  Type lstType(std::vector<Ty> has, TyFlag is_inf, TyFlag is_tpl); //
  Type lstType(std::vector<Type*>* has, TyFlag is_inf, TyFlag is_tpl);
  Type lstType(TyFlag is_inf, TyFlag is_tpl); //

  Type funType(Ty from, Ty to); //
  Type funType(Ty from, Type* to); //
  Type funType(Type* from, Ty to); //
  Type funType(Type* from, Type* to);

  /**
   * Parse a type from the given stream. The overload to
   * the `>>` operator is also provided as a convenience,
   * but this function allows retreiving a name
   * (which would be specified with the `<name> ::`
   * syntax). If `named` is not nullptr but the parsed
   * type representation does not specify a name, the
   * empty string is written into `named`.
  */
  void parseType(std::istream& in, std::string* named, Type& res);

  std::ostream& operator<<(std::ostream& out, Type const& ty);
  std::istream& operator>>(std::istream& in, Type& res);

} // namespace sel

#endif // SEL_TYPE_HPP

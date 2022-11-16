#include <typeinfo>

#include "sel/builtins.hpp"
#include "sel/visitors.hpp"

namespace sel {

  template <typename list> struct linear_search;
  template <typename car, typename cdr>
  struct linear_search<bin_types::cons<car, cdr>> {
    static inline Val* the(std::string const& name) {
      if (car::name == name) return new typename car::Head();
      return linear_search<cdr>::the(name);
    }
  };
  template <>
  struct linear_search<bin_types::nil> {
    static inline Val* the(std::string const& _) {
      return nullptr;
    }
  };

  Val* lookup_name(std::string const& name) {
    return linear_search<bin_types::bins>::the(name);
  }

  // Val* lookup_name(std::string const& name) {
  //   // if ("abs"    == name) return new Abs1();
  //   if ("add"    == name) return new bin::Add::Head();
  //   // if ("flip"   == name) return new Flip2();
  //   // if ("idk"    == name) return new bin::Idk::Head();
  //   // if ("join"   == name) return new Join2();
  //   if ("map"    == name) return new bin::Map::Head();
  //   if ("repeat" == name) return new bin::Repeat::Head();
  //   // if ("split"  == name) return new Split2();
  //   // if ("sub"    == name) return new bin::Sub::Head();
  //   if ("tonum"  == name) return new bin::Tonum::Head();
  //   // if ("tostr"  == name) return new Tostr1();
  //   return nullptr;
  // }

  template <typename NextT, typename from1, typename from2, typename to>
  void bins_helpers::_bin_be<NextT, bins_helpers::fun<from1, bins_helpers::fun<from2, to>>>::accept(Visitor& v) const {
    v.visit(*this); // visitBody
  }

  template <typename NextT, typename from, typename to>
  void bins_helpers::_bin_be<NextT, bins_helpers::fun<from, to>>::the::accept(Visitor& v) const {
    v.visit(*(typename Base::Next*)this); // visitTail
  }

  template <typename NextT, typename from, typename to>
  void bins_helpers::_bin_be<NextT, bins_helpers::fun<from, to>>::accept(Visitor& v) const{
    v.visit(*this); // visitHead
  }

} // namespace sel

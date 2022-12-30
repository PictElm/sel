#include "sel/builtins.hpp"
#include "sel/visitors.hpp"

#include <cmath>

namespace sel {

  void StrChunks::accept(Visitor& v) const {
    v.visitStrChunks(type(), this->chunks);
  }

  // internal
  template <typename list> struct linear_search;
  template <typename car, typename cdr>
  struct linear_search<bins_ll::cons<car, cdr>> {
    static inline Val* the(std::string const& name) {
      if (car::name == name) return new typename car::Head();
      return linear_search<cdr>::the(name);
    }
  };
  template <>
  struct linear_search<bins_ll::nil> {
    static inline Val* the(std::string const& _) {
      return nullptr;
    }
  };

  Val* lookup_name(std::string const& name) {
    return linear_search<bins_ll::bins>::the(name);
  }

  // internal
  template <typename list> struct push_names_into;
  template <typename car, typename cdr>
  struct push_names_into<bins_ll::cons<car, cdr>> {
    static inline void the(std::vector<std::string>& name) {
      name.push_back(car::name);
      push_names_into<cdr>::the(name);
    }
  };
  template <>
  struct push_names_into<bins_ll::nil> {
    static inline void the(std::vector<std::string>& _) { }
  };

  unsigned list_names(std::vector<std::string>& names) {
    constexpr unsigned count = ll::count<bins_ll::bins>::the;
    names.reserve(count);
    push_names_into<bins_ll::bins>::the(names);
    return count;
  }

  namespace bins_helpers {

    template <typename Impl, typename one>
    void _bin_be<Impl, ll::cons<one, ll::nil>>::the::accept(Visitor& v) const {
      v.visit(*(Impl*)this); // visitOne
    }

    template <typename Impl, typename last_arg, char b>
    void _bin_be<Impl, cons<fun<last_arg, unk<b>>, nil>>::the::accept(Visitor& v) const {
      v.visit(*(Impl*)this); // visitOne2
    }

    template <typename NextT, typename to, typename from, typename from_again, typename from_more>
    void _bin_be<NextT, ll::cons<to, ll::cons<from, ll::cons<from_again, from_more>>>>::accept(Visitor& v) const {
      v.visit(*this); // visitBody
    }

    template <typename NextT, typename last_to, typename last_from>
    void _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::the::accept(Visitor& v) const {
      v.visit(*(typename Base::Next*)this); // visitTail
    }

    template <typename NextT, typename last_to, typename last_from>
    void _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::accept(Visitor& v) const {
      v.visit(*this); // visitHead
    }

  } // namespace bins_helpers

#define _depth(__depth) _depth_ ## __depth
#define _depth_0 arg
#define _depth_1 base->_depth_0
#define _depth_2 base->_depth_1
#define _depth_3 base->_depth_2

#define _bind_some(__count) _bind_some_ ## __count
#define _bind_some_1(a)          _bind_one(a, 0)
#define _bind_some_2(a, b)       _bind_one(a, 1); _bind_some_1(b)
#define _bind_some_3(a, b, c)    _bind_one(a, 2); _bind_some_2(b, c)
#define _bind_some_4(a, b, c, d) _bind_one(a, 3); _bind_some_3(b, c, d)

#define _bind_count(__count, ...) _bind_some(__count)(__VA_ARGS__)
#define _bind_one(__name, __depth) auto& __name = *this->_depth(__depth); (void)__name
// YYY: could it somehow be moved into `BIN_...`? in a way
// that it is only written once and the named arg refs
// are available all throughout the struct
#define bind_args(...) _bind_count(__VA_COUNT(__VA_ARGS__), __VA_ARGS__)

  namespace bins {

    double abs_::value() {
      return std::abs(arg->value());
    }

    double add_::value() {
      bind_args(a, b);
      return a.value() + b.value();
    }

    Val* const_::impl() {
      bind_args(take, ignore);
      return &take;
    }

    Val* drop_::operator*() {
      bind_args(n, l);
      if (!done) {
        for (size_t k = 0; k < n.value() && !l.end(); k++)
          ++l;
        done = true;
      }
      return *l;
    }
    Lst& drop_::operator++() {
      bind_args(n, l);
      if (!done) {
        for (size_t k = 0; k < n.value() && !l.end(); k++)
          ++l;
        done = true;
      }
      ++l;
      return *this;
    }
    bool drop_::end() const {
      bind_args(n, l);
      if (done) return l.end();
      // tldw: it should still be `false` when we just
      // reached `l.end()`, because that means there is
      // a value to deref (end just means there is no
      // iterating anymore)
      bool wasnt_end = true;
      size_t k;
      for (k = 0; k < n.value() && (wasnt_end = !l.end()); k++)
        ++l;
      done = true;
      return l.end() && (!wasnt_end || n.value() != k);
    }

    Val* dropwhile_::operator*() {
      bind_args(p, l);
      if (!done) {
        while (!l.end() && p(*l))
          ++l;
        done = true;
      }
      return *l;
    }
    Lst& dropwhile_::operator++() {
      bind_args(p, l);
      if (!done) {
        while (!l.end() && p(*l))
          ++l;
        done = true;
      }
      ++l;
      return *this;
    }
    bool dropwhile_::end() const {
      bind_args(p, l);
      // TODO: update (no proper way to make predicates for now..)
      return done && l.end();
    }

    Val* filter_::operator*() {
      bind_args(p, l);
      if (!curr) {
        while (!((Num*)p(*l))->value()) ++l;
        curr = *l;
      }
      return *l;
    }
    Lst& filter_::operator++() {
      bind_args(p, l);
      ++l;
      while (!((Num*)p(*l))->value()) ++l;
      curr = *l;
      return *this;
    }
    bool filter_::end() const {
      bind_args(p, l);
      return l.end();
    }

    Val* flip_::impl() {
      bind_args(fun, b, a);
      return (*(Fun*)fun(&a))(&b);
    }

    Val* id_::impl() {
      bind_args(take);
      return &take;
    }

    Val* if_::impl() {
      bind_args(condition, consequence, alternative, argument);
      return ((Num*)condition(&argument))->value()
        ? &consequence
        : &alternative;
    }

    Val* iterate_::operator*() {
      bind_args(f, o);
      return !curr ? &o : curr;
    }
    Lst& iterate_::operator++() {
      bind_args(f, o);
      curr = f(!curr ? &o : curr);
      return *this;
    }
    bool iterate_::end() const { return false; }

    std::ostream& join_::stream(std::ostream& out) {
      bind_args(sep, lst);
      if (beginning) beginning = false;
      else sep.entire(out);
      Str* it = (Str*)*lst;
      it->entire(out);
      ++lst;
      return out;
    }
    bool join_::end() const {
      bind_args(sep, lst);
      return lst.end();
    }
    std::ostream& join_::entire(std::ostream& out) {
      bind_args(sep, lst);
      if (lst.end()) return out;
      Str* it = (Str*)(*lst);
      it->entire(out);
      while (!lst.end()) {
        sep.entire(out);
        ++lst;
        it = (Str*)(*lst);
        it->entire(out);
      }
      return out;
    }

    Val* map_::operator*() {
      bind_args(f, l);
      if (!curr) curr = f(*l);
      return curr;
    }
    Lst& map_::operator++() {
      bind_args(f, l);
      curr = nullptr;
      ++l;
      return *this;
    }
    bool map_::end() const {
      bind_args(f, l);
      return l.end();
    }

    std::ostream& nl_::stream(std::ostream& out) {
      bind_args(s);
      // return !s.end() ? out << s : (done = true, out << '\n');
      done = true;
      return s.entire(out) << '\n';
    }
    bool nl_::end() const { return done; }
    std::ostream& nl_::entire(std::ostream& out) {
      bind_args(s);
      done = true;
      return s.entire(out) << '\n';
    }

    double pi_::value() {
      return M_PI;
    }

    Val* repeat_::operator*() { return arg; }
    Lst& repeat_::operator++() { return *this; }
    bool repeat_::end() const { return false; }

    Val* replicate_::operator*() {
      if (!did) did++;
      bind_args(n, o);
      return &o;
    }
    Lst& replicate_::operator++() {
      did++;
      return *this;
    }
    bool replicate_::end() const {
      bind_args(n, o);
      return did >= n.value();
    }

    void reverse_::once() {
      bind_args(l);
      if (!l.end()) {
        cache.push_back(*l);
        while (!l.end())
          cache.push_back(*(++l));
      }
      did_once = true;
      curr = cache.size()-1;
    }
    Val* reverse_::operator*() {
      if (!did_once) once();
      return cache[curr];
    }
    Lst& reverse_::operator++() {
      curr--;
      return *this;
    }
    bool reverse_::end() const {
      if (did_once) return 0 == curr;
      bind_args(l);
      return l.end();
    }

    Val* singleton_::operator*() {
      done = true;
      return arg;
    }
    Lst& singleton_::operator++() {
      done = true;
      return *this;
    }
    bool singleton_::end() const { return done; }

    void split_::once() {
      bind_args(sep, str);
      std::ostringstream oss;
      sep.entire(oss);
      ssep = oss.str();
      did_once = true;
    }
    void split_::next() {
      if (!did_once) once();
      bind_args(sep, str);
      std::string buf = acc.str();
      std::string::size_type at = buf.find(ssep);
      if (std::string::npos != at) {
        // found in current acc, pop(0)
        curr = buf.substr(0, at);
        acc = std::ostringstream(buf.substr(at+ssep.size()));
        return;
      }
      if (str.end()) {
        // send the rest of acc, set end
        curr = buf;
        at_end = true;
        return;
      }
      acc << str;
      return next();
    }
    Val* split_::operator*() {
      if (!init) { next(); init = true; }
      return new StrChunks(curr);
    }
    Lst& split_::operator++() {
      if (!init) { next(); init = true; }
      next();
      return *this;
    }
    bool split_::end() const {
      return at_end;
    }

    double sub_::value() {
      bind_args(a, b);
      return a.value() - b.value();
    }

    Val* take_::operator*() {
      if (!did) did++;
      bind_args(n, l);
      return *l;
    }
    Lst& take_::operator++() {
      bind_args(n, l);
      did++;
      ++l;
      return *this;
    }
    bool take_::end() const {
      bind_args(n, l);
      return did >= n.value() || l.end();
    }

    Val* takewhile_::operator*() {
      bind_args(p, l);
      return *l;
    }
    Lst& takewhile_::operator++() {
      bind_args(p, l);
      ++l;
      return *this;
    }
    bool takewhile_::end() const {
      bind_args(p, l);
      return l.end() || !p(*l);
    }

    double tonum_::value() {
      if (!done) {
        bind_args(s);
        std::stringstream ss;
        s.entire(ss);
        ss >> r;
        done = true;
      }
      return r;
    }

    std::ostream& tostr_::stream(std::ostream& out) { read = true; return out << arg->value(); }
    bool tostr_::end() const { return read; }
    std::ostream& tostr_::entire(std::ostream& out) { read = true; return out << arg->value(); }

    Val* zipwith_::operator*() {
      bind_args(f, l1, l2);
      if (!curr) curr = (*(Fun*)f(*l1))(*l2);
      return curr;
    }
    Lst& zipwith_::operator++() {
      bind_args(f, l1, l2);
      curr = nullptr;
      ++l1;
      ++l2;
      return *this;
    }
    bool zipwith_::end() const {
      bind_args(f, l1, l2);
      return l1.end() || l2.end();
    }

  } // namespace bins

} // namespace sel

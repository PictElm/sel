#include "sel/builtins.hpp"
#include "sel/visitors.hpp"

#include <cmath>

namespace sel {

  Val* StrChunks::copy() const {
    return new StrChunks(chunks);
  }
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
    Val* _bin_be<Impl, ll::cons<one, ll::nil>>::the::copy() const {
      TRACE(copyOne, typeid(*this).name());
      return new typename _bin_be<Impl, ll::cons<one, ll::nil>>::the::Base::Next(); // copyOne
    }
    template <typename Impl, typename one>
    void _bin_be<Impl, ll::cons<one, ll::nil>>::the::accept(Visitor& v) const {
      v.visit(*(Impl*)this); // visitOne
    }

    template <typename Impl, typename last_arg, char b>
    Val* _bin_be<Impl, cons<fun<last_arg, unk<b>>, nil>>::the::copy() const {
      TRACE(copyOne2, typeid(*this).name());
      return new typename _bin_be<Impl, cons<fun<last_arg, unk<b>>, nil>>::the::Base::Next(); // copyOne2
    }
    template <typename Impl, typename last_arg, char b>
    void _bin_be<Impl, cons<fun<last_arg, unk<b>>, nil>>::the::accept(Visitor& v) const {
      v.visit(*(Impl*)this); // visitOne2
    }

    template <typename NextT, typename to, typename from, typename from_again, typename from_more>
    Val* _bin_be<NextT, ll::cons<to, ll::cons<from, ll::cons<from_again, from_more>>>>::copy() const {
      typedef _bin_be<NextT, ll::cons<to, ll::cons<from, ll::cons<from_again, from_more>>>> a;
      TRACE(copyBody, typeid(*this).name());
      return new _bin_be<NextT, ll::cons<to, ll::cons<from, ll::cons<from_again, from_more>>>>(
        (a::Base*)base->copy(),
        (a::Arg*)arg->copy()
      ); // copyBody
    }
    template <typename NextT, typename to, typename from, typename from_again, typename from_more>
    void _bin_be<NextT, ll::cons<to, ll::cons<from, ll::cons<from_again, from_more>>>>::accept(Visitor& v) const {
      v.visit(*this); // visitBody
    }

    template <typename NextT, typename last_to, typename last_from>
    Val* _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::_the_when_not_unk::copy() const {
      typedef _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::the a;
      TRACE(copyTail1, typeid(*this).name());
      return new typename a::Base::Next(
        (typename a::Base*)base->copy(),
        (typename a::Arg*)arg->copy()
      ); // copyTail1
    }
    template <typename NextT, typename last_to, typename last_from>
    Val* _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::_the_when_is_unk::copy() const {
      typedef _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::the a;
      TRACE(copyTail2, typeid(*this).name());
      return new typename a::Base::Next(
        _base.base,
        _base.arg
      ); // copyTail2
    }
    template <typename NextT, typename last_to, typename last_from>
    void _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::the::accept(Visitor& v) const {
      v.visit(*(typename Base::Next*)this); // visitTail
    }

    template <typename NextT, typename last_to, typename last_from>
    Val* _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>::copy() const {
      TRACE(copyHead, typeid(*this).name());
      return new _bin_be<NextT, ll::cons<last_to, ll::cons<last_from, ll::nil>>>(); // copyHead
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

    void conjunction_::once() {
      bind_args(l, r);
      while (!l.end()) {
        std::ostringstream oss;
        ((Str*)*l)->entire(oss);
        inleft.insert(oss.str());
        ++l;
      }
      while (!r.end()) {
        std::ostringstream oss;
        ((Str*)*r)->entire(oss);
        if (inleft.end() != inleft.find(oss.str())) break;
        ++r;
      }
      did_once = true;
    }
    Val* conjunction_::operator*() {
      bind_args(l, r);
      if (!did_once) once();
      return *r;
    }
    Lst& conjunction_::operator++() {
      bind_args(l, r);
      if (!did_once) once();
      ++r;
      while (!r.end()) {
        std::ostringstream oss;
        ((Str*)*r)->entire(oss);
        if (inleft.end() != inleft.find(oss.str())) break;
        ++r;
      }
      return *this;
    }
    bool conjunction_::end() const {
      bind_args(l, r);
      return (did_once ? inleft.empty() : l.end()) || r.end();
    }

    Val* const_::impl() {
      bind_args(take, ignore);
      return &take;
    }

    double div_::value() {
      bind_args(a, b);
      return a.value() / b.value();
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
      size_t k;
      for (k = 0; k < n.value() && !l.end(); k++)
        ++l;
      done = true;
      return l.end() && n.value() != k;
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
      if (done) return l.end();
      // XXX: untested (still no proper predicate to facilitate)
      while (!l.end() && p(*l))
        ++l;
      done = true;
      return l.end();
    }

    Val* filter_::operator*() {
      bind_args(p, l);
      if (!curr) {
        while (!l.end() && !((Num*)p(*l))->value()) ++l;
        curr = *l;
      }
      return *l;
    }
    Lst& filter_::operator++() {
      bind_args(p, l);
      ++l;
      while (!l.end() && !((Num*)p(*l))->value()) ++l;
      curr = *l;
      return *this;
    }
    bool filter_::end() const {
      bind_args(p, l);
      // TODO/FIXME/...
      // XXX: still dont like this model: no way to tell
      // if we are at the end without scanning but then
      // this is no lazy at all..?
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
      if (beginning) {
        std::ostringstream oss;
        sep.entire(oss);
        ssep = oss.str();
        beginning = false;
      } else out << ssep;
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
      if (beginning) {
        std::ostringstream oss;
        sep.entire(oss);
        ssep = oss.str();
        beginning = false;
      }
      // first iteration unrolled (because no separator)
      ((Str*)*lst)->entire(out);
      ++lst;
      for (; !lst.end(); ++lst) {
        ((Str*)*lst)->entire(out << ssep);
      }
      return out;
    }

    Val* map_::operator*() {
      bind_args(f, l);
      return f(*l);
    }
    Lst& map_::operator++() {
      bind_args(f, l);
      ++l;
      return *this;
    }
    bool map_::end() const {
      bind_args(f, l);
      return l.end();
    }

    double mul_::value() {
      bind_args(a, b);
      return a.value() * b.value();
    }

    std::ostream& nl_::stream(std::ostream& out) {
      bind_args(s);
      return !s.end() ? out << s : (done = true, out << '\n');
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
      return 0 == n.value() || n.value() < did;
    }

    void reverse_::once() {
      bind_args(l);
      if (!l.end()) {
        cache.push_back(*l);
        while (!l.end())
          cache.push_back(*(++l));
      }
      did_once = true;
      curr = cache.size();
    }
    Val* reverse_::operator*() {
      if (!did_once) once();
      return cache[curr-1];
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

    Val* singleton_::operator*() { return arg; }
    Lst& singleton_::operator++() { done = true; return *this; }
    bool singleton_::end() const { return done; }

    void split_::once() {
      bind_args(sep, str);
      std::ostringstream oss;
      sep.entire(oss);
      ssep = oss.str();
      did_once = true;
    }
    void split_::next() {
      if (at_end) {
        at_past_end = true;
        return;
      }
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
      return at_past_end;
    }

    double startswith_::value() {
      if (!done) {
        bind_args(prefix, str);
        std::ostringstream osspx;
        osspx << prefix;
        std::string px = osspx.str();
        std::ostringstream oss;
        // BOF: copies! copies everywhere~! (or does it? sais its a temporary object...)
        while (oss.str().length() < px.length() && 0 == px.compare(0, oss.str().length(), oss.str()) && !str.end()) {
          oss << str;
        }
        does = 0 == oss.str().compare(0, px.length(), px);
        done = true;
      }
      return does;
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
      return n.value() < did || l.end();
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
      return l.end() || !((Num*)p(*l))->value();
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

    Val* uncurry_::impl() {
      bind_args(f, pair);
      return (*(Fun*)f(*pair))(*++pair);
    }

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

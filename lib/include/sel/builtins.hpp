#ifndef SEL_BUILTINS_HPP
#define SEL_BUILTINS_HPP

#include <sstream>
#include <vector>
#include <unordered_set>

#include "utils.hpp"
#include "engine.hpp"
#include "errors.hpp"

namespace sel {

  class StrChunks : public Str {
    typedef std::vector<std::string> ch_t;
    ch_t chunks;
    ch_t::size_type at;
  public:
    StrChunks(std::vector<std::string> chunks)
      : Str(TyFlag::IS_FIN)
      , chunks(chunks)
      , at(0)
    { }
    StrChunks(std::string single)
      : StrChunks(std::vector<std::string>({single}))
    { }
    std::ostream& stream(std::ostream& out) override {
      return out << chunks[at++];
    }
    bool end() const override {
      return chunks.size() <= at;
    }
    std::ostream& entire(std::ostream& out) override {
      for (auto const& it : chunks)
        out << it;
      return out;
    }
    Val* copy() const override;
    void accept(Visitor& v) const override;
  };

  /**
   * Seach for a value by name, return nullptr if not found.
   */
  Val* lookup_name(std::string const& name);

  /**
   * Seach for a value by name, return nullptr if not found.
   */
  unsigned list_names(std::vector<std::string>& names);

  /**
   * (mostly internal) namespace for TMP linked list
   */
  namespace ll {

    /**
     * empty list, see `ll::cons`
     */
    struct nil { };

    /**
     * linked lists of type (car,cdr) style, see `ll::nil`
     */
    template <typename A, typename D>
    struct cons { typedef A car; typedef D cdr; };
    template <typename O>
    struct cons<O, nil> { typedef O car; typedef nil cdr; };

    /**
     * make a linked lists of types (car,cdr) style from pack (see `ll::cons`)
     */
    template <typename H, typename... T>
    struct cons_l { typedef cons<H, typename cons_l<T...>::the> the; };
    template <typename O>
    struct cons_l<O> { typedef cons<O, nil> the; };

    template <typename from, typename into> struct _rev_impl;
    /**
     * reverse a list of types
     */
    template <typename list>
    struct rev { typedef typename _rev_impl<list, nil>::the the; };
    template <typename into>
    struct _rev_impl<nil, into> { typedef into the; };
    template <typename H, typename T, typename into>
    struct _rev_impl<cons<H, T>, into> { typedef typename _rev_impl<T, cons<H, into>>::the the; };

    /**
     * count the number of element
     */
    template <typename list> struct count;
    template <typename head, typename tail>
    struct count<cons<head, tail>> { static constexpr unsigned the = count<tail>::the+1; };
    template <typename only>
    struct count<cons<only, nil>> { static constexpr unsigned the = 1; };

  } // namespace ll

  /**
   * namespace with types used to help constructing builtins
   */
  namespace bins_helpers {

    using namespace ll;

    template <char c> struct unk {
      typedef Val vat;
      inline static Type make(char const* fname) {
        std::string* vname = new std::string(1, c);
        vname->push_back('_');
        vname->append(fname);
        return Type(Ty::UNK, {.name=vname}, 0);
      }
    };
    struct num {
      typedef Num vat;
      inline static Type make(char const* fname) {
        return Type(Ty::NUM, {0}, 0);
      }
      struct ctor : Num {
        ctor(char const* fname)
          : Num()
        { }
        ctor(char const* fname, Type const& base_fty, Type const& ty)
          : Num()
        { }
      };
    };
    /*template <TyFlag is_inf> */struct str {
      typedef Str vat;
      inline static Type make(char const* fname) {
        return Type(Ty::STR, {0}, TyFlag::IS_FIN/*is_inf*/);
      }
      struct ctor : Str {
        ctor(char const* fname)
          : Str(TyFlag::IS_FIN) // ZZZ: from template param
        { }
        ctor(char const* fname, Type const& base_fty, Type const& ty)
          : Str(TyFlag::IS_FIN) // ZZZ: from base_fty.applied(ty)
        { }
      };
    };
    template <typename/*...*/ has/*, TyFlag is_inf*/> struct lst {
      typedef Lst vat;
      inline static Type make(char const* fname) {
        return Type(Ty::LST, {.box_has=types1(new Type(has::make(fname)/*...*/))}, TyFlag::IS_FIN/*is_inf*/);
      }
      struct ctor : Lst {
        ctor(char const* fname): Lst(make(fname)) { }
        ctor(char const* fname, Type const& base_fty, Type const& ty)
          : Lst(base_fty.applied(ty))
        { }
      };
    };
    template <typename from, typename to> struct fun {
      typedef Fun vat;
      inline static Type make(char const* fname) {
        return Type(Ty::FUN, {.box_pair={new Type(from::make(fname)), new Type(to::make(fname))}}, 0);
      }
      struct ctor : Fun {
        ctor(char const* fname): Fun(make(fname)) { }
        ctor(char const* fname, Type const& base_fty, Type const& ty)
          : Fun(base_fty.applied(ty))
        { }
      };
    };

    /*
     * vat: 'Val abstract type', so ex the class `Num` for Ty::NUM
     *
     * ->: Next
     * <-: Base
     *
     * Head <-> Body <..> Body <-> Tail
     *
     * Head: empty struct
     * Body and Tail: struct with base and arg fields
     *
     * Head and Body: extend from Fun
     * Tail: extends from the last vat
     *
     * `the`, for `_bin_be`, is the base class for the actual implementation of the Tail
     * (for example with "add", `the` extends `Num`) because it is available at any level
     * of the chain, that:
     * - `T::the::args` is the arity of the function (so 2 for "add")
     * - `T::the::Head` it the Head type (so `F` such as `F()(some_num)(other_num)`)
     * - `T::the::Base::Next` is the actual implementation (so `Add` for "add")
     * - and so `T::the::Base::Next::name` is the name constexpr ("add")
     *
     * `type` is expected to be a ll of unk/num/str/lst/fun
     * defined specialisations:
     *  - <Impl, cons<one, nil>>                                     // eg. "pi" (note that it's not `<Impl, one>`!)
     *  - <Impl, cons<last_to, cons<last_from, nil>>>                // defines `the`
     *  - <Impl, cons<to, cons<from, cons<from_again, from_more>>>>  // recursion for above
     *
     * when the last element of `type` is of unk, the implementation in the
     * tail struct is done by overriding a method called `Val* impl()`
     *
     * not use directly, see `builtin`
     */
    template <typename Implementation, typename reversed_type> struct _bin_be;

    template <typename Impl> struct _fake_bin_be { typedef Impl Next; };

    template <typename Impl, typename one>
    struct _bin_be<Impl, cons<one, nil>> {
      struct the : one::ctor {
        typedef Impl Head;
        typedef _fake_bin_be<Impl> Base;

        constexpr static unsigned args = 0;

        the()
          : one::ctor(Impl::name)
        { }

        Val* copy() const override; // copyOne
        void accept(Visitor& v) const override; // visitOne
      };
    };

    template <typename Impl, typename last_arg, char b>
    struct _bin_be<Impl, cons<fun<last_arg, unk<b>>, nil>> {
      struct the : fun<last_arg, unk<b>>::ctor {
        typedef Impl Head;
        typedef _fake_bin_be<Impl> Base;

        constexpr static unsigned args = 0;

        typedef typename last_arg::vat _LastArg;
        _LastArg* arg;

        the()
          : fun<last_arg, unk<b>>::ctor(Impl::name)
          , arg(nullptr)
        { }

        // to be overriden in `Implementation`
        virtual Val* impl() = 0;

        Val* operator()(Val* arg) override {
          this->arg = coerse<_LastArg>(arg);
          return impl();
        }
        Val* copy() const override; // copyOne2
        void accept(Visitor& v) const override; // visitOne2
      };
    };

    template <typename other> struct _fun_first_par_type { typedef void the; }; // ZZZ
    template <typename from, typename to> struct _fun_first_par_type<fun<from, to>> { typedef from the; };

    template <typename other> struct _fun_last_ret_type { typedef other the; };
    template <typename from, typename to> struct _fun_last_ret_type<fun<from, to>> { typedef typename _fun_last_ret_type<to>::the the; };

    template <typename other>
    struct _one_to_fun_last_ret_type { typedef void the; };
    template <typename from, typename to>
    struct _one_to_fun_last_ret_type<fun<from, to>> { typedef fun<from, to> the; };
    template <typename from, typename to_from, typename to_to>
    struct _one_to_fun_last_ret_type<fun<from, fun<to_from, to_to>>> { typedef typename _one_to_fun_last_ret_type<fun<to_from, to_to>>::the the; };

    template <typename R> struct _is_unk { constexpr static bool the = false; };
    template <char c> struct _is_unk<unk<c>> { constexpr static bool the = true; };

    /**
     * typedef Head Base; // when unary,
     * typedef typename Head::Next Base; // when binary,
     * typedef typename Head::Next::Next Base; // when ternary, ... (recursive)
     */
    template <typename T> struct _one_to_nextmost;
    template <typename N, typename ty>
    struct _one_to_nextmost<_bin_be<N, ty>> { typedef _bin_be<N, ty> the; };
    template <typename N, typename ty1, typename ty2>
    struct _one_to_nextmost<_bin_be<_bin_be<N, ty2>, ty1>> { typedef _bin_be<N, ty2> the; };
    template <typename N, typename ty1, typename ty2, typename ty3>
    struct _one_to_nextmost<_bin_be<_bin_be<_bin_be<N, ty3>, ty2>, ty1>> { typedef typename _one_to_nextmost<_bin_be<_bin_be<N, ty3>, ty2>>::the the; };

    template <typename NextT, typename last_to, typename last_from>
    struct _bin_be<NextT, cons<last_to, cons<last_from, nil>>> : fun<last_from, last_to>::ctor {
      typedef NextT Next; // type `this` instantiates in `op()`

      typedef typename last_from::vat _next_arg_ty;

      typedef typename _fun_last_ret_type<last_to>::the _ty_tail;
      typedef typename _one_to_fun_last_ret_type<fun<last_from, last_to>>::the _ty_one_to_tail;

      constexpr static bool _is_unk_tail = _is_unk<_ty_tail>::the;

      // is not used when `_is_unk_tail` (eg. 'X(a) -> a')
      struct _the_when_not_unk : std::conditional<
          _is_unk_tail,
          num, // (YYY: anything that has `::ctor` with a fitting `virtual ::accept`)
          _ty_tail
      >::type::ctor {
        typedef _bin_be Head; // type instanciated in looking up by name
        typedef typename _one_to_nextmost<Head>::the Base; // type which instantiates `this`

        typedef typename Base::_next_arg_ty Arg;

        constexpr static unsigned args = Base::args + 1;

        Base* base;
        Arg* arg;

        // this is the (inherited) ctor for the tail type
        _the_when_not_unk(Base* base, Arg* arg)
          : _ty_tail::ctor(Base::Next::name, base->type(), arg->type())
          , base(base)
          , arg(arg)
        { }

        Val* copy() const override; // copyTail1
      };

      // is used when `_is_unk_tail` (eg. 'X(a) -> a')
      struct _the_when_is_unk : _ty_one_to_tail::ctor {
        typedef _bin_be Head; // type instanciated in looking up by name
        typedef typename _one_to_nextmost<Head>::the Base; // type which instantiates `this`

        typedef typename Base::_next_arg_ty Arg;

        constexpr static unsigned args = Base::args + 1; // YYY: ?

        // tldr: inserts the arg as this own `arg`, and push back `base` once
        struct _ProxyBase : _ty_one_to_tail::ctor {
          typedef _the_when_is_unk the;
          constexpr static unsigned args = Base::args + 1;
          Base* base;
          Arg* arg;
          _ProxyBase(Base* base, Arg* arg)
            : _ty_one_to_tail::ctor(Base::Next::name, base->type(), arg->type()) // YYY: too hacky?
            , base(base)
            , arg(arg)
          { }
          Val* operator()(Val* arg) override { throw RuntimeError("operation not permited: operator() on proxy base"); } //{ return nullptr; } // YYY: still quite hacky?
          Val* copy() const override { throw RuntimeError("operation not permited: copy() on proxy base"); } //{ return new _ProxyBase(base, arg); } // YYY: need, more, hacky! (none's supposed to call that)
        } _base;
        _ProxyBase* base;
        typedef typename _fun_first_par_type<_ty_one_to_tail>::the::vat _LastArg;
        _LastArg* arg;

        // this is the (inherited) ctor for the tail type when ends on unk
        _the_when_is_unk(Base* base, Arg* arg)
          : _ty_one_to_tail::ctor(Base::Next::name, base->type(), arg->type())
          , _base(base, arg)
          , base(&_base)
          , arg(nullptr)
        { }

        // to be overriden in `Implementation`
        virtual Val* impl() = 0;

        Val* operator()(Val* arg) override {
          this->arg = coerse<_LastArg>(arg);
          return impl();
        }
        Val* copy() const override; // copyTail2
      };

      typedef typename std::conditional<
          _is_unk_tail,
          _the_when_is_unk,
          _the_when_not_unk
      >::type _the;
      struct the : _the {
        typedef typename _the::Head Head;
        typedef typename _the::Base Base;
        constexpr static unsigned args = _the::args;
        using _the::_the;
        void accept(Visitor& v) const override; // visitTail
      };

      constexpr static unsigned args = 0;

      // this is the ctor for the head type
      _bin_be()
        : fun<last_from, last_to>::ctor(the::Base::Next::name)
      { }

      Val* operator()(Val* arg) override { return new Next(this, coerse<_next_arg_ty>(arg)); }
      Val* copy() const override; // visitHead
      void accept(Visitor& v) const override; // visitHead
    };

    template <typename NextT, typename to, typename from, typename from_again, typename from_more>
    struct _bin_be<NextT, cons<to, cons<from, cons<from_again, from_more>>>> : fun<from, to>::ctor {
      typedef NextT Next; // type `this` instantiates in `op()`
      typedef _bin_be<_bin_be, cons<fun<from, to>, cons<from_again, from_more>>> Base; // type which instantiates `this`

      typedef typename from::vat _next_arg_ty;
      typedef typename Base::_next_arg_ty Arg;

      typedef typename Base::the the; // bubble `the` up

      constexpr static unsigned args = Base::args + 1;

      Base* base;
      Arg* arg;

      // this is the ctor for body types
      _bin_be(Base* base, Arg* arg)
        : fun<from, to>::ctor(the::Base::Next::name, base->type(), arg->type())
        , base(base)
        , arg(arg)
      { }

      Val* operator()(Val* arg) override { return new Next(this, coerse<_next_arg_ty>(arg)); }
      Val* copy() const override; // copyBody
      void accept(Visitor& v) const override; // visitBody
    };

    template <typename list>
    struct _make_fun_if_unk_tail { typedef list the; };
    template <char c, typename head, typename tail>
    struct _make_fun_if_unk_tail<cons<unk<c>, cons<head, tail>>> { typedef cons<fun<head, unk<c>>, tail> the; };

    template <typename Implementation, typename type>
    struct builtin {
      typedef typename _bin_be<Implementation, typename _make_fun_if_unk_tail<typename rev<type>::the>::the>::the the;
    };

  } // namespace bins_helpers

#define _BIN_num \
      double value() override;
#define _BIN_str \
      std::ostream& stream(std::ostream& out) override; \
      bool end() const override; \
      std::ostream& entire(std::ostream& out) override;
#define _BIN_lst \
      Val* operator*() override; \
      Lst& operator++() override; \
      bool end() const override;
#define _BIN_unk \
      Val* impl() override;

// used to remove the parenthesis from `__decl` and `__body`
#define __rem_par(...) __VA_ARGS__

// YYY: C-pp cannot do case operations in stringify,
// so to still have no conflict with the 65+ kw of C++,
// strategy is `lower_` (eg. `add_`, has `::name = "add"`)
// (couldn't get it with a constexpr tolower either...)
// SEE: https://stackoverflow.com/a/4225302
#define BIN(__ident, __decl, __docstr, __body) \
    struct __ident##_ \
        : bins_helpers::builtin<__ident##_, ll::cons_l<__rem_par __decl>::the>::the { \
      constexpr static char const* name = #__ident; \
      constexpr static char const* doc = __docstr; \
      using the::the; \
      __rem_par __body \
    }

// YYY: could not find a reliable way to infer base solely on _d
// because `macro(templt<a, b>)` has 2 arguments...
#define BIN_num(_i, _d, _s, _b) BIN(_i, _d, _s, (_BIN_num; __rem_par _b))
#define BIN_str(_i, _d, _s, _b) BIN(_i, _d, _s, (_BIN_str; __rem_par _b))
#define BIN_lst(_i, _d, _s, _b) BIN(_i, _d, _s, (_BIN_lst; __rem_par _b))
#define BIN_unk(_i, _d, _s, _b) BIN(_i, _d, _s, (_BIN_unk; __rem_par _b))

  /**
   * namespace containing the actual builtins
   */
  namespace bins {

    using bins_helpers::unk;
    using bins_helpers::num;
    using bins_helpers::str;
    using bins_helpers::lst;
    using bins_helpers::fun;

    BIN_num(abs, (num, num),
      "return the absolute value of a number", ());

    BIN_num(add, (num, num, num),
      "add two numbers", ());

    BIN_lst(conjunction, (lst<unk<'a'>>, lst<unk<'a'>>, lst<unk<'a'>>),
      "logical conjunction between two lists treated as sets; it is right-lazy and the list order is taken from the right argument (for now items are expected to be strings, until arbitraty value comparison)", (
      bool did_once = false;
      std::unordered_set<std::string> inleft;
      void once();
    ));

    BIN_unk(const, (unk<'a'>, unk<'b'>, unk<'a'>),
      "always evaluate to its first argument, ignoring its second argument", ());

    BIN_num(div, (num, num, num),
      "divide the first number by the second number", ());

    BIN_lst(drop, (num, lst<unk<'a'>>, lst<unk<'a'>>),
      "return the suffix past a given count, or the empty list if it is shorter", (
      mutable bool done = false;
    ));

    BIN_lst(dropwhile, (fun<unk<'a'>, num>, lst<unk<'a'>>, lst<unk<'a'>>),
      "return the suffix remaining from the first element not verifying the predicate onward", (
      mutable bool done = false;
    ));

    BIN_lst(filter, (fun<unk<'a'>, num>, lst<unk<'a'>>, lst<unk<'a'>>),
      "return the list of elements which satisfy the predicate", (
      Val* curr = nullptr;
    ));

    BIN_unk(flip, (fun<unk<'a'>, fun<unk<'b'>, unk<'c'>>>, unk<'b'>, unk<'a'>, unk<'c'>),
      "flip the two parameters by passing the first given after the second one", ());

    BIN_unk(id, (unk<'a'>, unk<'a'>),
      "the identity function, returns its input", ());

    BIN_unk(if, (fun<unk<'a'>, num>, unk<'b'>, unk<'b'>, unk<'a'>, unk<'b'>),
      "take a condition, a consequence and an alternative; return consequence if the argument verifies the condition, alternative otherwise", ());

    BIN_lst(iterate, (fun<unk<'a'>, unk<'a'>>, unk<'a'>, lst<unk<'a'>>),
      "return an infinite list of repeated applications of the function to the input", (
      Val* curr = nullptr;
    ));

    BIN_str(join, (str, lst<str>, str),
      "join a list of string with a separator between entries", (
      std::string ssep;
      bool beginning = true;
    ));

    BIN_lst(map, (fun<unk<'a'>, unk<'b'>>, lst<unk<'a'>>, lst<unk<'b'>>),
      "make a new list by applying an unary operation to each value from a list", ());

    BIN_num(mul, (num, num, num),
      "multiply tow numbers", ());

    BIN_str(nl, (str, str),
      "append a new line to a string", (
      bool done = false;
    ));

    BIN_num(pi, (num),
      "pi, what did you expect", ());

    BIN_lst(repeat, (unk<'a'>, lst<unk<'a'>>),
      "repeat an infinite amount of copies of the same value", ());

    BIN_lst(replicate, (num, unk<'a'>, lst<unk<'a'>>),
      "replicate a finite amount of copies of the same value", (
      size_t did = 0;
    ));

    BIN_lst(reverse, (lst<unk<'a'>>, lst<unk<'a'>>),
      "reverse the order of the elements in the list", (
      std::vector<Val*> cache;
      bool did_once = false;
      size_t curr;
      void once();
    ));

    BIN_lst(singleton, (unk<'a'>, lst<unk<'a'>>),
      "make a list of a single item", (
      bool done = false;
    ));

    BIN_lst(split, (str, str, lst<str>),
      "break a string into pieces separated by the argument, consuming the delimiter; note that an empty delimiter does not split the input on every character, but rather is equivalent to `const [repeat ::]`", ( // YYY: .. i think..?
      bool did_once = false;
      std::string ssep;
      std::ostringstream acc = std::ostringstream(std::ios_base::ate);
      std::string curr;
      bool at_end = false;
      bool at_past_end = false;
      // std::vector<Val*> cache;
      bool init = false;
      void once();
      void next();
    ));

    BIN_num(startswith, (str, str, num),
      "true if the string starts with the given prefix", (
      bool done = false, does;
    ));

    BIN_num(sub, (num, num, num),
      "substract the second number from the first", ());

    BIN_lst(take, (num, lst<unk<'a'>>, lst<unk<'a'>>),
      "return the prefix of a given length, or the entire list if it is shorter", (
      size_t did;
    ));

    BIN_lst(takewhile, (fun<unk<'a'>, num>, lst<unk<'a'>>, lst<unk<'a'>>),
      "return the longest prefix of elements statisfying the predicate", ());

    BIN_num(tonum, (str, num),
      "convert a string into number", (
      double r;
      bool done = false;
    ));

    BIN_str(tostr, (num, str),
      "convert a number into string", (
      bool read = false;
    ));

    BIN_unk(uncurry, (fun<unk<'a'>, fun<unk<'b'>, unk<'c'>>>, lst<unk<'w'/* TODO: tuple */>>, unk<'c'>),
      "convert a curried function to a function on pairs", ());

    BIN_lst(zipwith, (fun<unk<'a'>, fun<unk<'b'>, unk<'c'>>>, lst<unk<'a'>>, lst<unk<'b'>>, lst<unk<'c'>>),
      "make a new list by applying an binary operation to each corresponding value from each lists; stops when either list ends", (
      Val* curr = nullptr;
    ));

  } // namespace bins

  /**
   * namespace with the types `bins` and `bins_all` which lists
   *  - every builtin `Tail` types in alphabetical order
   *  - every builtin types (inc. intermediary) (not in any order)
   */
  namespace bins_ll {

    using namespace ll;

    /**
     * make a list from a list, by also unpacking every `::Base`s
     */
    template <typename TailL>
    struct _make_bins_all {
      typedef cons<
        typename TailL::car,
        typename _make_bins_all<cons<typename TailL::car::Base, typename TailL::cdr>>::the
      > the;
    };
    template <typename Next, typename last_to, typename last_from, typename TailL>
    struct _make_bins_all<cons<bins_helpers::_bin_be<Next, cons<last_to, cons<last_from, nil>>>, TailL>> {
      typedef cons<
        bins_helpers::_bin_be<Next, cons<last_to, cons<last_from, nil>>>,
        typename _make_bins_all<TailL>::the
      > the;
    };
    template <>
    struct _make_bins_all<nil> {
      typedef nil the;
    };
    template <typename Impl, typename cdr>
    struct _make_bins_all<cons<bins_helpers::_fake_bin_be<Impl>, cdr>> {
      typedef typename _make_bins_all<cdr>::the the;
    };

    using namespace bins;

#ifdef BINS_MIN
    // YYY: these are used in parsing..
    typedef cons_l
      < add_
      , div_
      , flip_
      , mul_
      , sub_
      >::the bins; //bins_min; // YYY: could have these only here, but would need to merge with below while keeping sorted (not strictly necessary, but convenient)
#else
    // XXX: still would love if this list could be built automatically
    typedef cons_l
      < abs_
      , add_
      , conjunction_
      , const_
      , div_
      , drop_
      , dropwhile_
      , flip_
      , filter_
      // , head_
      , id_
      , if_
      // , init_
      , iterate_
      , join_
      // , last_
      , map_
      , mul_
      , nl_
      , pi_
      , repeat_
      , replicate_
      , reverse_
      , singleton_
      , split_
      , startswith_
      , sub_
      // , tail_
      , take_
      , takewhile_
      , tonum_
      , tostr_
      , uncurry_
      , zipwith_
      >::the bins;
#endif
    typedef _make_bins_all<bins>::the bins_all;

  } // namespace bins_ll

} // namespace sel

#endif // SEL_BUILTINS_HPP

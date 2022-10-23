#include "sel/utils.hpp"
#include "sel/types.hpp"
#include "sel/errors.hpp"

namespace sel {

  // internal
  std::ostream& operator<<(std::ostream& out, Ty ty) {
    switch (ty) {
      case Ty::UNK: out << "UNK"; break;
      case Ty::NUM: out << "NUM"; break;
      case Ty::STR: out << "STR"; break;
      case Ty::LST: out << "LST"; break;
      case Ty::FUN: out << "FUN"; break;
    }
    return out;
  }

  // internal
  std::ostream& operator<<(std::ostream& out, TyFlag tf) {
    out << (TyFlag::IS_INF & tf ? "IS_INF" : "IS_FIN");
    if (TyFlag::IS_TPL & tf) out << "| IS_TPL";
    return out;
  }

  Type::Type() { TRACE(Type, raw(this)); }

  Type::Type(Ty base, Type::P p, uint8_t flags)
    : base(base)
    , p(p)
    , flags(flags)
  { TRACE(Type, raw(this)<<"; "<<base); }

  Type::Type(Type const& ty) {
    TRACE(Type&, raw(this)<<"; "<<ty);
    switch (base = ty.base) {
      case Ty::UNK:
        p.name = new std::string(*ty.p.name);
        break;

      case Ty::LST:
        p.box_has = new std::vector<Type*>();
        p.box_has->reserve(ty.p.box_has->size());
        for (auto const& it : *ty.p.box_has)
          p.box_has->push_back(new Type(*it));
        break;

      case Ty::FUN:
        p.box_pair[0] = new Type(*ty.p.box_pair[0]);
        p.box_pair[1] = new Type(*ty.p.box_pair[1]);
        break;

      default: ;
    }
    flags = ty.flags;
  }

  Type::Type(Type&& ty) noexcept {
    TRACE(Type&&, raw(this)<<"; "<<ty);
    switch (base = ty.base) {
      case Ty::UNK:
        p.name = ty.p.name;
        ty.p.name = nullptr;
        break;

      case Ty::LST:
        p.box_has = ty.p.box_has;
        ty.p.box_has = nullptr;
        break;

      case Ty::FUN:
        p.box_pair[0] = ty.p.box_pair[0];
        p.box_pair[1] = ty.p.box_pair[1];
        ty.p.box_pair[0] = nullptr;
        ty.p.box_pair[1] = nullptr;
        break;

      default: ;
    }
    flags = ty.flags;
  }

  Type::~Type() {
    TRACE(~Type, raw(this)<<"; "<<base)
    switch (base) {
      case Ty::UNK:
        delete p.name;
        break;

      case Ty::LST:
        delete p.box_has;

      case Ty::FUN:
        delete p.box_pair[0];
        delete p.box_pair[1];
        break;

      default: ;
    }
  }

  Type unkType(std::string* name) {
    TRACE(unkType, *name<<"*"<<raw(name));
    return Type(Ty::UNK, {.name=name}, 0);
  }

  Type numType() {
    TRACE(numType, "");
    return Type(Ty::NUM, {0}, 0);
  }

  Type strType(TyFlag is_inf) {
    TRACE(strType, is_inf);
    return Type(Ty::STR, {0}, static_cast<uint8_t>(is_inf));
  }

  Type lstType(Type* has, TyFlag is_inf) {
    TRACE(lstType, *has<<"*"<<raw(has)<<", "<<is_inf);
    auto v = new std::vector<Type*>();
    v->push_back(has);
    return Type(Ty::LST, {.box_has=v}, static_cast<uint8_t>(is_inf));
  }

  Type lstType(std::vector<Type*>* has, TyFlag is_inf, TyFlag is_tpl) {
    TRACE(lstType, "{"<<has->size()<<" types}*"<<raw(has)<<", "<<(TyFlag)(is_inf|is_tpl));
    return Type(Ty::LST, {.box_has=has}, is_inf | is_tpl);
  }

  Type funType(Type* from, Type* to) {
    TRACE(funType, *from<<"*"<<raw(from)<<", "<<*to<<"*"<<raw(to));
    return Type(Ty::FUN, {.box_pair={from,to}}, 0);
  }

  // internal
  enum class TyTokenType {
    UNKNOWN,
    TY_EQ,
    P_OPEN, P_CLOSE,
    B_OPEN, B_CLOSE,
    ARROW,
    COMMA,
    STAR,
    NAME, // unknown named type (first letter lower case)
    TY_NAME, // known type (first letter upper case)
  };
  // internal
  struct TyToken {
    TyTokenType type;
    std::string text;
  };

  // internal
  std::ostream& operator<<(std::ostream& out, TyToken const& tt) {
    out << "TyToken { .type=";
    switch (tt.type) {
      case TyTokenType::UNKNOWN: out << "UNKNOWN"; break;
      case TyTokenType::TY_EQ:   out << "TY_EQ";   break;
      case TyTokenType::P_OPEN:  out << "P_OPEN";  break;
      case TyTokenType::P_CLOSE: out << "P_CLOSE"; break;
      case TyTokenType::B_OPEN:  out << "B_OPEN";  break;
      case TyTokenType::B_CLOSE: out << "B_CLOSE"; break;
      case TyTokenType::ARROW:   out << "ARROW";   break;
      case TyTokenType::COMMA:   out << "COMMA";   break;
      case TyTokenType::STAR:    out << "STAR";    break;
      case TyTokenType::NAME:    out << "NAME";    break;
      case TyTokenType::TY_NAME: out << "TY_NAME"; break;
    }
    return out << ", .text=\"" << tt.text << "\" }";
  }

  // internal
  std::istream& operator>>(std::istream& in, TyToken& tt) {
    char c = in.get();
    char cc = 0;
    if (in.eof()) return in;
    switch (c) {
      case ':':
        if (':' != (cc = in.get())) goto unknown_token_push2;
        tt.type = TyTokenType::TY_EQ;
        tt.text = "::";
        break;

      case '(':
        tt.type = TyTokenType::P_OPEN;
        tt.text = "(";
        break;

      case ')':
        tt.type = TyTokenType::P_CLOSE;
        tt.text = ")";
        break;

      case '[':
        tt.type = TyTokenType::B_OPEN;
        tt.text = "[";
        break;

      case ']':
        tt.type = TyTokenType::B_CLOSE;
        tt.text = "]";
        break;

      case '-':
        if ('>' != (cc = in.get())) goto unknown_token_push2;
        tt.type = TyTokenType::ARROW;
        tt.text = "->";
        break;

      case ',':
        tt.type = TyTokenType::COMMA;
        tt.text = ",";
        break;

      case '*':
        tt.type = TyTokenType::STAR;
        tt.text = "*";
        break;

      default:
        if (isspace(c)) break;
        if ((c < 'A' || 'Z' < c) && (c < 'a' || 'z' < c)) goto unknown_token_push1;

        tt.type = islower(c)
          ? TyTokenType::NAME
          : TyTokenType::TY_NAME;
        tt.text = c;
        while ('a' <= in.peek() && in.peek() <= 'z') // no upper cases? should not, only first character
          tt.text.push_back(in.get());
    }

    if (false) {
unknown_token_push2:
      tt.text = cc;
unknown_token_push1:
      if (cc) tt.text+= c;
      else    tt.text = c;
      tt.type = TyTokenType::UNKNOWN;
    }

    while (!in.eof() && isspace(in.peek())) in.get();
    return in;
  }

  // internal
  void parseTypeImpl(TyToken const& first, std::istream_iterator<TyToken>& tts, Type& res) {
    static auto const eos = std::istream_iterator<TyToken>();
    TyToken new_first;

    // type ::= name
    //        | ty_name
    //        | (type)
    //        | [type]
    //        | type -> type
    //        | (type, type)
    //        | type*
    switch (first.type) {
      case TyTokenType::UNKNOWN:
        throw ParseError("type expression",
          ( std::string("got unknown tokens")
          + " '" + first.text + " " + tts->text + " ...'"
          ), "- what -");

      case TyTokenType::NAME:
        res.base = Ty::UNK;
        res.p.name = new std::string(first.text);
        break;

      case TyTokenType::TY_NAME:
        if ("Num" == first.text)
          res.base = Ty::NUM;
        else if ("Str" == first.text)
          res.base = Ty::STR;
        else
          throw NameError(first.text, "- what -");
        break;

      case TyTokenType::P_OPEN:
        if (eos == tts) throw EOSError("type expression after '('", "- what -");
        new_first = *tts;
        parseTypeImpl(new_first, ++tts, res);
        if (eos == tts) throw EOSError("token ',' or matching token ')'", "- what -");
        //        | (type, type)
        if (TyTokenType::COMMA == tts->type) {
          auto v = new std::vector<Type*>();
          v->push_back(new Type(std::move(res)));
          res.p.box_has = v;
          res.base = Ty::LST;
          res.flags = TyFlag::IS_TPL;
          do {
            new_first = *++tts;
            Type* last = new Type();
            parseTypeImpl(new_first, ++tts, *last);
            res.p.box_has->push_back(last);
          } while (TyTokenType::COMMA == tts->type);
        }
        if (eos == tts) throw EOSError("matching token ')'", "- what -");
        if (TyTokenType::P_CLOSE != tts->type)
          throw ParseError("matching token ')'",
            ( std::string("got unexpected tokens")
            + " '" + tts++->text + " " + tts->text + " ...'"
            ), "- what -");
        tts++;
        break;

      case TyTokenType::B_OPEN:
        if (eos == tts) throw EOSError("type expression after '['", "- what -");
        res.p.box_has = new std::vector<Type*>(1);
        new_first = *tts;
        parseTypeImpl(new_first, ++tts, *(*res.p.box_has)[0]);
        // TODO: list of repeating pattern: '[' <type> {',' <type>} ']'
        if (eos == tts) throw EOSError("matching token ']'", "- what -");
        if (TyTokenType::B_CLOSE != tts->type)
          throw ParseError("matching token ')'",
            ( std::string("got unexpected tokens")
            + " '" + tts++->text + " " + tts->text + " ...'"
            ), "- what -");
        res.base = Ty::LST;
        res.flags = 0;
        tts++;
        break;

      default:
        throw ParseError("type expression",
          ( std::string("got unexpected tokens")
          + " '" + first.text + " " + tts->text + " ...'"
          ), "- what -");
    }

    if (eos == tts) return;
    //        | type*
    if (TyTokenType::STAR == tts->type) {
      res.flags|= TyFlag::IS_INF;
      tts++;
    }

    if (eos == tts) return;
    //        | type -> type
    if (TyTokenType::ARROW == tts->type) {
      res.p.box_pair[0] = new Type(std::move(res));
      new_first = *++tts;
      parseTypeImpl(new_first, ++tts, *(res.p.box_pair[1] = new Type()));
      res.base = Ty::FUN;
      res.flags = 0;
    }
  }

  void parseType(std::istream& in, std::string* named, Type& res) {
    static auto const eos = std::istream_iterator<TyToken>();
    auto lexer = std::istream_iterator<TyToken>(in);

    // first check for "<name> '::'"
    // note that the whole block needs to check the 2
    // first tokens, and as such need to pop at least one;
    // `first` is handed over to the `parseTypeImple`
    // to compensate
    TyToken first = *lexer;
    if (TyTokenType::NAME == first.type && eos != lexer) {
      TyToken second = *++lexer;
      if (TyTokenType::TY_EQ == second.type) {
        lexer++; // drop '::'
        if (eos == lexer) throw std::string("end of token stream");
        if (named) named->assign(first.text);
        first = *lexer++; // pop `first` (will forward to impl)
      } else if (named) named->assign("");
    } else {
      if (named) named->assign("");
      lexer++; // pop `first` (will forward to impl)
    }

    parseTypeImpl(first, lexer, res);
  }

  /**
   * Two types compare equal (are compatible) if any:
   * - any UNK
   * - both STR or both NUM
   * - both LST and recursive on has
   * - both FUN and recursive on fst and snd
   *
   * As such, this does not look at the flags (eg. IS_INF).
   */
  bool Type::operator==(Type const& other) const {
    if (Ty::UNK == base || Ty::UNK == other.base)
      return true;

    if (base != other.base)
      return false;

    if ((TyFlag::IS_INF & flags) != (TyFlag::IS_INF & other.flags))
      return false;

    switch (base) {
      case Ty::NUM:
      case Ty::STR:
        return true;

      case Ty::LST:
        // return !p.box_has || !other.p.box_has || *p.box_has == *other.p.box_has;
        return true; // TODO: if both tuples and box_has non nullptr, check types one by one [?]

      case Ty::FUN:
        return *p.box_pair[0] == *other.p.box_pair[0]
            && *p.box_pair[1] == *other.p.box_pair[1];

      // unreachable
      default: return false;
    }
  }
  bool Type::operator!=(Type const& other) const {
    return !(*this == other);
  }

  std::ostream& operator<<(std::ostream& out, Type const& ty) {
    switch (ty.base) {
      case Ty::UNK:
        out << *ty.p.name;
        break;

      case Ty::NUM:
        out << "Num";
        break;

      case Ty::STR:
        out << "Str";
        break;

      case Ty::LST:
        {
          char const* crap = "[";
          for (auto const& it : *ty.p.box_has) {
            out << crap << *it;
            crap = ", ";
          }
          out << "]";
        }
        break;

      case Ty::FUN:
        if (Ty::FUN == ty.p.box_pair[0]->base)
          out << "(" << *ty.p.box_pair[0] << ") -> " << *ty.p.box_pair[1];
        else
          out << *ty.p.box_pair[0] << " -> " << *ty.p.box_pair[1];
        break;
    }

    if (TyFlag::IS_INF & ty.flags) out << "*";
    return out;
  }

  std::istream& operator>>(std::istream& in, Type& res) {
    parseType(in, nullptr, res);
    return in;
  }

} // namespace sel

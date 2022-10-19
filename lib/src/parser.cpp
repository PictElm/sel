#include <ostream>
#include <istream>
#include <string>

#include "sel/parser.hpp"
#include "sel/visitors.hpp"

namespace sel {

  void NumLiteral::accept(Visitor& v) const { v.visitNumLiteral(type(), n); }

  // internal
  struct Token {
    size_t loc;
    enum class Type {
      NAME,
      LIT_NUM,
      LIT_STR,
      LIT_LST_OPEN,
      LIT_LST_CLOSE,
      UN_OP,
      BIN_OP,
      SUB_OPEN,
      SUB_CLOSE,
      COMMA,
    } type;
    union {
      std::string name;
      double num;
      std::string str;
      char chr;
    } as;
  };

  // internal
  std::istream& operator>>(std::istream& in, Token& t) {
    char c = in.get();
    switch (c) {
      case ',': t.type = Token::Type::COMMA; t.as.chr = c; break;

      case '[': t.type = Token::Type::SUB_OPEN; t.as.chr = c; break;
      case ']': t.type = Token::Type::SUB_CLOSE; t.as.chr = c; break;

      case '{': t.type = Token::Type::LIT_LST_OPEN; t.as.chr = c; break;
      case '}': t.type = Token::Type::LIT_LST_CLOSE; t.as.chr = c; break;

      case '@':
      case '%':
        t.type = Token::Type::UN_OP;
        t.as.chr = c;
        break;

      case '+':
      case '-':
      case '.':
      case '/':
        t.type = Token::Type::BIN_OP;
        t.as.chr = c;
        break;

      case ':':
        t.type = Token::Type::LIT_STR;
        throw "TODO: t.as.str";
        break;

      // case trait::eof():

      default:
        if ('0' <= c && c <= '9') {
          t.type = Token::Type::LIT_NUM;
          throw "TODO: t.as.num";
          break;
        }

        if ('a' <= c && c <= 'z') {
          t.type = Token::Type::LIT_STR;
          throw "TODO: t.as.str";
          break;
        }
    }
    return in;
  }

  std::ostream& operator<<(std::ostream& out, Application const& app) {
    throw "TODO: operator<< for Application\n";
    return out;
  }

  std::istream& operator>>(std::istream& in, Application& app) {
    throw "TODO: operator>> for Application\n";
    return in;
  }

} // namespace sel

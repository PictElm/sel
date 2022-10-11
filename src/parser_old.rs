use std::{iter::Peekable, str::Chars, vec};

use crate::{
    engine::{Function, Value},
    runtime::{Application, Environ},
    prelude::PreludeLookup,
};

#[derive(Debug, Clone, Copy)]
pub enum Unop {
    Array,
    Flip,
}

#[derive(Debug, Clone, Copy)]
pub enum Binop {
    Addition,
    Substraction,
    Multiplication,
    Division,
    // Range,
}

#[derive(Debug, Clone, Copy)]
pub enum Operator {
    Unary(Unop),
    Binary(Binop),
}

#[derive(Debug, Clone)]
pub enum Token {
    Name(String),
    Literal(Value),
    Operator(Operator),
    // SubScript(Vec<Token>),
    SubscriptBegin,
    SubscriptEnd,
    Composition,
}

pub struct Lexer<'a>(Peekable<Chars<'a>>);
fn lex_string<'a>(script: &'a String) -> Lexer<'a> {
    Lexer(script.chars().peekable())
}

impl<'a> Iterator for Lexer<'a> {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
        match self.0.peek() {
            None => None,

            Some(c) if c.is_ascii_whitespace() => {
                // self.0.next();
                while !self.0.next_if(|cc| cc.is_ascii_whitespace()).is_none() {}
                self.next()
            }

            Some('#') => {
                self.0.by_ref().skip_while(|cc| '\n' != *cc).next();
                self.next()
            }

            Some(c) if c.is_ascii_digit() => {
                let mut acc: String = "".to_string();
                loop {
                    match self.0.next_if(|cc| cc.is_ascii_digit()) {
                        Some(cc) => {
                            acc.push(cc);
                        }
                        None => {
                            break;
                        }
                    }
                }
                Some(Token::Literal(Value::Num(acc.parse().unwrap())))
            }

            Some(c) if c.is_ascii_lowercase() => {
                let mut acc: String = "".to_string();
                loop {
                    match self.0.next_if(|cc| cc.is_ascii_lowercase()) {
                        Some(cc) => {
                            acc.push(cc);
                        }
                        None => {
                            break;
                        }
                    }
                }
                Some(Token::Name(acc))
            }

            Some('{') => {
                let mut lvl: u32 = 0;
                let acc = self
                    .0
                    .by_ref()
                    .take_while(|cc| match cc {
                        '{' => {
                            lvl += 1;
                            true
                        }
                        '}' => {
                            assert!(0 < lvl, "Unbalanced string literal: missing opening '{{'");
                            lvl -= 1;
                            0 < lvl
                        }
                        _ => true,
                    })
                    .skip(1)
                    .collect();
                assert!(0 == lvl, "Unbalanced string literal: missing closing '}}'");
                Some(Token::Literal(Value::Str(acc)))
            }

            Some(c) => {
                let r = match c {
                    '[' => Some(Token::SubscriptBegin),
                    ']' => Some(Token::SubscriptEnd),
                    ',' => Some(Token::Composition),
                    '+' => Some(Token::Operator(Operator::Binary(Binop::Addition))),
                    '-' => Some(Token::Operator(Operator::Binary(Binop::Substraction))),
                    '.' => Some(Token::Operator(Operator::Binary(Binop::Multiplication))),
                    '/' => Some(Token::Operator(Operator::Binary(Binop::Division))),
                    // ':' => Some(Token::Operator(Operator::Binary(Binop::Range))),
                    '@' => Some(Token::Operator(Operator::Unary(Unop::Array))),
                    '%' => Some(Token::Operator(Operator::Unary(Unop::Flip))),
                    '}' => panic!("Unbalanced string literal: missing opening '{{'"),
                    c => todo!("Unhandled character '{c}'"),
                };
                self.0.next();
                r
            }
        } // match peek
    } // fn next
} // impl Iterator for Lexer

pub trait Lex {
    // YYY: may seal it, as well as the Lexer struct
    type TokenIter: Iterator<Item = Token>;
    fn lex(self) -> Self::TokenIter;
}

impl<'a> Lex for &'a String {
    type TokenIter = Lexer<'a>;
    fn lex(self) -> Self::TokenIter {
        lex_string(self)
    }
}
impl Lex for Vec<Token> {
    type TokenIter = vec::IntoIter<Token>;
    fn lex(self) -> Self::TokenIter {
        self.into_iter()
    }
}

// #[derive(Clone)]
pub struct Parser<T>(Peekable<T::TokenIter>)
where
    T: Lex;

pub fn parse_string<'a>(script: &'a String) -> Parser<&'a String> {
    Parser(script.lex().peekable())
}
fn parse_vec(tokens: Vec<Token>) -> Parser<Vec<Token>> {
    Parser(tokens.lex().peekable())
}

impl<'a, T> Parser<T>
where
    T: Lex + 'a,
{
    pub fn result(self, env: Environ) -> (Environ, Application) {
        let (env, app) = self.collect_values(env);
        (env, app.into_iter().collect())
    }

    fn as_value(self, env: Environ) -> (Environ, Value) {
        let (env, fs) = self.collect_values(env);

        if 1 == fs.len() {
            return (env, fs.into_iter().next().unwrap());
        }

        let firstin = fs
            .first()
            .map(|f| match f {
                Value::Fun(f) => f.maps.0.clone(),
                _ => panic!("type error"),
            })
            .unwrap();
        let lastout = fs
            .last()
            .map(|f| match f {
                Value::Fun(f) => f.maps.1.clone(),
                _ => panic!("type error"),
            })
            .unwrap();

        (env, Value::Fun(Function {
            name: "(script)".to_string(),
            maps: (firstin, lastout),
            args: fs,
            func: |env, this| {
                let arg = this.args.last().unwrap().clone();
                this.args[..this.args.len() - 1]
                    .iter()
                    .fold((env, arg), |r, f| Function::from(f.clone()).apply(r))
            },
        }))
    } // fn result

    /// atom ::= literal
    ///        | name
    ///        | unop (binop | atom)
    ///        | binop atom
    ///        | subscript
    /// subscript ::= '[' [atom binop atom | _elements1] ']'
    fn next_atom(&mut self, env: Environ) -> Option<(Environ, Value)> {
        todo!()
        // match self.0.next() {
        //     None | Some(Token::Composition) => None,

        //     Some(Token::Literal(value)) => Some((env, value.clone())),

        //     Some(Token::Name(name)) => Some((
        //         env,
        //         env.prelude
        //             .lookup_name(name.to_string())
        //             .expect(&format!("Unknown name '{name}'")),
        //     )),

        //     Some(Token::Operator(Operator::Unary(un))) => {
        //         let unf = env.prelude.lookup_unary(un);
        //         let (opr, is_opr_bin) =
        //             if let Some(Token::Operator(Operator::Binary(bin))) = self.0.peek() {
        //                 let r = env.prelude.lookup_binary(*bin, env);
        //                 self.0.next();
        //                 (r, true)
        //             } else {
        //                 (self.next_atom(env).expect(&format!("Missing argument for unary {un:?}")), false)
        //             };
        //         let r = unf.apply(opr);
        //         Some(
        //             if is_opr_bin {
        //                 let f: Function = r.1.into();
        //                 f.apply(self.next_atom(env).expect(&format!("Missing argument for binary after unary {un:?}")))
        //             } else { r }
        //         )
        //     }

        //     Some(Token::Operator(Operator::Binary(bin))) => Some({
        //         let (env, val) = env.prelude.lookup_binary(bin, env);
        //         let binf: Function = val.into();
        //         binf.apply(self.next_atom(env).expect(&format!("Missing argument for binary {bin:?}")))
        //     }),

        //     Some(Token::SubscriptBegin) => {
        //         let mut lvl: u32 = 1;
        //         let tokens = self
        //             .0
        //             .by_ref()
        //             .take_while(|tt| match tt {
        //                 Token::SubscriptBegin => {
        //                     lvl += 1;
        //                     true
        //                 }
        //                 Token::SubscriptEnd => {
        //                     assert!(
        //                         0 < lvl,
        //                         "Unbalanced subscript expression: missing opening '['"
        //                     );
        //                     lvl -= 1;
        //                     0 < lvl
        //                 }
        //                 _ => true,
        //             })
        //             .collect();
        //         assert!(
        //             0 == lvl,
        //             "Unbalanced subscript expression: missing closing ']'"
        //         );
        //         Some(parse_vec(tokens).as_value(env))
        //     }
        //     Some(Token::SubscriptEnd) => {
        //         panic!("Unbalanced subscript expression: missing opening '['")
        //     }
        // } // match lexer next
    } // next_atom

    /// script ::= _elements1
    /// _elements1 ::= element {',' element} [',']
    /// element ::= atom {atom}
    fn next_value(mut self, env: Environ) -> Option<(Environ, Value)> {
        match self.next_atom(env) {
            None => None,

            Some(mut pair) => {
                loop {
                    match self.next_atom(pair.0) {
                        None => { break; }
                        Some(new_pair) => { pair = Function::from(new_pair.1).apply(new_pair); }
                    }
                }
                Some(pair)
            }
        } // match next_atom
    } // next_value

    fn collect_values(self, env: Environ) -> (Environ, Vec<Value>) {
        todo!()
    }
} // impl Parser

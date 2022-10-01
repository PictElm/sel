use std::{env, io::stdin};

use crate::{parser::parse_string, engine::{Value, Apply}};

mod parser;
mod engine;
mod prelude;

fn main() {
    let app = parse_string(&env::args().skip(1).collect()).result();

    for line in stdin().lines() {
        match line {
            Ok(it) => {
                match app.clone().apply(Value::Str(it)) {
                    Value::Str(c) => println!("{}", c),
                    other => println!("-> {:?}", other),
                }
            },
            Err(e) => panic!("{}", e),
        }
    }
}

use crate::{
    engine::{Function, Value},
    prelude::Prelude,
};

// YYY: more private fields
pub struct Application<'a> {
    pub funcs: Vec<Function>,
    pub env: &'a Environ<'a>,
}

// YYY: more private fields
pub struct Environ<'a> {
    pub prelude: &'a Prelude,
    // pub app: Box<Application<'a>>,
}

impl<'a> Environ<'a> {
    pub fn new(prelude: &'a Prelude) -> Self {
        Self {
            prelude,
            // app: todo!(),
        }
    }
}

impl<'a> Application<'a> {
    pub fn new<T: Iterator<Item = Value>>(env: &'a Environ, funcs_iter: T) -> Self {
        Application {
            funcs: funcs_iter
                .into_iter()
                .map(|v| match v {
                    Value::Fun(f) => f,
                    other => {
                        println!("trying to build an application from functions,");
                        println!("but got: {other}");
                        panic!("type error");
                    }
                })
                .collect(),
            env,
        }
    }

    pub fn apply(&self, line: String) -> String {
        self.funcs
            .iter()
            .fold(Value::Str(line), |acc, cur| cur.clone().apply(acc))
            .as_text()
    }
}

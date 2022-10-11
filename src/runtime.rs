use crate::{
    engine::{Function, Value},
    prelude::Prelude,
};

// YYY: more private fields
pub struct Application {
    pub funcs: Vec<Function>,
}

// YYY: more private fields
#[derive(Clone)]
pub struct Environ<'a> {
    pub prelude: &'a Prelude,
    // pub app: Box<Application<'a>>,
}

impl<'a> Environ<'a> {
    pub fn new(prelude: &'a Prelude) -> Self {
        Self { prelude }
    }
}

impl FromIterator<Value> for Application {
    fn from_iter<T: IntoIterator<Item = Value>>(iter: T) -> Self {
        Application {
            funcs: iter
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
        }
    }
}

impl Application {
    pub fn apply(&self, env: Environ, line: String) -> (Environ, String) {
        let (e, r) = self.funcs
            .iter()
            .fold((env, Value::Str(line)), |acc, cur| cur.clone().apply(acc));
        (e, r.as_text())
    }
}

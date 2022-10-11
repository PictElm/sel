use crate::runtime::{Application, Environ};

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

pub fn parse_string(script: String, env: Environ) -> (Environ, Application) {
    todo!()
}

// trait Value {
//     type Valued;
//     fn eval(self, env: Environ) -> (Environ, ());
//     // fn apply(self, arg: impl Value) -> Self::Value;
//     // fn coerse<T: Value>(self) -> <T>::Value;
// }

type Number = f32;
struct Function {}

// impl Value for Function {}

trait Value {
    fn eval(self, env: Environ) -> (Environ, ());
}

impl<T: Iterator<Item = impl Value>> Value for T {
    fn eval(mut self, _env: Environ) -> (Environ, ()) {
        let _a = self.next().unwrap();
        let _b = _a.eval(_env);
        todo!()
    }
}

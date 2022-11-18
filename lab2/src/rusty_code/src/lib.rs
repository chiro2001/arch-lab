use lazy_static::lazy_static;
use mut_static::MutStatic;

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        // fn rusty_cxxbridge_integer() -> i32;
    }
}

#[no_mangle]
pub extern "C" fn rust_start() -> bool { true }

#[no_mangle]
pub extern "C" fn rust_finish(f0: u64, f1: u64, f2: u64, f3: u64) -> bool {
    // let mut data: Vec<u8> = vec![];
    // for f in vec![f0, f1, f2, f3] {
    //     for c in f.to_le_bytes() {
    //         data.push(c)
    //     }
    // }
    // let args = String::from_utf8(data).unwrap();
    // println!("args: {}", args);
    // println!("args: {}", f0);
    // println!("hi");
    // true

    // f0 == 0
    vec![f0, f1, f2, f3].iter().sum::<u64>() == 0
}

pub fn rusty_cxxbridge_integer() -> i32 {
    42
}

#[no_mangle]
pub extern "C" fn rusty_extern_c_integer() -> i32 {
    322
}


#[derive(Debug, Default, Copy, Clone)]
pub struct TestResult {
    pub taken_correct: u64,
    pub taken_incorrect: u64,
    pub not_taken_correct: u64,
    pub not_taken_incorrect: u64,
    pub used: bool,
    pub name: &'static str,
}

impl TestResult {
    pub fn name_me(&mut self, name: &'static str) -> &mut Self {
        self.name = name;
        self
    }
}

const METHOD_COUNT_MAX: usize = 4;
lazy_static! {
    pub static ref RESULTS: MutStatic<Vec<TestResult>> = MutStatic::from(vec![TestResult::default(); METHOD_COUNT_MAX]);
}

pub fn predict_branch(pc: u64, direction: bool) {}

fn print_result() {
    for result in RESULTS.read().unwrap().iter() {
        println!("{:?}", result);
    }
}

#[cfg(test)]
mod test {
    use crate::{print_result, RESULTS};

    #[test]
    fn simple_test() {
        RESULTS.write().unwrap()[0].name_me("Hi");
        print_result()
    }
}
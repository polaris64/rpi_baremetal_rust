/*
 * Using standard library
 */
/*
#![feature(allocator_api, heap_api)]

use std::alloc::{GlobalAlloc, Layout};
*/

#![no_std]
#![feature(alloc, allocator_api, heap_api, lang_items, panic_implementation)]

// Needed for LLVM symbols such as memcpy
extern crate rlibc;


/*
 * Define a custom allocator
 */
use core::alloc::{GlobalAlloc, Layout};

// Linker symbols
extern {
    static __heap_start: u32;
}

struct HeapAllocator;

unsafe impl GlobalAlloc for HeapAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        static mut OFF: u32 = 0x0;
        let ptr: *mut u8 = (&__heap_start + OFF) as *mut u8;
        OFF += layout.size() as u32;
        ptr
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // TODO: implement!
    }
}

#[global_allocator]
static GLOBAL: HeapAllocator = HeapAllocator;

use core::panic::PanicInfo;

#[panic_implementation]
#[no_mangle]
pub fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[lang = "oom"]
#[no_mangle]
pub extern "C" fn oom() -> ! {
    loop {}
}

#[no_mangle]
pub fn __aeabi_unwind_cpp_pr0() {
    loop {}
}

#[no_mangle]
pub fn __aeabi_unwind_cpp_pr1() {
    loop {}
}

extern crate alloc;
//use alloc::arc::Arc;
//use alloc::boxed::Box;
//use alloc::vec::Vec;
//use alloc::string::String;

use alloc::fmt::*;

#[no_mangle]
pub extern fn add(lhs: u32, rhs: u32) -> u32 {
    lhs + rhs
}

#[no_mangle]
pub extern fn string_test(num: u32) -> *const u8 {
    unsafe {
        let s = format(format_args!("{0}: add({0}, {0}) = {1} (heap starts at {2:p})", num, add(num, num), &__heap_start));
        s.as_ptr()
    }
}

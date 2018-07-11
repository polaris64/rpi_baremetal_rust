/*
 * Using standard library
 */
/*
#![feature(allocator_api, heap_api)]

use std::alloc::{GlobalAlloc, Layout};
*/

#![no_std]
#![feature(alloc, allocator_api, core_intrinsics, heap_api, lang_items, panic_implementation)]

// Linker symbols
extern {
    static __heap_start: u32;
}

extern crate alloc;

// Needed for LLVM symbols such as memcpy
extern crate rlibc;


/*
 * Use WeeAlloc as global allocator
 */
extern crate wee_alloc;

#[global_allocator]
static GLOBAL: wee_alloc::WeeAlloc = wee_alloc::WeeAlloc::INIT;

/*
 * Define a custom allocator
 */
/*
use core::alloc::{GlobalAlloc, Layout};

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
*/

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

//use alloc::arc::Arc;
//use alloc::vec::Vec;
//use alloc::string::String;

use alloc::boxed::Box;
use alloc::fmt::*;

#[no_mangle]
pub extern fn add(lhs: u32, rhs: u32) -> u32 {
    lhs + rhs
}

#[no_mangle]
pub extern fn string_test(num: u32) {
    let test_box = Box::new(42);
    let s = format(
        format_args!(
            "Input number is {0}. Calling add({0}, {0}): result is {1}. Address of test_box is {2:p}",
            num,
            add(num, num),
            &test_box
        )
    );
    Uart::puts(&s);
}

mod font8x8;
mod framebuffer;
mod gpio;
mod mmio;
mod uart;

use uart::Uart;
use framebuffer::{FrameBuffer24, Pixel24};

#[no_mangle]
pub extern "C" fn rust_main() {
    Uart::init();

    let col_blue:   Pixel24 = Pixel24 {r: 100, g: 128, b: 250};
    let col_green:  Pixel24 = Pixel24 {r: 100, g: 250, b: 128};
    let col_white:  Pixel24 = Pixel24 {r: 255, g: 255, b: 255};

    Uart::puts("Initialising framebuffer... ");

    let mut fb_res = FrameBuffer24::new(800, 600);

    match fb_res {
        Ok(ref mut fb) => {
            Uart::puts("OK\n");
            let s = format(format_args!("{:?}", fb));
            fb.draw_test_pattern();
            fb.write_string("-------------------------------------------------------------------------------\n",   &col_blue);
            fb.write_string("--== Welcome to the Raspberry Pi bare-metal system, by Simon Pugnet (2018) ==--\n",   &col_blue);
            fb.write_string("-------------------------------------------------------------------------------\n\n", &col_blue);
            fb.write_string("Framebuffer details: -\n", &col_green);
            fb.write_string(&s, &col_green);
            fb.write_string("\n\n", &col_green);
        },
        Err(_) => Uart::puts("ERROR\n"),
    }

    loop {
        match fb_res {
            Ok(ref mut fb) => fb.putcursor(&col_white),
            _ => {},
        }
        let ch: u8 = Uart::getc();
        Uart::putc(ch);
        match fb_res {
            Ok(ref mut fb) => fb.writechar(ch as char, &col_white),
            _ => {},
        }
    }
}

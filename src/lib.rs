/*
 * Using standard library
 */
/*
#![feature(allocator_api, heap_api)]

use std::alloc::{GlobalAlloc, Layout};
*/

#![no_std]
#![feature(alloc, allocator_api, core_intrinsics, lang_items, panic_implementation)]

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

#[panic_handler]
#[no_mangle]
pub fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

use core::alloc::Layout;

#[lang = "oom"]
#[no_mangle]
pub extern "C" fn oom(_layout: Layout) -> ! {
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

use alloc::fmt::*;
use alloc::string::String;


mod font8x8;
mod framebuffer;
mod gpio;
mod mmio;
mod uart;

use uart::Uart;
use framebuffer::{FrameBuffer24, Pixel24};

fn write_prompt(fb: &mut FrameBuffer24, col: &Pixel24) {
    fb.writechar('>', col);
    fb.writechar(' ', col);
}

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
            write_prompt(fb, &col_green);
        },
        Err(_) => Uart::puts("ERROR\n"),
    }

    let mut command: String = String::with_capacity(255);

    loop {
        match fb_res {
            Ok(ref mut fb) => fb.putcursor(&col_white),
            _ => {},
        }
        let ch: u8 = Uart::getc();
        Uart::putc(ch);
        match fb_res {
            Ok(ref mut fb) => {
                fb.writechar(ch as char, &col_white);
                if ch as char == '\n' || ch as char == '\r' {
                    let s = format(format_args!("Your command was: {}\n", command));
                    fb.write_string(&s, &col_green);
                    command.clear();
                    write_prompt(fb, &col_green);
                } else {
                    command.push(ch as char);
                }
            },
            _ => {},
        }

    }
}

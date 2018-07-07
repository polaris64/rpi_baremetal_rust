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
    uart_puts(&s);
}

mod font8x8;
mod framebuffer;
mod gpio;
mod mmio;
mod uart;

use uart::{uart_init, uart_putc, uart_getc, uart_puts};
use framebuffer::{
    FBInitResult,
    FrameBufferInfo,
    Pixel24,
    fb24_draw_test_pattern,
    fb24_putcursor,
    fb24_writechar,
    fb24_write_string,
    init_framebuffer,
};

#[no_mangle]
pub extern "C" fn rust_main() {
    uart_init();

    uart_puts("Initialising framebuffer...\n");

    let mut fbinfo: FrameBufferInfo = FrameBufferInfo::new();
    let fb_res:     FBInitResult    = init_framebuffer(&mut fbinfo, 800, 600, 24);
    let col_blue:   Pixel24 = Pixel24 {r: 100, g: 128, b: 250};
    let col_green:  Pixel24 = Pixel24 {r: 100, g: 250, b: 128};
    let col_white:  Pixel24 = Pixel24 {r: 255, g: 255, b: 255};

    match fb_res {
        FBInitResult::Success => {
            uart_puts("Framebuffer initialised, displaying info...\n");
            let s = format(format_args!("{:?}", fbinfo));
            uart_puts(&s);
            uart_puts("\n");
            fb24_draw_test_pattern(&fbinfo);

            fb24_write_string(&mut fbinfo, "-------------------------------------------------------------------------------\n",   &col_blue);
            fb24_write_string(&mut fbinfo, "--== Welcome to the Raspberry Pi bare-metal system, by Simon Pugnet (2018) ==--\n",   &col_blue);
            fb24_write_string(&mut fbinfo, "-------------------------------------------------------------------------------\n\n", &col_blue);
            fb24_write_string(&mut fbinfo, "Framebuffer details: -\n", &col_green);
            fb24_write_string(&mut fbinfo, &s, &col_green);
            fb24_write_string(&mut fbinfo, "\n\n", &col_green);
        },
        FBInitResult::RequestNotProcessed => uart_puts("Framebuffer: unable to process initialisation request\n"),
        FBInitResult::ResponseError       => uart_puts("Framebuffer: initialisation error\n"),
    }

    loop {
        fb24_putcursor(&fbinfo, &col_white);
        let ch: u8 = uart_getc();
        uart_putc(ch);
        fb24_writechar(&mut fbinfo, ch as char, &col_white);
    }
}

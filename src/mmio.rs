use core::ptr;

// 0x20000000 on RPi 1
// 0x3F000000 on RPi 2+
pub const PERIPHERAL_BASE: usize = 0x3F000000;

// GPIO registers
pub const GPIO_BASE: usize = 0x200000;
pub const GPPUD:     usize = GPIO_BASE + 0x94; // GPIO pin pull-up/down enable
pub const GPPUDCLK0: usize = GPIO_BASE + 0x98; // GPIO pin pull-up/down enable clock 0
//pub const GPPUDCLK1: usize = GPIO_BASE + 0x9c; // GPIO pin pull-up/down enable clock 1

// UART0 registers
pub const UART0_BASE:   usize = PERIPHERAL_BASE + GPIO_BASE + 0x1000;
pub const UART0_DR:     usize = UART0_BASE + 0x00; // Data register
//pub const UART0_RSRECR: usize = UART0_BASE + 0x04; // Read status register
pub const UART0_FR:     usize = UART0_BASE + 0x18; // Flag register
pub const UART0_IBRD:   usize = UART0_BASE + 0x24; // Integer Baud rate divisor
pub const UART0_FBRD:   usize = UART0_BASE + 0x28; // Fractional Baud rate divisor
pub const UART0_LCRH:   usize = UART0_BASE + 0x2c; // Line control register
pub const UART0_CR:     usize = UART0_BASE + 0x30; // Control register
pub const UART0_IMSC:   usize = UART0_BASE + 0x38; // Interrupt mask set clear register
pub const UART0_ICR:    usize = UART0_BASE + 0x44; // Interrupt clear register

pub const GPU_MAILBOX_BASE:   usize = PERIPHERAL_BASE + 0xB880;
pub const GPU_MAILBOX_READ:   usize = GPU_MAILBOX_BASE;
pub const GPU_MAILBOX_STATUS: usize = GPU_MAILBOX_BASE + 0x18;
pub const GPU_MAILBOX_WRITE:  usize = GPU_MAILBOX_BASE + 0x20;

/*
 * MMIO read/write operations must be volatile as read/write has side-effects and must not be
 * optimised by the compiler. core::ptr::(read|write)_volatile<T>() methods are used to perform
 * these volatile actions.
 */

pub fn mmio_read(addr: usize) -> u32 {
    unsafe {
        ptr::read_volatile::<u32>(addr as *const u32)
    }
}

pub fn mmio_write(addr: usize, data: u32) {
    unsafe {
        ptr::write_volatile::<u32>(addr as *mut u32, data);
    }
}

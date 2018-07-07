use mmio::{self, mmio_write, mmio_read};

extern "C" {
    fn delay(count: u32);
}

pub fn uart_init() {
    // Disables all aspects of UART using CR
    mmio_write(mmio::UART0_CR, 0x0);

    // Disable GPIO pins: writing 0 to GPPUD marks that pins should be disabled,
    // and GPPUDCLK0 marks which pins. Finally, writing 0 to GPPUDCLK0 fialises
    // the changes.
    mmio_write(mmio::GPPUD, 0x0);
    unsafe { delay(150); }
    mmio_write(mmio::GPPUDCLK0, (1 << 14) | (1 << 15));
    unsafe { delay(150); }
    mmio_write(mmio::GPPUDCLK0, 0x0);

    // Set all flags in the Interrupt Clear Register (clear all pending
    // interrupts)
    mmio_write(mmio::UART0_ICR, 0x7ff);

    // Set baud rate to 115200. Divisor is UART_CLOCK_SPEED / (16 * BAUD). The
    // integer part is stored in UART0_IBRD while the fractional portion is
    // stored in UART0_FBRD (FRAC * 64) + 0.5.  Result ends up being ~1.67, so
    // UART0_IBRD is 1 and UART0_FBRD is (0.67 * 64) + 0.5 = 43.
    mmio_write(mmio::UART0_IBRD,  1);
    mmio_write(mmio::UART0_FBRD, 43);

    // Write bit 4, 5 and 6 to Line Control Register: -
    //   - Bit 4: use 8 item deep FIFO instead of single item register
    //   - Bits 5 and 6: use 8-bit words for RX/TX
    mmio_write(mmio::UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Disable all interrupts from UART0 by setting relevant bits in Interrupt
    // Mask Set Clear register
    mmio_write(mmio::UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    // Bit 0: enable UART0 hardware
    // Bit 8: enable RX
    // Bit 9: enable TX
    mmio_write(mmio::UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

pub fn uart_putc(ch: u8) {
    // Loop until flag register bit 5 (TXFF: transmit FIFO is full) is unset
    loop {
        let reg: u32 = mmio_read(mmio::UART0_FR);
        if reg & (1 << 5) == 0 {
            break;
        }
    }

    // Write character to data register
    mmio_write(mmio::UART0_DR, ch as u32);
}

pub fn uart_getc() -> u8 {
    // Loop until flag register bit 4 (RXFE: receive FIFO is empty) is unset
    loop {
        let reg: u32 = mmio_read(mmio::UART0_FR);
        if reg & (1 << 4) == 0 {
            break;
        }
    }

    // Read character from data register and return
    mmio_read(mmio::UART0_DR) as u8
}

pub fn uart_puts(s: &str) {
    for ch in s.as_bytes() {
        uart_putc(*ch);
    }
}

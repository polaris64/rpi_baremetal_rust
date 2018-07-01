#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib.h"
#include "utility.h"
#include "gpio.h"
#include "timers.h"
#include "framebuffer.h"
#include "uart.h"

framebuffer_info_t fbinfo;
extern uint32_t __start;
extern uint32_t __end;

// Rust test function
extern uint32_t add(uint32_t, uint32_t);
extern char* string_test(uint32_t);

void death_loop(uint32_t min, uint32_t max, uint32_t step)
{
  bool     led_on     = true;
  uint32_t wait_time  = min;
  int32_t  wait_delta = step;

  // Set GPIO pin 47 (ACT LED) to "write"
  setGpioFunction(47, 1);

  while (1)
  {
    // Set/unset pin output
    setGpio(47, led_on);

    // Wait for a bit...
    delay(wait_time);

    // Toggle led_on
    led_on = !led_on;

    // Update wait_time
    wait_time += wait_delta;
    if (wait_time < min || wait_time > max)
    {
      wait_time  = wait_delta > 0 ? max   : min;
      wait_delta = wait_delta > 0 ? -step : step;
    }
  }
}

void output_debug_info(framebuffer_info_t *fbinfo, pixel_24bpp_t col)
{
  char buf[32];

  fb24_writestr(fbinfo, "Kernel details: -", col);

  // Output kernel base address
  fb24_writestr(fbinfo, "\n - Loaded at: 0x", col);
  itoa(buf, 32, (int32_t)((uint32_t)&__start), 16);
  fb24_writestr(fbinfo, buf, col);

  // Output kernel end address
  fb24_writestr(fbinfo, "\n - Ends at:   0x", col);
  itoa(buf, 32, (int32_t)((uint32_t)&__end), 16);
  fb24_writestr(fbinfo, buf, col);

  // Output kernel size
  fb24_writestr(fbinfo, "\n - Size:      0x", col);
  itoa(buf, 32, (int32_t)((uint32_t)(&__end) - (uint32_t)(&__start)), 16);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n\n", col);
}

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
  (void) r0;
  (void) r1;
  (void) atags;
  unsigned char ch = 0;
  uint32_t total = 0;
  uint32_t width  = 800;
  uint32_t height = 600;
  uint8_t bpp = 24;
  pixel_24bpp_t p24_white = {0xff, 0xff, 0xff};
  pixel_24bpp_t p24_blue  = {0x60, 0x80, 0xff};

  // Initialise the framebuffer
  if (init_framebuffer(&fbinfo, width, height, bpp) != 0)
    death_loop(0x1000, 0x20000, 0x1000);

  // Initialise the UART
  uart_init();

  // Draw a FB background (for testing)
  drawFBTestPattern(&fbinfo, bpp, ch);

  // Write some introductory text to the FB
  fb24_writestr(
    &fbinfo,
    "Simple Raspberry PI 2 OS by Simon Pugnet\n----------------------------------------\n\n",
    p24_blue
  );

  // Print system information
  output_debug_info(&fbinfo, p24_blue);
  fb24_print_debug_info(&fbinfo, p24_blue);

  fb24_writestr(&fbinfo, "\n", p24_white);
  fb24_writestr(
    &fbinfo,
    "Characters received on the serial bus will be echoed to this framebuffer.\n\n",
    p24_blue
  );


  for (char i = 0x20; i < 0x80; i++)
    fb24_writechar(&fbinfo, i, p24_white);
  fb24_writechar(&fbinfo, '\n', p24_white);

  while (1)
  {
    // Draw the cursor
    fb24_putcursor(&fbinfo, p24_white);

    // Wait for an echo character over UART
    ch = uart_getc();
    total += ch;
    uart_putc(ch);

    // Write the received character to the FB
    fb24_writechar(&fbinfo, ch, p24_white);

    if (ch == '\r')
    {
      fb24_writestr(&fbinfo, string_test(total), p24_white);
      fb24_writestr(&fbinfo, "\n", p24_white);
      total = 0;
    }
  }

  uart_puts("Halting...\n");

  // Returning from kernel_main returns to instruction after BL kernel_main in
  // main.S, which is the CPU core halt loop
}

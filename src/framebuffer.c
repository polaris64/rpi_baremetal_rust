#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "framebuffer.h"

#include "font8x8.h"
#include "lib.h"

// TODO: define bitfields for mailbox structs: -
//
// typedef struct {
//   uint8_t  channel:  4; // First 4 bits are channel ID
//   uint32_t data:    28; // Top 28 bits are message data
// } mail_message_t;
//
// typedef struct {
//   uint32_t reserved: 30;
//   uint8_t  empty:     1; // bit 31 must be unset before reading
//   uint8_t  full:      1; // bit 32 must be unset before writing
// } mail_status_t;

// Property channel messages
//   - Must be 16-byte aligned
//   - Structure (each is 32-bit): -
//     -      0x00: Buffer size 
//     -      0x04: Request/response code
//     -      0x08: Tags
//     -        NN: ...Tags...
//     - 0xNN +  4: End Tag (all 0)
//     - 0xNN +  8: Padding
//     - 0xNN + 16: Padding
//   - Tags: -
//     - Command to get/set data about hardware device
//     - 0x00: Tag ID (0x000XYZZZ (X: device ID, Y: command type (0: get, 4: test, 8: set), Z: specific command))
//     - 0x04: Value buffer size in bytes
//     - 0x08: Request/response code
//     - 0x0C: Value buffer (length in 0x04)
//     - 0xNN: Value buffer ---->| then Padding

// Get framebuffer
//   - Set screen size (Tag ID 0x00048003)
//   - Set virtual screen size (Tag ID 0x00048004)
//     - Value buffer for both is 4 byte width and 4 byte height
//   - Set depth (Tag ID 0x00048005)
//     - Value buffer is 4 byte depth
//
// So the overall message should be: -
//  - 80 (32-bit): message size
//  -  0 (32-bit): this is a request, so code is 0
//  - 0x00048003, 8, 0, 1024, 768: 1st Tag, 8 byte value buffer, 0 (request), 1024 width, 768 height
//  - 0x00048004, 8, 0, 1024, 768: 2nd Tag, 8 byte value buffer, 0 (request), 1024 width, 768 height
//  - 0x00048005, 4, 0,   32:      3rd Tag, 4 byte value buffer, 0 (request), 32bpp
//  - 0 (32-bit): end Tag
//  - 0, 0, 0: padding (16-byte alignment)
//
// The buffer must be 16-byte aligned so that the low 4 bits are 0.
// Bitwise OR this address with 8 to set channel number (1000b)
// Send to mailbox.
// Check response code in buffer: 0 means error (not processed), 0x80000001 is an error code, 0x80000000 is success
//
// Then need to request framebuffer with Tag ID 0x00040001: -
//  - 32 (32-bit): message size
//  -  0 (32-bit): request
//  - 0x00040001, 8, 0, 16, 0: 8 byte value buffer, 0 (request), 4 byte requested alignment (16 byte), 4 byte for response (FB address)
//  - 0 (32-bit): end Tag
//   
// On success, the FB address will be stored in the value buffer of the 0x00040001 Tag above.

mail_message_t mailbox_read(int channel)
{
  uint32_t       counter   =     0;
  uint32_t       max_count = 20000;
  mail_status_t  stat;
  mail_message_t res;

  // Loop until the channel read matches the requested channel
  do
  {

    // Loop until the mailbox status shows not empty
    do
    {
      stat = *GPU_MAILBOX_STATUS;
      counter++;
    } while (stat.empty && counter < max_count);

    // Read from mailbox
    res = *GPU_MAILBOX_READ;

  } while (res.channel != channel && counter < max_count);

  return res;
}

void mailbox_write(mail_message_t msg, int channel)
{
  mail_status_t stat;
  msg.channel = channel;

  // Loop until the mailbox status shows not full
  do
  {
    stat = *((mail_status_t*)GPU_MAILBOX_STATUS);
  } while (stat.full);

  // Write message to mailbox
  *GPU_MAILBOX_WRITE = msg;
}

uint32_t init_framebuffer(framebuffer_info_t *fbinfo, uint32_t width, uint32_t height, uint32_t bits)
{
  fb_init_message_t  req       __attribute((aligned(16)));
  fb_alloc_message_t req_alloc __attribute((aligned(16)));
  mail_message_t     mail;

  req.type = REQUEST;

  // Tag 0: set physical dimensions (size: 20)
  req.tags[0].proptag = FB_SET_PHYSICAL_DIMENSIONS;
  req.tags[0].size    = 2 * sizeof(uint32_t);
  req.tags[0].type    = REQUEST;
  req.tags[0].value_buffer.fb_screen_size.width  = width;
  req.tags[0].value_buffer.fb_screen_size.height = height;

  // Tag 1: set virtual dimensions (size: 20)
  req.tags[1].proptag = FB_SET_VIRTUAL_DIMENSIONS;
  req.tags[1].size    = 2 * sizeof(uint32_t);
  req.tags[1].type    = REQUEST;
  req.tags[1].value_buffer.fb_screen_size.width  = width;
  req.tags[1].value_buffer.fb_screen_size.height = height;

  // Tag 2: set bit depth (size: 16)
  req.tags[2].proptag = FB_SET_BITS_PER_PIXEL;
  req.tags[2].size    = sizeof(uint32_t);
  req.tags[2].type    = REQUEST;
  req.tags[2].value_buffer.fb_bits_per_pixel = bits;

  req.end_tag = NULL_TAG;

  req.size = 3 * sizeof(uint32_t) + (req.tags[0].size + req.tags[1].size + req.tags[2].size) + (9 * sizeof(uint32_t));

  // Message size must be padded to 16 byte alignment
  req.size += req.size % 16 ? 16 - (req.size % 16) : 0;

  // Low 4 bits of address are 0 as address is 16 byte aligned abnd "data" must
  // be highest 28 bits (32 - 4), so shift right by 4
  mail.data = ((uint32_t)&req) >> 4;

  mailbox_write(mail, PROPERTY_CHANNEL);
  mail = mailbox_read(PROPERTY_CHANNEL);

  if (req.type == REQUEST)
    return 1;
  if (req.type == RESPONSE_ERROR)
    return 2;

  // Fill in all possible fbinfo attributes so far
  fbinfo->width  = req.tags[0].value_buffer.fb_screen_size.width;
  fbinfo->height = req.tags[0].value_buffer.fb_screen_size.height;
  fbinfo->chars_width  = fbinfo->width  / CHAR_WIDTH;
  fbinfo->chars_height = fbinfo->height / CHAR_HEIGHT;
  fbinfo->x = 0;
  fbinfo->y = 0;
  fbinfo->pitch = fbinfo->width * (bits / 8);

  // Create allocate framebuffer request
  req_alloc.type = REQUEST;
  req_alloc.tag.proptag = FB_ALLOCATE_BUFFER;
  req_alloc.tag.size    = 2 * sizeof(uint32_t);
  req_alloc.tag.type    = REQUEST;
  req_alloc.tag.value_buffer.fb_screen_size.width  = 0;
  req_alloc.tag.value_buffer.fb_screen_size.height = 0;
  req_alloc.tag.value_buffer.fb_allocate_align     = 16;
  req_alloc.end_tag = NULL_TAG;
  req_alloc.size = 6 * sizeof(uint32_t) + req_alloc.tag.size;
  req_alloc.size += req_alloc.size % 16 ? 16 - (req_alloc.size % 16) : 0;

  mail.data = ((uint32_t)&req_alloc) >> 4;
  mailbox_write(mail, PROPERTY_CHANNEL);
  mail = mailbox_read(PROPERTY_CHANNEL);

  if (req_alloc.type == REQUEST)
    return 1;
  if (req_alloc.type == RESPONSE_ERROR)
    return 2;

  fbinfo->buf  = req_alloc.tag.value_buffer.fb_allocate_res.fb_addr;
  fbinfo->size = req_alloc.tag.value_buffer.fb_allocate_res.fb_size;

  return 0;
}

void drawFBTestPattern(framebuffer_info_t *fbinfo, uint8_t bpp, unsigned char offset)
{
  uint16_t x = 0;
  uint16_t y = 0;
  uint32_t off_y = 0, off = 0;
  uint32_t pitch    = fbinfo->pitch;
  uint32_t bytes_pp = bpp / 8;
  pixel_16bpp_t p16;
  pixel_24bpp_t p24;
  pixel_32bpp_t p32;

  switch (bpp)
  {
    case 16:
      for (y = 0; y < fbinfo->height; y++)
      {
        off_y = y * pitch;
        for (x = 0; x < fbinfo->width; x++)
        {
          off = off_y + (x * bytes_pp);
          p16.r = x + y + offset;
          p16.g = x - y + offset;
          p16.b = x + y - offset;
          *(pixel_16bpp_t*)(fbinfo->buf + off) = p16;
        }
      }
      break;
    case 24:
      for (y = 0; y < fbinfo->height; y++)
      {
        off_y = y * pitch;
        for (x = 0; x < fbinfo->width; x++)
        {
          off = off_y + (x * bytes_pp);
          p24.r = x + y + offset;
          p24.g = x - y + offset;
          p24.b = x + y - offset;
          *(pixel_24bpp_t*)(fbinfo->buf + off) = p24;
        }
      }
      break;
    case 32:
      for (y = 0; y < fbinfo->height; y++)
      {
        off_y = y * pitch;
        for (x = 0; x < fbinfo->width; x++)
        {
          off = off_y + (x * bytes_pp);
          p32.r = x + y + offset;
          p32.g = x - y + offset;
          p32.b = x + y - offset;
          *(pixel_32bpp_t*)(fbinfo->buf + off) = p32;
        }
      }
      break;
  }
}

void fb24_scroll_y(framebuffer_info_t *fbinfo, uint32_t pixels)
{
  uint32_t x, y;
  pixel_24bpp_t *src, *dst;
  pixel_24bpp_t black = {0, 0, 0};

  // Scroll current pixels
  for (y = pixels; y < fbinfo->height; y++)
  {
    for (x = 0; x < fbinfo->width; x++)
    {
      src = (pixel_24bpp_t*)(fbinfo->buf + (y            * fbinfo->pitch) + (x * 3));
      dst = (pixel_24bpp_t*)(fbinfo->buf + ((y - pixels) * fbinfo->pitch) + (x * 3));
      *dst = *src;
    }
  }

  // Fill new pixels with black
  for (y = fbinfo->height - pixels; y < fbinfo->height; y++)
  {
    for (x = 0; x < fbinfo->width; x++)
    {
      dst = (pixel_24bpp_t*)(fbinfo->buf + (y * fbinfo->pitch) + (x * 3));
      *dst = black;
    }
  }
}

inline void fb24_putpixel(framebuffer_info_t *fbinfo, uint32_t x, uint32_t y, pixel_24bpp_t p)
{
  if (x > fbinfo->width - 1)
    return;
  if (y > fbinfo->height - 1)
    return;
  *((pixel_24bpp_t*)(fbinfo->buf + (y * fbinfo->pitch) + (x * 3))) = p;
}

void fb24_putchar(framebuffer_info_t *fbinfo, unsigned char ch, uint32_t posx, uint32_t posy, pixel_24bpp_t col)
{
  unsigned char x, y;
  pixel_24bpp_t black = {0, 0, 0};
  char *char_data;
  char  row_data;

  if (ch >= 0x80)
    return;

  char_data = font8x8[ch];

  // Draw character at (posx,posy)
  for (y = 0; y < 8; y++)
  {
    row_data = char_data[y];
    for (x = 0; x < 8; x++)
    {
      fb24_putpixel(
        fbinfo,
        posx + x,
        posy + y, 
        (row_data & (0x80 >> x)) != 0 ? col : black
        //(row_data & (0x01 << x)) != 0 ? col : black
      );
    }
  }
}

void fb24_writechar(framebuffer_info_t *fbinfo, unsigned char ch, pixel_24bpp_t col)
{
  uint32_t offx = fbinfo->x * CHAR_WIDTH;
  uint32_t offy = fbinfo->y * CHAR_HEIGHT;
  pixel_24bpp_t black = {0, 0, 0};
  uint32_t diff;

  // Carriage return
  if (ch == '\n' || ch == '\r')
  {
    fb24_putcursor(fbinfo, black);
    fbinfo->x = 0;
    fbinfo->y++;
    goto handlescroll;
  }

  // Backspace
  if (ch == '\b')
  {
    if (fbinfo->x == 0)
    {
      fbinfo->x = fbinfo->chars_width - 1;
      fbinfo->y--;
    }
    else
      fbinfo->x--;
    return;
  }

  fb24_putchar(fbinfo, ch, offx, offy, col);

  // Increment FB cursor position
  fbinfo->x++;
  if (fbinfo->x > fbinfo->chars_width)
  {
    fbinfo->x = 0;
    fbinfo->y++;
    goto handlescroll;
  }

  return;

handlescroll:
  // Scroll FB if necessary
  if (fbinfo->y >= fbinfo->chars_height)
  {
    diff = fbinfo->y - fbinfo->chars_height + 1;
    fb24_scroll_y(fbinfo, diff * CHAR_HEIGHT);
    fbinfo->y -= diff;
  }
}

inline void fb24_putcursor(framebuffer_info_t *fbinfo, pixel_24bpp_t col)
{
  unsigned char x, y;
  uint32_t offx = fbinfo->x * CHAR_WIDTH;
  uint32_t offy = fbinfo->y * CHAR_HEIGHT;

  // Draw cursor at (offx,offy)
  for (y = 0; y < 8; y++)
    for (x = 0; x < 8; x++)
      fb24_putpixel(fbinfo, offx + x, offy + y, col);
}

void fb24_writestr(framebuffer_info_t *fbinfo, char *str, pixel_24bpp_t col)
{
  char *ch;
  for (ch = str; *ch != '\0'; ch++)
    fb24_writechar(fbinfo, *ch, col);
}

void fb24_print_debug_info(framebuffer_info_t *fbinfo, pixel_24bpp_t col)
{
  char buf[32];

  fb24_writestr(fbinfo, "Framebuffer details: -\n", col);
  fb24_writestr(fbinfo, " - Width:   ", col);
  itoa(buf, 32, fbinfo->width, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Height:  ", col);
  itoa(buf, 32, fbinfo->height, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Length:  ", col);
  itoa(buf, 32, fbinfo->size, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Pitch:   ", col);
  itoa(buf, 32, fbinfo->pitch, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Chars X: ", col);
  itoa(buf, 32, fbinfo->chars_width, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Chars Y: ", col);
  itoa(buf, 32, fbinfo->chars_height, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - Address: 0x", col);
  itoa(buf, 32, (uint32_t)fbinfo->buf, 16);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n - BPP:     ", col);
  itoa(buf, 32, (fbinfo->pitch / fbinfo->width) * 8, 10);
  fb24_writestr(fbinfo, buf, col);
  fb24_writestr(fbinfo, "\n", col);
}

/*
bool GPUMailboxWrite(uint8_t mailbox, uint32_t value)
{
  volatile uint32_t *reg_status = (uint32_t*)(PERIPHERAL_BASE | GPU_MAILBOX_STATUS);
  volatile uint32_t *reg_write  = (uint32_t*)(PERIPHERAL_BASE | GPU_MAILBOX_WRITE);

  // Mailbox must be 0..15
  if (mailbox > 15)
    return false;
  
  // Lowest 4 bits of value must all be 0
  if ((value & 0x0F) != 0)
    return false;

  // Wait for topmost bit of status register to become 0
  while ((*reg_status & 0x80000000) != 0);

  // Combine mailbox and value to one 32-bit value and write to "Write"
  // register
  *reg_write = (uint32_t)mailbox | value;

  return true;
}

uint32_t GPUMailboxRead(uint8_t mailbox)
{
  volatile uint32_t *reg_status = (uint32_t*)(PERIPHERAL_BASE | GPU_MAILBOX_STATUS);
  volatile uint32_t *reg_read   = (uint32_t*)(PERIPHERAL_BASE | GPU_MAILBOX_READ);
  uint32_t read = 0;

  // Mailbox must be 0..15
  if (mailbox > 15)
    return 1;

  // Keep reading until the read mailbox matches the mailbox from which we wish
  // to read
  do
  {
    // Wait for 30th bit of status register to become 0
    while (((*reg_status) & 0x40000000) != 0);

    read = *reg_read;
  }
  while ((read & 0x0F) != mailbox);

  return read >> 4;
}

bool initFramebuffer(volatile FramebufferInfo *info)
{
  volatile uint32_t addr = (uint32_t)((uint8_t*)info);
  uint32_t res = 1;

  if (!GPUMailboxWrite(1, addr))
    return false;

  res = GPUMailboxRead(1);
  return res == 0;
}

bool initFramebufferInfo(volatile FramebufferInfo *fbinfo, uint32_t w, uint32_t h, uint8_t bits)
{
  if (w > 4096)
    return false;
  if (h > 4096)
    return false;
  if (!(bits == 1 || bits == 2 || bits == 4 || bits == 8 || bits == 16 || bits == 24 || bits == 32))
    return false;

  fbinfo->width = w;
  fbinfo->height = h;
  fbinfo->vWidth = w;
  fbinfo->vHeight = h;
  fbinfo->bitDepth = bits;
  fbinfo->pitch = 0;
  fbinfo->x = 0;
  fbinfo->y = 0;
  fbinfo->pointer = 0;
  fbinfo->size = 0;

  return true;
}

void drawFBTestPattern(FramebufferInfo fbinfo, unsigned char offset)
{
  uint16_t x = 0;
  uint16_t y = 0;
  uint32_t off = 0;

  for (y = 0; y < fbinfo.height; y++)
  {
    for (x = 0; x < fbinfo.width; x++)
    {
      off = (y * fbinfo.pitch) + (x * 4);
      *((uint8_t*)fbinfo.pointer + off)     = (uint8_t)(x + y + offset); // R
      *((uint8_t*)fbinfo.pointer + off + 1) = (uint8_t)(x - y - offset); // G
      *((uint8_t*)fbinfo.pointer + off + 2) = (uint8_t)(x + y - offset); // B
      *((uint8_t*)fbinfo.pointer + off + 3) = 255; // A
    }
  }
}
*/

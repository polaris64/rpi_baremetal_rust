#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "framebuffer.h"

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

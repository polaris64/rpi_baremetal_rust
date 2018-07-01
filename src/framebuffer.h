#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "peripherals.h"

#define GPU_MAILBOX_BASE   PERIPHERAL_BASE + 0xB880
#define GPU_MAILBOX_READ   ((mail_message_t*)GPU_MAILBOX_BASE)
#define GPU_MAILBOX_STATUS ((mail_status_t*)(GPU_MAILBOX_BASE + 0x18))
#define GPU_MAILBOX_WRITE  ((mail_message_t*)(GPU_MAILBOX_BASE + 0x20))

#define PROPERTY_CHANNEL    8
#define FRAMEBUFFER_CHANNEL 1

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 8

// Defines a 32-bit mailbox message
typedef struct {
  uint8_t  channel:  4; // Low 4 bits are channel ID
  uint32_t data:    28; // High 28 bits are message data
} mail_message_t;

typedef struct {
  uint32_t reserved: 30;
  uint8_t  empty:     1; // bit 31 must be unset before reading
  uint8_t  full:      1; // bit 32 must be unset before writing
} mail_status_t;

// Property mailbox tag types
typedef enum {
  NULL_TAG                   = 0x0,
  FB_ALLOCATE_BUFFER         = 0x00040001,
  FB_RELEASE_BUFFER          = 0x00048001,
  FB_GET_PHYSICAL_DIMENSIONS = 0x00040003,
  FB_SET_PHYSICAL_DIMENSIONS = 0x00048003,
  FB_GET_VIRTUAL_DIMENSIONS  = 0x00040004,
  FB_SET_VIRTUAL_DIMENSIONS  = 0x00048004,
  FB_GET_BITS_PER_PIXEL      = 0x00040005,
  FB_SET_BITS_PER_PIXEL      = 0x00048005,
  FB_GET_BYTES_PER_ROW       = 0x00040008,
} property_tag_t;

typedef enum {
  REQUEST          = 0x0,
  RESPONSE_SUCCESS = 0x80000000,
  RESPONSE_ERROR   = 0x80000001,
} buffer_req_res_code_t;


// Framebuffer allocation data (for FB_ALLOCATE_BUFFER)
typedef struct {
  void     *fb_addr; // Address of FB start
  uint32_t  fb_size; // Total size of FB
} fb_allocate_res_t;

// Framebuffer size data (for FB_(G|S)ET_(PHYSICAL|VIRTUAL)_DIMENSIONS)
typedef struct {
  uint32_t width;
  uint32_t height;
} fb_screen_size_t;

// A tag value buffer can any one of these types at a time (32 bits)
typedef union {
  uint32_t          fb_allocate_align;
  fb_allocate_res_t fb_allocate_res;
  fb_screen_size_t  fb_screen_size;
  uint32_t          fb_bits_per_pixel;
  uint32_t          fb_bytes_per_row;
} value_buffer_t;

// A property mailbox message can contain one or more of these
typedef struct {
  property_tag_t        proptag;      // Tag type
  uint32_t              size;         // Size of the value buffer (bytes)
  buffer_req_res_code_t type;         // Type: request or response
  value_buffer_t        value_buffer; // Tag value buffer
} property_message_tag_t;

typedef struct {
  uint32_t               size;    // Size of buffer, including "size"
  buffer_req_res_code_t  type;    // Request/response and response type
  property_message_tag_t tags[3]; // Tags for the message
  uint32_t               end_tag; // NULL tag
} fb_init_message_t;

typedef struct {
  uint32_t               size;    // Size of buffer, including "size"
  buffer_req_res_code_t  type;    // Request/response and response type
  property_message_tag_t tag;     // Only Tag for the message
  uint32_t               end_tag; // NULL tag
} fb_alloc_message_t;

typedef struct {
  uint32_t  width;
  uint32_t  height;
  uint32_t  pitch;
  void     *buf;
  uint32_t  size;
  uint32_t  chars_width;
  uint32_t  chars_height;
  uint32_t  x;
  uint32_t  y;
} framebuffer_info_t;

typedef struct {
  uint8_t b: 5;
  uint8_t g: 6;
  uint8_t r: 5;
} __attribute__((packed)) pixel_16bpp_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} __attribute__((packed)) pixel_24bpp_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} __attribute__((packed)) pixel_32bpp_t;

uint32_t get_value_buffer_len(property_message_tag_t*);

uint32_t init_framebuffer(framebuffer_info_t*, uint32_t, uint32_t, uint32_t);
void     drawFBTestPattern(framebuffer_info_t*, uint8_t, unsigned char);

void fb24_scroll_y(framebuffer_info_t*, uint32_t);

// Put pixels, chars, strings at arbitrary locations
void fb24_putpixel(framebuffer_info_t*, uint32_t, uint32_t, pixel_24bpp_t);
void fb24_putchar(framebuffer_info_t*, unsigned char, uint32_t, uint32_t, pixel_24bpp_t);
void fb24_putcursor(framebuffer_info_t*, pixel_24bpp_t);

// Put chars, strings at framebuffer text cursor location
void fb24_writestr(framebuffer_info_t*, char*, pixel_24bpp_t);
void fb24_writechar(framebuffer_info_t*, unsigned char, pixel_24bpp_t);

void fb24_print_debug_info(framebuffer_info_t*, pixel_24bpp_t);


// NOTE: Raspberry Pi 1
/*
typedef struct _framebuffer_info {
  uint32_t width;
  uint32_t height;
  uint32_t vWidth;
  uint32_t vHeight;
  uint32_t pitch;
  uint32_t bitDepth;
  uint32_t x;
  uint32_t y;
  uint32_t pointer;
  uint32_t size;
} FramebufferInfo;


bool     GPUMailboxWrite(uint8_t mailbox, uint32_t value);
uint32_t GPUMailboxRead(uint8_t mailbox);
bool     initFramebuffer(volatile FramebufferInfo *info);
bool     initFramebufferInfo(volatile FramebufferInfo *fbinfo, uint32_t w, uint32_t h, uint8_t bits);
void     drawFBTestPattern(FramebufferInfo, unsigned char);
*/

#endif

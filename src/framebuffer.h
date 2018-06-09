#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "peripherals.h"

#define GPU_MAILBOX_BASE   0xB880UL
#define GPU_MAILBOX_READ   0xB880UL
#define GPU_MAILBOX_STATUS 0xB898UL
#define GPU_MAILBOX_WRITE  0xB8A0UL

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

#endif

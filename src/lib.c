#include <stdint.h>

#include "lib.h"

int itoa(char *buf, uint32_t buf_len, int32_t input, uint8_t base)
{
  int i, neg = 0;
  uint32_t digits = 0;
  uint32_t temp;

  if (input < 0)
  {
    input = -input;
    neg = 1;
    digits++;
    buf[0] = '-';
  }
  temp = input;

  // Find number of digits required
  do
  {
    temp = temp / base;
    digits++;
  } while (temp > 0);

  // Make sure not to overflow buffer
  if (digits >= buf_len - 1)
    return -1;

  for (i = digits - 1; i >= (neg == 0 ? 0 : 1); i--)
  {
    buf[i] = "0123456789ABCDEF"[input % base];
    input /= base;
  }
  buf[digits] = '\0';

  return digits;
}

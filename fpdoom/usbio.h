#ifndef USBIO_H
#define USBIO_H

#include "common.h"

void usb_init_base(void);
void usb_init(void);
int usb_getchar(int wait);
int usb_read(void *dst, unsigned len, int wait);
int usb_write(const void *src, unsigned len);

#define USB_WAIT 1
#define USB_NOWAIT 0

#endif  // USBIO_H

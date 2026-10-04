#ifndef _AVR_PROGMEM_COMPAT_H_
#define _AVR_PROGMEM_COMPAT_H_

// Compatibility shim for <avr/pgmspace.h> on non-AVR platforms
#include <pgmspace.h>

#ifndef PROGMEM
#define PROGMEM
#endif

#endif // _AVR_PROGMEM_COMPAT_H_

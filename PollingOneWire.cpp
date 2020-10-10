#include <Arduino.h>
#include "PollingOneWire.h"
#include "util/OneWire_direct_gpio.h"

#ifdef ARDUINO_ARCH_ESP32
// due to the dual core esp32, a critical section works better than disabling interrupts
#  define noInterrupts() {portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;portENTER_CRITICAL(&mux)
#  define interrupts() portEXIT_CRITICAL(&mux);}
// for info on this, search "IRAM_ATTR" at https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/general-notes.html 
#  define NOT_IN_FLASH IRAM_ATTR
#else
#  define NOT_IN_FLASH 
#endif

#define ONEWIRE_RESET_NO_DEVICES (0)
#define ONEWIRE_RESET_OK (1)
#define ONEWIRE_POLLING_RESET_PULL_LOW_DELAY (2)
#define ONEWIRE_POLLING_RESET_PRESENCE_DELAY (3)


#define POLLING_RESET_NOT_STARTED (0)
#define POLLING_RESET_PULL_LOW_DELAY (1)
#define POLLING_RESET_POST_PRESENCE_DELAY (2)

uint8_t NOT_IN_FLASH PollingOneWire::polling_reset(void)
{
	IO_REG_TYPE mask IO_REG_MASK_ATTR = bitmask;
	volatile IO_REG_TYPE *reg IO_REG_BASE_ATTR = baseReg;
	uint8_t retries = 125;

  // --------------------------------------------------------
  // RESET_ASYNC_NOT_STARTED
  // --------------------------------------------------------
  // state 0 means we have not started the reset cycle
  if (polling_reset_state == RESET_ASYNC_NOT_STARTED) {
    noInterrupts();
    DIRECT_MODE_INPUT(reg, mask);
    interrupts();
    // wait until the wire is high... just in case
    do {
      if (--retries == 0) return ONEWIRE_RESET_NO_DEVICES;
      delayMicroseconds(2);
    } while ( !DIRECT_READ(reg, mask));

    noInterrupts();
    DIRECT_WRITE_LOW(reg, mask);
    DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
    interrupts();

    polling_reset_state = RESET_ASYNC_PULL_LOW_DELAY;
    polling_reset_time = micros();
    return ONEWIRE_RESET_PULL_LOW_DELAY;
  }

  // --------------------------------------------------------
  // RESET_ASYNC_PULL_LOW_DELAY
  // --------------------------------------------------------
  // state 1 means have pulled down the bus and need to keep it
  // low for at least 480us
  if (polling_reset_state == RESET_ASYNC_PULL_LOW_DELAY) {
    if (micros() - polling_reset_time < 480) {
      return ONEWIRE_RESET_PULL_LOW_DELAY;
    }

    noInterrupts();
    DIRECT_MODE_INPUT(reg, mask);	// allow it to float
    delayMicroseconds(70);
    uint8_t r = !DIRECT_READ(reg, mask);
    interrupts();

    if (r != 1) {
      return ONEWIRE_RESET_NO_DEVICES;
    }

    polling_reset_state = RESET_ASYNC_POST_PRESENCE_DELAY;
    polling_reset_time = micros();
    return ONEWIRE_RESET_ASYNC_PRESENCE_DELAY;
  }

  // --------------------------------------------------------
  // RESET_ASYNC_POST_PRESENCE_DELAY
  // --------------------------------------------------------
  if (polling_reset_state == RESET_ASYNC_POST_PRESENCE_DELAY) {
    if (micros() - polling_reset_time < 410) {
      return ONEWIRE_RESET_ASYNC_PRESENCE_DELAY;
    }
  }

  polling_reset_state = RESET_ASYNC_NOT_STARTED; // done
  return ONEWIRE_RESET_OK;
}

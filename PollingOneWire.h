#include <Arduino.h>
#include "OneWire.h"



class PollingOneWire : public OneWire {

  private:
    // the internal polling reset state
    uint8_t polling_reset_state;
    // hold the microseconds 
    unsigned long polling_reset_time;


  public:
    // Perform a 1-Wire reset cycle without blocking. 
    // Returns ONEWIRE_RESET_NO_DEVICES if there is no device or the bus is shorted or otherwise held low for more than 250uS
    // Returns ONEWIRE_RESET_OK if a device responds with a presence pulse.
    // Returns ONEWIRE_RESET_PULL_LOW_DELAY the code needs to wait 480us
    // Returns ONEWIRE_RESET_PRESENCE_DELAY the code needs to wait 410us
    uint8_t polling_reset(void);
};


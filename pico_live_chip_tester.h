#define PRINT_ERROR_THRESHOLD 2
#define CORRECT_ERRORS
// #define REPORT_SLOW_CAPTURE

#define DEBUG
// TODO: could easily drive the RX/TX status LEDs from the PIO, would save precious CPU cycles
// could probably even be done using the PIO's side-set feature
#define STATUS_LEDS
// #define VERBOSE_STATUS_LEDS

#define SWAP_RAS_AND_CAS_ADDRESSES

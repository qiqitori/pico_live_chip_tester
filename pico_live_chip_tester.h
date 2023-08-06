#define PRINT_ERROR_THRESHOLD 2
#define CORRECT_ERRORS
// #define REPORT_SLOW_CAPTURE

#define DEBUG
// TODO: could easily drive the RX/TX status LEDs from the PIO, would save precious CPU cycles
// could probably even be done using the PIO's side-set feature
#define STATUS_LEDS
// #define VERBOSE_STATUS_LEDS

#define SWAP_RAS_AND_CAS_ADDRESSES

extern uint8_t g_memory[65536];
extern bool g_is_read;
extern bool g_is_write;
extern bool g_din;
extern bool g_dout;
extern uint16_t g_state_at_ras;
extern uint16_t g_state_at_cas;
extern uint8_t g_ras_address;
extern uint8_t g_cas_address;
extern uint32_t g_waited;

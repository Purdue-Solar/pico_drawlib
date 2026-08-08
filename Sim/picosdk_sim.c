#ifdef SIMULATION

#include "picosdk_sim.h"
#ifdef _WIN32
#include <windows.h>
#endif

void gpio_init(uint gpio) {

}
void gpio_set_dir(uint gpio, bool out) {

}
void gpio_put(uint gpio, bool value) {

}
void sleep_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(1000 * ms);
#endif
}
bool stdio_init_all(void) {
    return true;
}


#endif // SIMULATION

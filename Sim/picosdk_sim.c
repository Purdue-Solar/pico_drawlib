#ifdef SIMULATION

#include "picosdk_sim.h"

void gpio_init(uint gpio) {

}
void gpio_set_dir(uint gpio, bool out) {

}
void gpio_put(uint gpio, bool value) {

}
void sleep_ms(uint32_t ms) {
    usleep(1000 * ms);
}
bool stdio_init_all(void) {
    return true;
}


#endif // SIMULATION

#pragma once
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "pdl.hpp"

extern PDLInfo sharedInfo;
extern mutex_t info_mutex;
/* The purpose of the mutex is to indicate to the other core that the 
struct is being read or written to, and therefore cannot be read or written.
Since a cpu cannot read an entire struct at once, this eliminates the risk
of the data being out of sync with eachother.
*/
void core1_entry();
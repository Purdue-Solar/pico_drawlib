#pragma once

#include <stdbool.h>

#include <pico/stdlib.h>

void myassert(bool val);
void myassert_checkpoint(bool val, int n_blink);

#ifndef DISPLAY
#define DISPLAY

#define RGB(r, g, b) (((r) << 11) | ((g) << 5) | (b))

#define COLOR_BLACK create_color(0, 0, 0)
#define COLOR_WHITE create_color(255, 255, 255)
#define COLOR_RED create_color(255, 0, 0)
#define COLOR_GREEN create_color(0, 255, 0)
#define COLOR_BLUE create_color(0, 0, 255)
#define COLOR_YELLOW create_color(255, 255, 0)
#define COLOR_PURPLE create_color(255, 0, 255)
#define COLOR_CYAN create_color(0, 255, 255)

#endif

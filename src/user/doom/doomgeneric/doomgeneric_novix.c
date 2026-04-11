#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <_syscall.h>
#include "doomkeys.h"
#include "doomgeneric.h"

#define CAPS_LOCK 0
#define NUM_LOCK 1
#define SHIFTED 2
#define PRESSED 3   // pressed or released

typedef struct video_info
{
    uint16_t pitch;			    // number of bytes per horizontal line
	uint16_t width;			    // width in pixels
    uint16_t height;			// height in pixels
    uint8_t bpp;			    // bits per pixel in this mode
    uint8_t bytes_per_pixel;

    uint8_t memory_model;

    uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;

    uint32_t framebuffer;		    // physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
}__attribute__((packed)) video_info_t;

typedef struct surface
{
    int x, y;
    uint16_t width;
    uint16_t height;
    void* pixels;
}__attribute__((packed)) surface_t;

typedef struct key_event
{
    uint8_t code;
    uint8_t modifier_mask;
}__attribute__((packed)) key_event_t;

uint32_t ticks = 0;
int keyboard;
int framebuffer;
key_event_t event = {0};
surface_t surface;
video_info_t vid_info;

void DG_Init()
{
    keyboard = open("/dev/keyboard", O_RDONLY | O_NONBLOCK, 0);
    framebuffer = open("/dev/fb", O_WRONLY, 0);

    if(keyboard < 0 || framebuffer < 0) exit(-1);

    __sys_ioctl(framebuffer, 0, &vid_info);

    printf("framebuffer: red_off: %d, green_off: %d, blue_off: %d, bpp: %d\n", vid_info.red_position, vid_info.green_position, vid_info.blue_position, vid_info.bytes_per_pixel);

    surface.x = 0;
    surface.y = 0;
    surface.width = DOOMGENERIC_RESX;
    surface.height = DOOMGENERIC_RESY;
    surface.pixels = (void*)DG_ScreenBuffer;
}

void DG_DrawFrame()
{
    // blit the surface
    __sys_ioctl(framebuffer, 1, &surface);
}

void DG_SleepMs(uint32_t ms)
{
    // __sys_sleep(ms);
}

uint32_t DG_GetTicksMs()
{
    return ticks++;
}

int to_doom_key(uint8_t code)
{
    switch (code) {
    case 0x1C:      return KEY_ENTER;
    case 0x01:      return KEY_ESCAPE;
    case 0x1E:      return KEY_LEFTARROW;
    case 0x20:      return KEY_RIGHTARROW;
    case 0x11:      return KEY_UPARROW;
    case 0x1F:      return KEY_DOWNARROW;
    case 0x39:      return KEY_FIRE; // space
    case 0x1D:      return KEY_USE; // control
    case 0x36:
    case 0x2A:      return KEY_RSHIFT;
    default:        return 'a';
    }
}

int DG_GetKey(int* pressed, unsigned char* key)
{
    if(read(keyboard, &event, 1) < 0) return 0;

    *pressed = event.modifier_mask & (1 << PRESSED);
    *key = KEY_RIGHTARROW;//to_doom_key(event.code);

    return 1;
}

void DG_SetWindowTitle(const char * title)
{
    printf("----set window\n");
}

int main()
{
    doomgeneric_Create(0, (char*[]){"/", NULL});

    while (1)
    {
        doomgeneric_Tick();
    }
    
    return 0;
}
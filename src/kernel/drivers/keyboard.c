/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <debug.h>
#include <string.h>
#include <memory.h>
#include <hal/irq.h>
#include <hal/pic.h>
#include <hal/io.h>
#include <mem_manager/heap.h>
#include <multitasking/lock.h>
#include <multitasking/scheduler.h>
#include <drivers/keyboard.h>
#include <drivers/device.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

typedef enum {
    KYBRD_ENC_OUTPUT_BUF    = 0x60,
    KYBRD_ENC_CMD_REG       = 0x60,
}KYBRD_ENCODER_IO;

typedef enum {
    KYBRD_CTRL_STATUS_REG   = 0x64,
    KYBRD_CTRL_CMD_REG      = 0x64,
}KYBRD_CTRL_IO;

typedef enum {
    KYBRD_CTRL_STATUS_OUT_BUF   = 0x01, //00000001
    KYBRD_CTRL_STATUS_IN_BUF    = 0x02, //00000010
    KYBRD_CTRL_STATUS_SYSTEM    = 0x04, //00000100
    KYBRD_CTRL_STATUS_CMD_DATA  = 0x08, //00001000
    KYBRD_CTRL_STATUS_LOCKED    = 0x10, //00010000
    KYBRD_CTRL_STATUS_AUX_BUF   = 0x20, //00100000
    KYBRD_CTRL_STATUS_TIMEOUT   = 0x40, //01000000
    KYBRD_CTRL_STATUS_PARITY    = 0x80, //10000000
}KYBRD_CTRL_STATUS;

typedef enum {
    SCANCODE_SET_1  = 0x01,
    SCANCODE_SET_2  = 0x02,
    SCANCODE_SET_3  = 0x03,
}KYBRD_SCANCODE_SET;

typedef enum{
    TYPEMATIC_DELAY_250     = 0x0,
    TYPEMATIC_DELAY_500     = 0x1,
    TYPEMATIC_DELAY_750     = 0x2,
    TYPEMATIC_DELAY_1000    = 0x3,
}TYPEMATIC_DELAY;

typedef enum{
    TYPEMATIC_RATE_30PER_SEC    = 0X00,
    TYPEMATIC_RATE_26PER_SEC    = 0x02,
    TYPEMATIC_RATE_24PER_SEC    = 0x02,
    TYPEMATIC_RATE_10PER_SEC    = 0x0F,
    TYPEMATIC_RATE_2PER_SEC     = 0x1F,
}TYPEMATIC_RATE;

#define MAX_KEYBOARD_BUFFER 256
#define CAPS_LOCK 0
#define NUM_LOCK 1
#define SHIFTED 2
#define PRESSED 3   // pressed or released


//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

bool g_keyboard_stateDisabled = false;
bool g_shiftPressed = false;
bool g_capsLockOn = false;
bool g_numLockOn = false;
bool g_scrollOn = false;
bool g_extended = false;

key_event_t this_keyboard[MAX_KEYBOARD_BUFFER];
uint8_t buffer_count;
mutex_t* keyboard_lock;

process_t* first_waiting_kbd;
process_t* last_waiting_kbd;

static char asciiTable[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

static char shiftTable[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-',
    '4', '5', '6', '+', '1', '2', '3', '0', '.'
};

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================

uint8_t KEYBOARD_readStatusReg();
void KEYBOARD_sendCmd(uint8_t port, uint8_t cmd);
uint8_t KEYBOARD_readOutputBuffer();
void KEYBOARD_updateLed(bool num, bool capslock, bool scroll);
void KEYBOARD_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet);
void KEYBOARD_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate);
bool KEYBOARD_selfTest();
bool KEYBOARD_interfaceTest();
void KEYBOARD_interruptHandler(Registers* regs);

//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================

uint8_t KEYBOARD_readStatusReg()
{
    return inb(KYBRD_CTRL_STATUS_REG);
}

void KEYBOARD_sendCmd(uint8_t port, uint8_t cmd)
{
    while(1)
        if(!(KEYBOARD_readStatusReg() & KYBRD_CTRL_STATUS_IN_BUF))
            break;
    outb(port, cmd);
}

uint8_t KEYBOARD_readOutputBuffer()
{
    while(1)
        if(KEYBOARD_readStatusReg() & KYBRD_CTRL_STATUS_OUT_BUF)
            break;
    return inb(KYBRD_ENC_OUTPUT_BUF);
}

void KEYBOARD_updateLed(bool num, bool capslock, bool scroll)
{
    uint8_t data = 0;

    // set or clear the bit
    data = (scroll) ? (data | 1) : (data & 1);
    data = (num) ? (data | 2) : (data & 2);
    data = (capslock) ? (data | 4) : (data & 4);

    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xED);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, data);
}

void KEYBOARD_updateScanCodeSet(KYBRD_SCANCODE_SET scanCodeSet)
{
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xF0);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, scanCodeSet);
}

void KEYBOARD_setTypematicMode(TYPEMATIC_DELAY delay, TYPEMATIC_RATE rate)
{
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, 0xF3);
    KEYBOARD_sendCmd(KYBRD_ENC_CMD_REG, rate | delay << 5);
}


bool KEYBOARD_selfTest()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAA);
    if(KEYBOARD_readOutputBuffer() == 0x55)
        return true;
    else
        return false;
}

bool KEYBOARD_interfaceTest()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAB);
    if(KEYBOARD_readOutputBuffer() == 0)
        return true;
    else
        return false;
}

void KEYBOARD_interruptHandler(Registers* regs)
{
    // send EOI as soon as possible
    PIC_sendEndOfInterrupt(1);

    uint8_t scancode = KEYBOARD_readOutputBuffer();
    bool is_pressed;

    if (scancode == 0xE0)
    {
        g_extended = true; // the next key is extended
        goto End;
    }

    if (scancode == 0xE1)
    {
        // handle pause key ...
        goto End;
    }

    if (scancode >= 0x80)
    {
        // BREAK CODE (key released)
        is_pressed = false;
        
        switch (scancode)
        {
        case LSHIFT_RELEASED:
            g_shiftPressed = false;
            goto End;   // because we don't want to register this as a keypress
        
        case RSHIFT_RELEASED:
            g_shiftPressed = false;
            goto End;
        case CAPSLOCK_RELEASED: goto End;
        
        default:
            break;
        }
    }
    else
    {
        // MAKE CODE (key pressed)
        is_pressed = true;
        if (g_extended)
        {
            // handling special keys like arrows etc.
            g_extended = false;
            goto End;
        }
        else
        {
            switch (scancode)
            {
            case CAPSLOCK_PRESSED:
                g_capsLockOn ^= true;
                KEYBOARD_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
                goto End;   // same here we don't want to register this as a key
            case NUMLOCK_PRESSED:
                g_numLockOn ^= true;
                KEYBOARD_updateLed(g_numLockOn, g_capsLockOn, g_scrollOn);
                goto End;

            case LSHIFT_PRESSED:
                g_shiftPressed = true;
                goto End;
            
            case RSHIFT_PRESSED:
                g_shiftPressed = true;
                goto End;
            
            default:
                break;
            }
        }
    }

    // ?? I'm crazy !! acquiring a mutex inside an interrupt...
    acquire_mutex(keyboard_lock);

    uint8_t status = (g_capsLockOn ? 1 : 0) << CAPS_LOCK | (g_numLockOn ? 1 : 0) << NUM_LOCK | (g_shiftPressed ? 1 : 0) << SHIFTED | (is_pressed ? 1 : 0) << PRESSED;

    // register the scancode and get rid of bit 7 if necessary
    this_keyboard[buffer_count++] = (key_event_t){(is_pressed ? scancode : scancode & 0x7F), status}; // dont worry it's circular buffer so when count exceed 256 it will reset at 0

    if(first_waiting_kbd)
    {
        process_t* released = first_waiting_kbd;
        first_waiting_kbd = first_waiting_kbd->next;
        unblock_task(released, false);
    }

    release_mutex(keyboard_lock);

End:
    // log_debug("Keyboard handler", "Fired");
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void KEYBOARD_disable()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAD);
    g_keyboard_stateDisabled = true;
}

void KEYBOARD_enable()
{
    KEYBOARD_sendCmd(KYBRD_CTRL_CMD_REG, 0xAE);
    g_keyboard_stateDisabled = false;
}

static int64_t read(uint8_t* buffer, int64_t offset , size_t len, void* priv, uint32_t flags);
static int64_t write(const uint8_t *buffer, int64_t offset, size_t len, void* priv, uint32_t flags);

void KEYBOARD_initialize()
{
    disableInterrupts();
    KEYBOARD_enable(); // just in case !

    if(!KEYBOARD_selfTest() || !KEYBOARD_interfaceTest())
    {
        KEYBOARD_disable();
        return;
    }

    // KEYBOARD_updateScanCodeSet(SCANCODE_SET_2); // apparently the 8082 chip translates automatically to scancode set 1
    KEYBOARD_setTypematicMode(TYPEMATIC_DELAY_1000, TYPEMATIC_RATE_2PER_SEC);

    first_waiting_kbd = NULL;
    last_waiting_kbd = NULL;

    buffer_count = 0;
    memset(this_keyboard, 0, MAX_KEYBOARD_BUFFER);
    keyboard_lock = create_mutex();

    device_t* new = kmalloc(sizeof(device_t));
    strcpy(new->name, "keyboard");
    new->priv = NULL;
    new->read = read;
    new->write = write;
    new->ioctl = NULL;

    add_device(new);
    
    disableInterrupts();
    IRQ_registerNewHandler(1, (IRQHandler)KEYBOARD_interruptHandler);
    enableInterrupts();
}

char KEYBOARD_scanToAscii(key_event_t* key)
{
    if (key->code > 127) 
        return NULL_KEY;

    if((key->modifier_mask & (1 << SHIFTED))  && (key->modifier_mask & (1 << CAPS_LOCK)))
        return asciiTable[key->code];
    if ((key->modifier_mask & (1 << SHIFTED))  || (key->modifier_mask & (1 << CAPS_LOCK)))
        return shiftTable[key->code];
    else
        return asciiTable[key->code];
}

int64_t read(uint8_t* buffer, int64_t offset , size_t len, void* priv, uint32_t flags)
{
    size_t toread = len;
    bool isFulfilled = false;

    do
    {
        acquire_mutex(keyboard_lock);

        if(buffer_count <= 0)
        {
            release_mutex(keyboard_lock);

            if(flags & VFS_O_NONBLOCK)
                return VFS_EAGAIN;

            lock_scheduler();
            if(first_waiting_kbd == NULL)
            {
                first_waiting_kbd = PROCESS_getCurrent();
                last_waiting_kbd = first_waiting_kbd;
            }
            else{
                last_waiting_kbd->next = PROCESS_getCurrent();
                last_waiting_kbd = last_waiting_kbd->next;
            }

            last_waiting_kbd->next = NULL;
            last_waiting_kbd->state = WAITING;
            unlock_scheduler();

            block_task();

            continue;
        }

        if(toread > buffer_count)
            toread = buffer_count;

        memcpy(buffer, this_keyboard, sizeof(key_event_t) * toread);

        if(toread)
        {
            for(int i = 0; i < toread && i < MAX_KEYBOARD_BUFFER - 1; i++)
                this_keyboard[i] = this_keyboard[i+1];
        }

        buffer_count -= toread;
        isFulfilled = true;

        release_mutex(keyboard_lock);

    }while (!isFulfilled);
    

    return toread;
}

int64_t write(const uint8_t *buffer, int64_t offset, size_t len, void* priv, uint32_t flags)
{
    // write to keyboard ?
}
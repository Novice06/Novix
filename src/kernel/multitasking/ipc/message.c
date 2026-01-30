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

#include <memory.h>
#include <hal/io.h>
#include <debug.h>
#include <mem_manager/heap.h>
#include <multitasking/scheduler.h>
#include <multitasking/process.h>
#include <multitasking/lock.h>
#include <multitasking/time.h>
#include <multitasking/ipc/message.h>

#define MAX_MESSAGES 256

typedef struct message
{
    uint32_t sender_id;
    uint32_t receiver_id;
    size_t size;
    uint8_t data[MAX_MESSAGE_SIZE];
}message_t;

struct endpoint
{
    bool is_open;
    int count;
    message_t* messages[MAX_MESSAGES];
    process_t* first_waiting;
    process_t* last_waiting;
    mutex_t* msg_lock;
};

struct endpoint* endpoints[MAX_PROCESS];

int send_msg(uint32_t receiver_id, void* data, size_t size)
{
    message_t* new = kmalloc(sizeof(message_t));
    do
    {
        if(endpoints[receiver_id] == NULL || !endpoints[receiver_id]->is_open)
        {
            kfree(new);
            return -1;
        }

        acquire_mutex(endpoints[receiver_id]->msg_lock);

        if(endpoints[receiver_id]->count >= MAX_MESSAGES)
        {
            if(endpoints[receiver_id]->first_waiting == NULL)
            {
                endpoints[receiver_id]->first_waiting = PROCESS_getCurrent();
                endpoints[receiver_id]->last_waiting = PROCESS_getCurrent();
                endpoints[receiver_id]->last_waiting->next = NULL;
            }
            else
            {
                endpoints[receiver_id]->last_waiting->next = PROCESS_getCurrent();
                endpoints[receiver_id]->last_waiting = PROCESS_getCurrent();
                endpoints[receiver_id]->last_waiting->next = NULL;
            }

            lock_scheduler();
            PROCESS_getCurrent()->state = WAITING;
            release_mutex(endpoints[receiver_id]->msg_lock);
            unlock_scheduler();

            block_task();
            continue;
        }

        
        new->sender_id = PROCESS_getCurrent()->id;
        new->receiver_id = receiver_id;

        if(size > MAX_MESSAGE_SIZE)
            size = MAX_MESSAGE_SIZE;
        
        new->size = size;
        memcpy(new->data, data, new->size);

        uint32_t index = endpoints[receiver_id]->count++;
        endpoints[receiver_id]->messages[index] = new;

        release_mutex(endpoints[receiver_id]->msg_lock);
        break;  // break the loop

    } while (1);
    
    lock_scheduler();
    if(PROCESS_get(receiver_id)->state == BLOCKED)
        unblock_task(PROCESS_get(receiver_id), false);

    unlock_scheduler();

    return 0;
}

bool receive_async_msg(void* dataOut, size_t *size)
{
    uint32_t index = PROCESS_getCurrent()->id;

    if(endpoints[index]->count <= 0)
        return false;

    acquire_mutex(endpoints[index]->msg_lock);

    message_t* received_msg = endpoints[index]->messages[0];
    endpoints[index]->count--;

    for(int i = 0; i < endpoints[index]->count; i++)    // shifting all the remaining messages to left
        endpoints[index]->messages[i] = endpoints[index]->messages[i+1];
    
    endpoints[index]->messages[endpoints[index]->count] = NULL;

    if(endpoints[index]->first_waiting)
    {
        process_t* waiting_proc = endpoints[index]->first_waiting;

        endpoints[index]->first_waiting = endpoints[index]->first_waiting->next;
        if(endpoints[index]->first_waiting == NULL)
            endpoints[index]->last_waiting = NULL;

        unblock_task(waiting_proc, false);
    }

    release_mutex(endpoints[index]->msg_lock);

    if(dataOut != NULL)
    {
        if(size != NULL)
            *size = received_msg->size;
        
        memcpy(dataOut, received_msg->data, received_msg->size);
    }

    kfree(received_msg);
    return true;
}

void receive_msg(void* dataOut, size_t *size)
{
    while (!receive_async_msg(dataOut, size))
    {
        PROCESS_getCurrent()->state = BLOCKED;
        block_task();
    }
}

void open_inbox()
{
    if(endpoints[PROCESS_getCurrent()->id] != NULL)
    {
        endpoints[PROCESS_getCurrent()->id]->is_open = true;
        return;
    }

    endpoints[PROCESS_getCurrent()->id] = kmalloc(sizeof(struct endpoint));
    endpoints[PROCESS_getCurrent()->id]->is_open = true;
    endpoints[PROCESS_getCurrent()->id]->count = 0;
    endpoints[PROCESS_getCurrent()->id]->msg_lock = create_mutex();
    endpoints[PROCESS_getCurrent()->id]->first_waiting = NULL;
    endpoints[PROCESS_getCurrent()->id]->last_waiting = NULL;
}

void close_inbox()
{
    if(endpoints[PROCESS_getCurrent()->id] != NULL)
        endpoints[PROCESS_getCurrent()->id]->is_open = false;
}

void destroy_endpoint()
{
    if(endpoints[PROCESS_getCurrent()->id] == NULL)
        return;

    uint32_t index = PROCESS_getCurrent()->id;
    close_inbox();  // first we close the endpoint

    acquire_mutex(endpoints[index]->msg_lock);  // we make sure no body is using this endpoints
    while (receive_async_msg(NULL, NULL));  // read all remaining messages (and also to unlock all waiting processes)

    destroy_mutex(endpoints[index]->msg_lock);

    kfree(endpoints[index]);
    endpoints[index] = NULL;
}
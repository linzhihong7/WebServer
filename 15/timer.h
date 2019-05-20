#ifndef TIMER_H
#define TIMER_H
#include "heap_timer.h"
#include "http_conn.h"
#define EXPIRED_TIME  15
int  timerheap_init();
void timerheap_free();
void th_add_timer(http_conn* request, size_t time_out);
void th_delete_timer(http_conn* request);
void th_update_timer(http_conn* request);
void th_timer_handler();
#endif

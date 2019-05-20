#include "timer.h"
heap_timer_t<http_conn> *timersheap;
int  timerheap_init()
{
  timersheap = new heap_timer_t<http_conn>(TIME_HEAP_SIZE);
  if(timersheap == NULL )
    return -1;
  else {
    return 0;
  }
}
void timerheap_free()
{
  delete[] timersheap;
}
void th_add_timer(http_conn* request, size_t time_out)
{
  heap_timer<http_conn>* new_timer = new heap_timer<http_conn>();
  new_timer->data = request;
  time_t cur = time( NULL );
  new_timer->expire = cur + EXPIRED_TIME;
  request->timer = new_timer;
  timersheap->add_timer( new_timer );
}
void th_delete_timer(http_conn* request)
{
  timersheap->del_timer(request->timer);
}
void th_update_timer(http_conn* request)
{
  time_t cur = time( NULL );
  time_t newTime = cur + EXPIRED_TIME;
  timersheap->update_timer(request->timer, newTime);
}
void th_timer_handler()
{
  timersheap->tick();
  alarm( EXPIRED_TIME );
}

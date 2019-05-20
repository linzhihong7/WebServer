#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "heap_timer.h"
#include <iostream>
#define  BUFSIZE 1024
#define TIME_HEAP_SIZE 65536
template < typename client_data >
class heap_timer {
public:
  heap_timer(){ }
  bool operator<( const heap_timer &rhs )
  {
    return (*this).expire < rhs.expire;
  }
  bool operator>( const heap_timer &rhs )
  {
    return  !(*this < rhs);
  }
public:
  time_t expire;
  client_data *data;
};
template < typename clientdata >
class  heap_timer_t {
public:
  heap_timer_t( int cap ) : capacity(cap), count(0)
  {
    data = new heap_timer<clientdata>*[cap+1];
    if( !data )
    {
      throw std::exception();
    }
    for( int i = 0; i <= capacity; ++i )
    {
      data[i] = NULL;
    }
  }
  ~heap_timer_t()
  {
    for( int i = 0; i <= capacity; ++ i )
      delete data[i];
    delete [] data;
  }
  void add_timer( heap_timer<clientdata> *timer )
  {
    if( count >= capacity )
      adjust_size();
    data[count+1] = timer;
    count++;
    shiftUp( count ) ;
  }
  void del_timer( heap_timer<clientdata> *timer )
  {
    if( timer == NULL )
      return;
    int i;
    for( i = 1; i <= count; ++i )
      if( timer == data[i] )
        break;
    if( i > count ) {
      std::cout << "The node dont exist in this heap" << std::endl;
      return;
    }
    data[i] = data[count];
    count--;
    shiftUp( i ) ;
    shifDown( i );
  }
  void pop_timer()
  {
    if( count == 0 )
      return;
    if( data[1] ){
      delete data[1];
      data[1] = data[count];
      count--;
      shifDown(1);
    }
  }
  void update_timer( heap_timer<clientdata> *node, time_t newTime )
  {
    int i;
    for( i = 1; i <= count; ++i )
    {
      if( data[i] == node )
        break;
    }
    if( i > count ) {
      std::cout << "The node dont exist in this heap" << std::endl;
      return;
    }
    data[i]->expire = newTime;
    shiftUp( i ) ;
    shifDown( i );
  }
  void tick()
  {
    heap_timer<clientdata> *timer = data[1];
    time_t cur = time(NULL);
    while( count > 0 ){
      if( !timer ) {
        break;
      }
      printf("timer->expre:  %d, cur: %d  \n", timer->expire,cur);
      if( timer->expire > cur )
      {
        break;
      }
      printf("close client %d\n",timer->data->m_sockfd );
      timer->data->close_conn();
      pop_timer();
      timer = data[1];
    }
  }
  heap_timer<clientdata>* extractMin()
  {
    if( count > 0 )
      return data[1];
    return NULL;
  }
  bool empty()
  {
    return count == 0;
  }
private:
  void shiftUp( int i )
  {
    while( i > 1 && data[i]->expire < data[i/2]->expire ) {
      std::swap( data[i], data[i/2] );
      i /= 2;
    }

  }
  void shifDown( int i )
  {
    while( 2*i <= count && data[i]->expire > data[2*i]->expire ) {
      std::swap( data[i], data[2*i] );
      i *= 2;
    }
  }
  void adjust_size() {
  }
private:
  heap_timer<clientdata> **data;
  int count;
  int capacity;
};
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool.h"
#include "http_conn.h"
#include "timer.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
#define TIME_SET  5


int time_out = false;
int  sig_pipefd[2];
//extern heap_timer_t<http_conn> *timersheap;

extern int addfd( int epollfd, int fd, bool one_shot );
extern int removefd( int epollfd, int fd );
void  sig_handler(int  sig)
{
  int  save_errno = errno;
  int   msg = sig;
  send(sig_pipefd[1],(char*)&msg,1,0);
  errno = save_errno;
}
void sigHandler( int pipefd ,bool &stop_server )
{
  char  signals[BUFSIZE];
  int  sig;
  int ret = recv( pipefd, signals, sizeof(signals), 0 );
  if( ret == -1 || ret == 0) {
    return;
  }
  else {
    for( int i = 0; i < ret; ++i ) {
      sig = static_cast<int>( signals[i] );
      switch ( sig ) {
        case SIGALRM : time_out = true;break;
        case SIGINT : stop_server = true; break;
      }
    }
  }
}
void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

void show_error( int connfd, const char* info )
{
    printf( "%s", info );
    send( connfd, info, strlen( info ), 0 );
    close( connfd );
}
int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    addsig( SIGPIPE, SIG_IGN );

    threadpool_t* pool = NULL;
    try
    {
        pool = new threadpool_t();
    }
    catch( ... )
    {
        return 1;
    }

    http_conn* users = new http_conn[ MAX_FD ];
    assert( users );
    int user_count = 0;

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    struct linger tmp = { 1, 0 };
    setsockopt( listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret >= 0 );

    ret = listen( listenfd, 5 );
    assert( ret >= 0 );

    epoll_event events[ MAX_EVENT_NUMBER ];
    int epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
    addfd( epollfd, listenfd, false );
    addsig( SIGALRM, sig_handler );
    socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );
    addfd( epollfd, sig_pipefd[0], false );
    http_conn::m_epollfd = epollfd;
    timerheap_init();
    bool stop_server = false;
    alarm( TIME_SET );

    while( !stop_server )
    {
        int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, -1 );
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }

        for ( int i = 0; i < number; i++ )
        {
            int sockfd = events[i].data.fd;
            if( sockfd == listenfd )
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                if ( connfd < 0 )
                {
                    printf( "errno is: %d\n", errno );
                    continue;
                }
                if( http_conn::m_user_count >= MAX_FD )
                {
                    show_error( connfd, "Internal server busy" );
                    continue;
                }
              /*  time_t cur = time( NULL );
                heap_timer<http_conn> *new_timer = new heap_timer<http_conn>();
                new_timer->data = &( users[connfd] );
                new_timer->expire = cur + 3*TIME_SET;
                users[connfd].timer = new_timer;
                timeheap->add_timer( new_timer );
                printf("new clien %d,Timer-expire: %d\n",connfd, users[connfd].timer->expire);
                heap_timer<http_conn>* minNode = timeheap->extractMin();
                printf("TimeHeap-expire:%d\n",minNode->expire);*/
                users[connfd].init( connfd, client_address );
                th_add_timer(users + connfd, EXPIRED_TIME);
            }
            else if( sockfd == sig_pipefd[0] )
            {
              sigHandler( sockfd, stop_server );
            }
            else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
            {
              //  timeheap->del_timer( users[sockfd].timer );
                th_delete_timer(users + sockfd);
                users[sockfd].close_conn();
            }
            else if( events[i].events & EPOLLIN )
            {
              th_update_timer(users + sockfd);
              pool->append( do_request, users + sockfd );
          /*    heap_timer<http_conn>* timer = users[sockfd].timer;
              time_t cur = time( NULL );
              timer->expire = cur + 3*TIME_SET;
              timeheap->update_timer( users[sockfd].timer, timer->expire);*/


              /*  else
                {
                    timeheap->del_timer( users[sockfd].timer );
                    users[sockfd].close_conn();
                }*/
            }
          /*  else if( events[i].events & EPOLLOUT )
            {
                if( !users[sockfd].write() )
                {
                    timeheap->del_timer( users[sockfd].timer );
                    users[sockfd].close_conn();
                }
            }*/
            else
            {}
        }
        if( time_out )
        {
          printf("client time out\n");
          th_timer_handler();
          time_out = false;
        }
    }
    close( epollfd );
    close( listenfd );
    timerheap_free();
    delete[] users;
    delete pool;
    return 0;
}

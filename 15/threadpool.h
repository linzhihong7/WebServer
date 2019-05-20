#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

typedef struct task {
  void (*func)(void*);
  void *arg;
}task_t;
typedef class threadpool
{
public:
    threadpool( int thread_number = 8, int max_requests = 10000 );
    ~threadpool();
    bool append(void (*func)(void*), void *arg);
private:
    static void* worker( void* arg );
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    pthread_t* m_threads;
    std::list< task_t* > m_workqueue;
    pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;
    bool m_stop;
}threadpool_t;

threadpool_t::threadpool( int thread_number, int max_requests ) :
        m_thread_number( thread_number ), m_max_requests( max_requests ), m_stop( false ), m_threads( NULL )
{
    if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
    {
        throw std::exception();
    }
    //initialize cond and mutex
    if( pthread_mutex_init( &m_mutex, NULL ) != 0 )
    {
      throw std::exception();
    }
    if ( pthread_cond_init( &m_cond, NULL ) != 0 )
    {
      pthread_mutex_destroy( &m_mutex );
      throw std::exception();
    }
    m_threads = new pthread_t[ m_thread_number ];
    if( ! m_threads )
    {
        throw std::exception();
    }

    for ( int i = 0; i < thread_number; ++i )
    {
        printf( "create the %dth thread\n", i );
        if( pthread_create( m_threads + i, NULL, worker, this ) != 0 )
        {
            delete [] m_threads;
            throw std::exception();
        }
        if( pthread_detach( m_threads[i] ) )
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

threadpool_t::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

bool threadpool_t::append(void (*func)(void*), void *arg)
{
    pthread_mutex_lock( &m_mutex );
    if ( m_workqueue.size() > m_max_requests )
    {
        pthread_mutex_unlock( &m_mutex );
        return false;
    }
    task_t* new_task = new task_t();
    new_task->func = func;
    new_task->arg = arg;
    m_workqueue.push_back( new_task );
    pthread_mutex_unlock( &m_mutex );
    pthread_cond_signal( &m_cond );
    return true;
}

void* threadpool_t::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

void threadpool_t::run()
{
    while ( ! m_stop )
    {
        pthread_mutex_lock( &m_mutex );
        pthread_cond_wait( &m_cond, &m_mutex );
        if ( m_workqueue.empty() )
        {
            pthread_mutex_unlock( &m_mutex );
            continue;
        }
        task_t* task = m_workqueue.front();
        m_workqueue.pop_front();
        pthread_mutex_unlock( &m_mutex );
        if ( !task )
        {
            continue;
        }
        (*(task->func))(task->arg);
        delete task;
    }
}

#endif

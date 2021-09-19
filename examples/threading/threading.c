#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
 
     	struct thread_data* thread_func_args = (struct thread_data *) thread_param; // obtain arguments from parameter thread_param
       
	usleep(thread_func_args->wait_to_obtain_ms); // wait for ms

	int rc;
        rc = pthread_mutex_lock(thread_func_args->mutex); // lock mutex
	if(rc != 0)
	{
		perror("Mutex lock failed\n");
		thread_func_args->thread_complete_success = false;
		return thread_param;
	}

	usleep(thread_func_args->wait_to_release_ms); // release for ms

	rc = pthread_mutex_unlock(thread_func_args->mutex); // unlock mutex
	if(rc != 0)
	{
		perror("Mutex unlock failed\n");
                thread_func_args->thread_complete_success = false;
		return thread_param;
	}

        thread_func_args->thread_complete_success = true;
        DEBUG_LOG("Thread completed\n");
	return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     struct thread_data *data = (struct thread_data*)malloc(sizeof(struct thread_data)); // allocate memory 
    
     // fill the structure with input parameters to the function
     data->wait_to_obtain_ms = wait_to_obtain_ms;
     data->wait_to_release_ms = wait_to_release_ms;
     data->mutex = mutex;

     pthread_t thread1;
     int rc;

     rc = pthread_create(&thread1, NULL, &threadfunc, data); // create pthread

     if(rc != 0)
     {
	     perror("Error while creating thread\n");
	     return false;
     }
     
     DEBUG_LOG("Thread Created\n");
     *thread = thread1;
     return true;

}


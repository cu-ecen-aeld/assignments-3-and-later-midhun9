#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "queue.h"
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>


#define PORT "9000"
#define MAX_CONNECTIONS 9
#define BUFFER_SIZE 500
#define TIMER_BUFFER_SIZE 100
#define FILE_PATH "/var/tmp/aesdsocketdata"


int socket_fd, client_fd, write_fd;
int stop = 0;



/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
typedef struct thread_data{

    /*
     * TODO: add other values your thread will need to manage
     * into this structure, use this structure to communicate
     * between the start_thread_obtaining_mutex function and
     * your thread implementation.
     */
	pthread_t thread_index;
	int client_fd;
	char *buffer;
    char *transmit_buffer;
	sigset_t signal_mask;

    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    	bool thread_complete_success;
}thread_data_t;

pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
// SLIST.
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    thread_data_t thread_parameters;
    SLIST_ENTRY(slist_data_s) entries;
};

slist_data_t *datap=NULL;

SLIST_HEAD(slisthead, slist_data_s) head;

        


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    	if (sa->sa_family == AF_INET) {
        	return &(((struct sockaddr_in*)sa)->sin_addr);
    	}

    	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void close_all()
{
	
	stop = 1;
	close(socket_fd);
	close(write_fd);
   	closelog();
   	remove(FILE_PATH);
    
	//cancel threads which are not completed and free associated pointers
   	SLIST_FOREACH(datap,&head,entries)
    	{
        	if (datap->thread_parameters.thread_complete_success != true)
		{
 		        pthread_cancel(datap->thread_parameters.thread_index);
            		free(datap->thread_parameters.buffer);
            		free(datap->thread_parameters.transmit_buffer);
       		}
        }
    
    	//free linked list
    	while(!SLIST_EMPTY(&head))
    	{
        	datap = SLIST_FIRST(&head);
        	SLIST_REMOVE_HEAD(&head,entries);
        	free(datap);
    	}
}

/* handler for SIGINT and SIGTERM */
static void signal_handler (int signo)
{
        if (signo == SIGINT || signo == SIGTERM)
	{
		syslog(LOG_DEBUG, "Caught Signal, exiting");
		shutdown(socket_fd,SHUT_RDWR);
		stop = 1;
		close_all();
		exit (EXIT_SUCCESS);
	}
	
        else
       	{
                /* this should never happen */
                printf ("Unexpected signal!\n");
                exit (EXIT_FAILURE);
        }
	
        
}

void receive_send_data(void* thread_param)
{
	
	thread_data_t *thread_func_args = (thread_data_t*)thread_param;

	int received_bytes, write_bytes, read_bytes, send_bytes;
	int loc = 0;
	int count = 1;

	// allocate memory for buffers
        thread_func_args->buffer = (char *)malloc(BUFFER_SIZE*sizeof(char));
        if(thread_func_args->buffer == NULL)
        {
                printf("Error while doing malloc\n");
                close_all();
                exit (EXIT_FAILURE);
        }

        thread_func_args->transmit_buffer = (char *)malloc(BUFFER_SIZE*sizeof(char));
        if(thread_func_args->transmit_buffer == NULL)
        {
                printf("Error while doing malloc\n");
                close_all();
                exit (EXIT_FAILURE);
        }



	//receive data
	do
	{
                received_bytes = recv(thread_func_args->client_fd, thread_func_args->buffer + loc, BUFFER_SIZE, 0); //sizeof(buffer)
                if(received_bytes <= 0)
                {
                        printf("Error while receiving\n");
                }

                loc += received_bytes;

                // increase buffer size dynamically if required
                count += 1;
                thread_func_args->buffer = (char *)realloc(thread_func_args->buffer, count*BUFFER_SIZE*(sizeof(char)));
                if(thread_func_args->buffer == NULL)
                {
                        close(thread_func_args->client_fd);
                        close_all();
                        printf("Error while doing realloc\n");
                        exit (EXIT_FAILURE);
                }


	}while(strchr(thread_func_args->buffer, '\n') == NULL);
	received_bytes = loc;
	thread_func_args->buffer[received_bytes] = '\0';
	//total_bytes += received_bytes; //keep track of total bytes

	int rc;
	rc = pthread_mutex_lock(&main_mutex); //lock mutex
	if(rc != 0)
	{
		perror("Mutex lock failed\n");
		close_all();
		exit (EXIT_FAILURE);
	} 	
					
	// block the signals SIGINT and SIGTERM
	rc = sigprocmask(SIG_BLOCK, &thread_func_args->signal_mask, NULL);
	if(rc == -1)
	{
		printf("Error while blocking the signals SIGINT, SIGTERM");
		close_all();
		exit (EXIT_FAILURE);							
	}			
					
	// write to the file
	write_bytes = write(write_fd, thread_func_args->buffer, received_bytes);
	if(write_bytes < 0)
	{
		printf("Error in writing bytes\n");
		close(thread_func_args->client_fd);
		close_all();
		exit (EXIT_FAILURE);
	}
					
	rc = pthread_mutex_unlock(&main_mutex); // unlock mutex
	if(rc != 0)
	{
		perror("Mutex unlock failed\n");
		close_all();
		exit (EXIT_FAILURE);
	}
					
	// unblock the signals SIGINT and SIGTERM
	rc = sigprocmask(SIG_UNBLOCK, &thread_func_args->signal_mask, NULL);
	if(rc == -1)
	{
		printf("Error while unblocking the signals SIGINT, SIGTERM");
		close_all();
		exit (EXIT_FAILURE);
	}
					
	int size  = lseek(write_fd,0,SEEK_END);
	thread_func_args->transmit_buffer = (char *)realloc(thread_func_args->transmit_buffer, size*sizeof(char));
	if(thread_func_args->transmit_buffer == NULL)
	{
		close(thread_func_args->client_fd);
		printf("Error while doing realloc\n");
		close_all();
		exit (EXIT_FAILURE);
	}
				  
	rc = pthread_mutex_lock(&main_mutex); //lock mutex
	if(rc != 0)
	{
		perror("Mutex lock failed\n");
		close_all();
		exit (EXIT_FAILURE);
	}
					
	// block the signals SIGINT and SIGTERM
	rc = sigprocmask(SIG_BLOCK, &thread_func_args->signal_mask, NULL);
	if(rc == -1)
	{
		printf("Error while blocking the signals SIGINT, SIGTERM");
		close_all();
		exit (EXIT_FAILURE);					
	}	
										
	lseek(write_fd, 0, SEEK_SET); //set cursor to start
	read_bytes = read(write_fd, thread_func_args->transmit_buffer, size); //read bytes
	if(read_bytes < 0)
	{
		close(thread_func_args->client_fd);
		printf("Error in reading bytes\n");
		close_all();
		exit (EXIT_FAILURE);
	}

	send_bytes = send(thread_func_args->client_fd, thread_func_args->transmit_buffer, read_bytes, 0);//send bytes
	if(send_bytes < 0)
	{
		close(thread_func_args->client_fd);
		printf("Error in sending bytes\n");
		close_all();
		exit (EXIT_FAILURE);
	}			
					
	rc = pthread_mutex_unlock(&main_mutex); // unlock mutex
	if(rc != 0)
	{
		perror("Mutex unlock failed\n");
		close_all();
		exit (EXIT_FAILURE);
	}				
					
	// unblock the signals SIGINT and SIGTERM
	rc = sigprocmask(SIG_UNBLOCK, &thread_func_args->signal_mask, NULL);
	if(rc == -1)
	{
		printf("Error while unblocking the signals SIGINT, SIGTERM");
		close_all();
		exit (EXIT_FAILURE);
	}
					
	// close connection
	close(thread_func_args->client_fd);
	//syslog(LOG_DEBUG, "Closed connection from %s", ipstr);
	free(thread_func_args->buffer);
	free(thread_func_args->transmit_buffer);
	thread_func_args->thread_complete_success = true;

}

//add timespecs and store in result
static void timespec_add(const struct timespec *ts_a, const struct timespec *ts_b, struct timespec *result)
{
    	result->tv_sec = ts_a->tv_sec + ts_b->tv_sec;
    	result->tv_nsec = ts_a->tv_nsec + ts_b->tv_nsec;
    	if( result->tv_nsec > 1000000000L )
       	{
       		result->tv_nsec -= 1000000000L;
        	result->tv_sec ++;
    	}
}

//service every 10 seconds
static void timer10sec_thread(union sigval sigval)
{
    	char time_buffer[TIMER_BUFFER_SIZE];
    	time_t current_time;
 	struct tm *timer_info;
    	time(&current_time);
    	timer_info = localtime(&current_time);

    	int timer_buffer_size = strftime(time_buffer,TIMER_BUFFER_SIZE,"timestamp:%a, %d %b %Y %T %z\n",timer_info);

    	pthread_mutex_lock(&main_mutex);

    	// write to file
    	int timer_writebytes = write(write_fd,time_buffer,timer_buffer_size);
    	if(timer_writebytes == -1)
	{
        	printf("Error in writing time to file\n");
        	close_all();
        	exit(-1);
    	}

    	pthread_mutex_unlock(&main_mutex);

}

// can do with timer_handler but SIGALRM is not thread-safe (re-entrancy issues?)
//void timer_handler(int signo)
//{
	//char time_buffer[TIMER_BUFFER_SIZE];
	//time_t current_time;
        //struct tm *timer_info;
        //time(&current_time);
        //timer_info = localtime(&current_time);

        //int timer_buffer_size = strftime(time_buffer,TIMER_BUFFER_SIZE,"timestamp:%a, %d %b %Y %T %z\n",timer_info);

        //pthread_mutex_lock(&main_mutex);

        ////write to file
        //int timer_writebytes = write(write_fd,time_buffer,timer_buffer_size);
        //if(timer_writebytes == -1){
        //printf("Error in writing time to file\n");
        //close_all();
        //exit(-1);
        //}

        //pthread_mutex_unlock(&main_mutex);
//}
	

int main(int argc, char*argv[])
{
        int rc;
	int enable_daemon = 0;
	char ipstr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *res;
	struct sockaddr_storage their_addr;
        socklen_t addr_size;
	sigset_t signal_set;

	openlog("aesd-socket-log", LOG_PID, LOG_USER);
        
	int opt;
    	while ((opt = getopt(argc, argv,"d")) != -1)
       	{
       	        switch (opt)
       		{
            		case 'd':
                		enable_daemon = 1;
				printf("enabled daemon\n");
				break;
            		default:
				break;
       	        }
    	}

	// Register signal_handler as our signal handler for SIGINT
        if (signal (SIGINT, signal_handler) == SIG_ERR)
       	{
                printf("Cannot handle SIGINT!\n");
                exit (EXIT_FAILURE);
        }
        
	// Register signal_handler as our signal handler for SIGTERM
        if (signal (SIGTERM, signal_handler) == SIG_ERR)
       	{
                printf("Cannot handle SIGTERM!\n");
                exit (EXIT_FAILURE);
        }

	rc = sigemptyset(&signal_set);
	if(rc == -1)
	{
		printf("Error while doing sigemptyset()");
		return -1;
	}

	rc = sigaddset(&signal_set, SIGINT);
        if(rc == -1)
        {
                printf("Error while adding SIGINT to sigaddset()");
                return -1;
        }

	rc = sigaddset(&signal_set, SIGTERM);
        if(rc == -1)
        {
                printf("Error while adding SIGTERM to sigaddset()");
                return -1;
        }

	socket_fd = socket(PF_INET, SOCK_STREAM, 0);
        
	if(socket_fd < 0)
	{
		printf("Error while creating socket\n");
		return -1;
	}
        
	int reuse_addr = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) == -1)
       	{
       		 syslog(LOG_ERR, "setsockopt");
       		 close(socket_fd);
       		 return -1;
        }

	memset(&hints, 0 , sizeof (hints));

	// load address struct with getaddrinfo
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, PORT, &hints, &res);

	// bind socket to the port we passes in getaddrinfo()
        rc = bind(socket_fd, res->ai_addr, res->ai_addrlen);
        if(rc < 0)
	{
		printf("Error while binding the socket\n");
		return -1;
	}

	freeaddrinfo(res);

	if(enable_daemon == 1)
	{

		pid_t pid = fork();

       		if(pid < 0 )
		{
           		syslog(LOG_ERR, "Forking error\n");
		}
       		if(pid > 0)
       		{
           		 syslog(LOG_DEBUG, "Daemon created\n");
           		 exit(EXIT_SUCCESS);
       		}
       		if(pid == 0)
       		{
			pid_t sid = setsid();

           		if(sid < 0)
           		{
                		syslog(LOG_ERR, "Error in setting session id\n");
                		exit(EXIT_FAILURE);
           		}
			
			// change to root directory
           		if (chdir("/") == -1)
           		{
                		syslog(LOG_ERR, "could not change directory to root");
               	        	close(socket_fd);
               	        	exit(EXIT_FAILURE);
           	        }

			// redirect input, output and error
           	        int dev_null_fd = open("/dev/null", O_RDWR);
           	 	dup2(dev_null_fd, STDIN_FILENO);
           	 	dup2(dev_null_fd, STDOUT_FILENO);
           	 	dup2(dev_null_fd, STDERR_FILENO);

			// close all file descriptors
           	 	close(STDIN_FILENO);
           	 	close(STDOUT_FILENO);
           	 	close(STDERR_FILENO);
		}
	}
        
   	// can add timer with timer_handler but SIGALRM is not thread-safe (re-entrancy issues?)    
    	//signal(SIGALRM, timer_handler);
    	//struct itimerval timer;
   	//timer.it_value.tv_sec = 10;
    	//timer.it_value.tv_usec = 0;
    	//timer.it_interval.tv_sec = 10;
    	//timer.it_interval.tv_usec = 0;
    	//if (setitimer (ITIMER_REAL, &timer, NULL) != 0)
    	//{
        	//printf("error in setting time\n");
        	//close_all();
      
    	//}
        
    	struct sigevent sev;

    	memset(&sev,0,sizeof(struct sigevent));
    
    	// setup timer
    	sev.sigev_notify = SIGEV_THREAD;
    	sev.sigev_notify_function = timer10sec_thread;
    	timer_t timer_id;
    
    	rc = timer_create(CLOCK_MONOTONIC,&sev,&timer_id);
    	if(rc == -1) 
    	{
        	printf("Error in timer_create()\n");
        	close_all();
    	}
    	struct timespec start_time;

	rc = clock_gettime(CLOCK_MONOTONIC,&start_time);
    	if(rc == -1)
    	{
        	printf("Error in clock_gettime()\n");
        	close_all();
    	} 

    	struct itimerspec itimerspec;
    	itimerspec.it_interval.tv_sec = 10;
    	itimerspec.it_interval.tv_nsec = 0;

    	timespec_add(&start_time, &itimerspec.it_interval, &itimerspec.it_value);

	rc = timer_settime(timer_id, TIMER_ABSTIME, &itimerspec, NULL);
    	if(rc == -1 )
       	{
       		printf("Error in timer_settime()\n");
        	close_all();
   	} 

	
	// listen
        rc = listen(socket_fd, MAX_CONNECTIONS);
        
	if(rc < 0)
	{
		printf("Error while listening to socket\n");
		return -1;
	}

        addr_size = sizeof(their_addr);  
        
        // open file at FILE_PATH
        write_fd = open(FILE_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
        if(write_fd < 0)
        {
               syslog(LOG_ERR, "Error: file specified as argument couldn't be opened(write_fd < 0)");
               return -1;
        }
	
	SLIST_INIT(&head);
	
	while(!stop)
	{
		// accept an incomming connection
		client_fd = accept(socket_fd, (struct sockaddr *)&their_addr, &addr_size); 
		if(client_fd < 0)
		{
			printf("Error while accpeting incomming connection\n");
			close_all();
			return -1;
		}
        
		// convert the IP to a string and print it:
		inet_ntop(AF_INET, get_in_addr((struct sockaddr *)&their_addr), ipstr, sizeof ipstr);
		syslog(LOG_DEBUG,"Accepted connection from %s", ipstr);

		datap = (slist_data_t*)malloc(sizeof(slist_data_t));
		if(datap == NULL)
		{
			printf("Error while doing malloc\n");
                        close(socket_fd);
                        close(write_fd);
                        closelog();
                        remove(FILE_PATH);
                        return -1;
		}
		SLIST_INSERT_HEAD(&head, datap, entries);
		datap->thread_parameters.client_fd = client_fd;
		datap->thread_parameters.thread_complete_success = false;
		datap->thread_parameters.signal_mask = signal_set;
		
		rc = pthread_create(&(datap->thread_parameters.thread_index), NULL, (void *)&receive_send_data, (void *) &(datap->thread_parameters)); // create pthread
		if(rc != 0)
		{
			printf("Error while creating pthread\n");
                        close_all();
                        return -1;
		}


		SLIST_FOREACH(datap, &head, entries) 
		{
     		  	if(datap->thread_parameters.thread_complete_success == true)
			{
				pthread_join(datap->thread_parameters.thread_index, NULL); // join threads if completed
			}
    		}

	}
	
	close_all();
	
        return 0;

}

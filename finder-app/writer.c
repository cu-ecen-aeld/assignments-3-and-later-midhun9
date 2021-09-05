/**
* @file  writer.c
* @brief This is a c program file which has functions to write a string to a specified file path passed as arguments.
* 
* @author Midhun Kumar Koneru
* @date 2/9/2021
*
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

int main(int argc , char *argv[])
{
	openlog("finder-app-write", LOG_PID, LOG_USER);
	if(argc < 3 && argc > 3)
	{
		printf("Input arguments not valid\n");
		syslog(LOG_ERR, "Input arguments not valid; First Argument : Full Path to the file, Second Argument : text string which will be written in the file");
		printf("First Argument : Full Path to the file\n");
		printf("Second Argument : text string which will be written in the file\n");
		return 1;
	}

	int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

	if(fd == -1)
	{
		syslog(LOG_ERR, "return error: file specified as argument couldn't be opened(fd < 0)");
		return 1;
	}

	ssize_t nr;

	// writing string to the path specified 
	nr = write(fd, argv[2], strlen(argv[2]));
	if (nr == -1)
	{
		printf("Couldn't write string to file\n");
		syslog(LOG_ERR, "Couldn't write string to file");
	}

	printf("wrote string to file specified\n");
	syslog(LOG_DEBUG, "writing %s to %s ", argv[2], argv[1]);
	closelog();
	return 0;
}

	
	




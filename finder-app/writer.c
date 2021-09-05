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
		syslog(LOG_ERR, "Input arguments not valid");
		printf("First Argument : Full Path to the file\n");
		printf("Second Argument : text string which will be written in the file\n");
		return 1;
	}

	int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

	if(fd == -1)
	{
		printf("return error\n");
		syslog(LOG_ERR, "return error");
		return 1;
	}

	ssize_t nr;

	/* write the string in 'buf' to 'fd' */
	//nr = write (fd, buf, strlen (buf));
	nr = write(fd, argv[2], strlen(argv[2]));
	if (nr == -1)
	{
		printf("Couldn't write string to file\n");
		syslog(LOG_ERR, "Couldn't write string to file");
	}

	printf("wrote string to file specified\n");
	syslog(LOG_DEBUG, "wrote %s to %s ", argv[2], argv[1]);
	closelog();
	return 0;
}

	
	




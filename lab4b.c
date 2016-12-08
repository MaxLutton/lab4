#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <stdlib.h>


sig_atomic_t volatile run_flag = 1;
int sockfd = 0;
int STOP = 0;
int waitTime = 0;
char scale = 'F'; //by default
int frequency = 3; //by default

void do_when_interrupted(int sig)
{
	if (sig == SIGINT)
		run_flag = 0;
}

float convert_temp(int temp)
{
		int B = 4275;
		float R = 1023.0/((float)temp) - 1.0;
		R = 100000.0*R;
		float temperature=1.0/(log(R/100000.0)/B+1/298.15)-273.15;
		
		if (scale == 'F')
			temperature = (1.8 * temperature) + 32;
		
		return temperature;

}



int read_handler(char* buff, int buf_size, FILE* log)
{
	write(STDOUT_FILENO, "TO LOG: ", 8);
	if (strncmp(buff, "OFF", 3) == 0)
	{
		if (buf_size == 3) //check for extraneous characters
		{
			//run_flag = 0;
			fprintf(log, "%s\n", buff);
			printf("%s\n", buff);
		        exit(1);
                }
		else
			fprintf(log, "%s I\n", buff);
	}
	else if (strncmp(buff, "STOP", 4) == 0)
	{
		if (buf_size == 4)
		{
			STOP = 1;
			fprintf(log, "%s\n", buff);
			printf("%s\n", buff);
		}
		else
		{
			fprintf(log, "%s I\n", buff);
			printf("%s I\n", buff);
		}
	}
	else if (strncmp(buff, "START", 5) == 0)
	{
		if (buf_size == 5)
		{
			STOP = 0;
			fprintf(log, "%s\n", buff);
			printf("%s\n", buff);
		}
		else
		{
			fprintf(log, "%s I\n", buff);
			printf("%s I\n", buff);
		}
	}
	else if (strncmp(buff, "SCALE", 5) == 0)
	{
		if (buf_size == 7)
		{
			if (buff[6] == 'F')
				scale = 'F';
			else if (buff[6] == 'C')
				scale = 'C';
			fprintf(log, "%s\n", buff);
			printf("%s\n", buff);

		}
		else{
			fprintf(log, "%s I\n", buff);
                        printf("%s I\n", buff);
                    }
	}
	else if (strncmp(buff, "FREQ", 4) == 0)
	{
		if (buf_size > 9) //FREQ=3600 is max 
		{
			fprintf(log, "%s I\n", buff);
			printf("%s I\n", buff);
			return 1;
		}
		int j = 5;
		int count = 0;
		int digits = buf_size - 5;
		char* freq_buff = malloc(sizeof(char)*digits);
		memset(freq_buff, 0, sizeof(freq_buff));
		for (j = 5; j < buf_size; j++)
		{
			if (isdigit(buff[j]))
			{
				freq_buff[count] = buff[j];
				count++;
			}
			else
				{
					fprintf(log, "%s I\n", buff);
					printf("%s I\n", buff);
					return 1;
				}
		}
		fprintf(log, "%s\n", buff);
		int temp = atoi(freq_buff);
		if (temp <= 3600 && temp >= 1)
			frequency = temp;
		else
		{
			fprintf(log, "%s I\n", buff);
			printf("%s I\n", buff);
		}

		free(freq_buff);
	}
	else{
		fprintf(log, "%s I\n", buff); //invalid command general case
                printf("%s I\n", buff);
                }
	fflush(log);
	
}



int main(int argc, char* argv[])
{


	uint16_t value;
	mraa_aio_context temp = mraa_aio_init(0);
	fd_set readset;


    //set up the initial connection
    struct hostent* server;
    struct sockaddr_in serv_addr;
    int init_socket = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname("lever.cs.ucla.edu");
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(16000);


    printf("connecting to general server...\n");
    if (connect(init_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
        	perror("failed to connect to server on first try\n");
        	exit(1);
        } 
    else   //memset(buff, 0, sizeof(buffer))
    	printf("connected to general server\n");
    //get personal port number
    char* message = "Port request 604493477";
    write(init_socket, message, 22);
    int portno = 0;
    read(init_socket, &portno, sizeof(int));

    close(init_socket);

    //connect to personal port number
    int my_socket = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    printf("connecting to personal server...\n");
    if (connect(my_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
        	perror("failed to connect to personal server\n");
        	exit(1);
        }
    else
    	printf("connected to personal server\n");

    char buffer[20];
    int size = 0;
    int run_flag = 1;

    FILE* log_file = fopen("log2", "w+");

    time_t time_stamp;

//    signal(SIGINT, do_when_interrupted);
    struct timeval dontWait;
    dontWait.tv_usec = 0;
    dontWait.tv_sec = 0;

    //use this to count the time between temp reads
    time_t start;
    time_t current;
    time(&start);

	while (run_flag)
	{

		//clear the buffer
        memset(buffer, 0, sizeof(buffer));

		FD_ZERO(&readset);
		FD_SET(my_socket, &readset);
		select(my_socket + 1, &readset, NULL, NULL, &dontWait);
        //if ready, read from socket
		if (FD_ISSET(my_socket, &readset))
		{
			size = read(my_socket, buffer, sizeof(buffer));
			read_handler(buffer, size, log_file);
		}

		//update timer
		time(&current);
		time_t diff = current - start;

		//get temp if frequency has passed and should be reading
		if (!STOP && diff >= frequency) //CHANGE FREQUENCY IF THEY WANT SOMETHING ELSE
		{

			struct tm* time_info;
			time_t time_raw;
			time(&time_raw);
			time_info = localtime(&time_raw);
		    //time_info->tm_hour like this
		    value = mraa_aio_read(temp);
		    char buf[30];
		    memset(buf, 0, 30);
		    float temperature = convert_temp(value);
		    sprintf(buf, "604493477 TEMP=%.1f\n", temperature);
		    write(my_socket, buf, 30);
		    write(STDOUT_FILENO, "TO SERVER: ", 11);
		    write(STDOUT_FILENO, buf, 30); //to know what you're sending
			fprintf(log_file, "%d:%d:%d %.1f %c\n", time_info->tm_hour, 
		    	time_info->tm_min, time_info->tm_sec, temperature, scale);
                        fflush(log_file);
			//reset time diff
			time(&start);
		}
	}
	mraa_aio_close(temp);

	fclose(log_file);
	return 0;
}

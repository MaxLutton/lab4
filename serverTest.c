#include <stdio.h>
#include <unistd.h>
#include <signal.h>
//#include <mraa/aio.h>
#include <time.h>
#include <string.h>
//#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <stdlib.h>

int main()
{
  struct hostent* server;
  struct sockaddr_in serv_addr;
  int init_socket = socket(AF_INET, SOCK_STREAM, 0);
  server = gethostbyname("lever.cs.ucla.edu");
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(16000);
  write(STDOUT_FILENO, "connecting to general server\n", 29 );
  
  if (connect(init_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("failed to connect to server on first try\n");
      exit(1);
    }
  else   //memset(buff, 0, sizeof(buffer))
    {
      write(STDOUT_FILENO,"SUCCESS: connected to general server!\n", 38);
    }

  char* message = "Port request 604493477";
  write(init_socket, message, 22);
  int portno = 0;
  read(init_socket, &portno, sizeof(int));

  char msg2[40];
  memset(msg2, 0, 40);
  sprintf(msg2, "Got port number: %d\n", portno);
  write(STDOUT_FILENO, msg2, sizeof(msg2));  

  close(init_socket);

  int my_socket = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  write(STDOUT_FILENO, "connecting to personal server...\n",33); 
  if (connect(my_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("failed to connect to personal server\n");
      exit(1);
    }
  else
    write(STDOUT_FILENO, "SUCCESS: connected to personal server.\n",39); 

  close(my_socket);

  
}

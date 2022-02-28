#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define FIFO_NAME "my_FIFO"
#define RESPONSE_FIFO_NAME "my_response_FIFO"
#define CONFIG_NAME "usr_CONFIG"

int logged = 0;


char* loginFnc(char s[] , char r[] )
{
	char aux[300];
	char usr[300];//the username 
	int i = 0;
	while(s[i] !=':')++i;
	strcpy(usr, s+i+1);
	
	int p_fd[2];
        if(pipe(p_fd) == -1)
        {
                return "ERR:Pipe creation failed.\0";
	}
	
	int c_fd , c_nr, rv;
	char msg[10];
	char *ptr;
	switch(fork())
	{
		case -1:
			return "fork() failed\n";
			
		case 0:		 
	   		c_fd = open( CONFIG_NAME , O_RDONLY );
			c_nr = read( c_fd , aux, 299 );
			if ( c_nr == -1 )
			{
				perror("Read config file failed\n");
			}
			else
			{
				ptr = strstr( aux , usr );
				if (ptr == NULL || usr[0] == '\0' ) 
				{
					strcpy(msg,"nok");
				}
				else
				{
					strcpy(msg,"ok");
				}
			}
			write(p_fd[1] , msg, strlen(msg) );//write in pipe
			exit(1);
		default:
			//Parent Porcess
			if (wait (&rv) < 0)
                        {
                                return "Wait() failed";
                        }
                        if(WEXITSTATUS(rv))
			{
				read(p_fd[0] , r , 299 ) ;//read from pipe
				if (!strncmp(r,"ok",2))
				{
					logged = 1;
					sprintf(r ,"Logged in as %s.", usr );			
					return r;
				}	
				return "User not found.";
			}
			else return "Login failed: child did not exit().";
	}
	return r;
}


void myTruncate( char s[] )
{
	int l = strlen(s);
	while ( l > 1 &&( s[l-1] ==' ' || s[l -1] == '\n' ) ) --l;
	s[l] = '\n';
	s[l + 1] = '\0';
	l++;
	int i = 0;
	while(s[i] == ' ')++i;
	if ( i > 0 )
	{
		char aux[l + 1];
		strcpy(aux , s + i); 
		strcpy(s , aux);
	}
}


char* loggedUsersFnc(char s[], char r[] )//this function uses sockets
{

	if(logged == 0 )
        {
                return "You have to be logged in to perform this action.\0";
        }
	int sockp[2], child;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) 
      	{ 
        	return "Err... socketpair"; 
      	}
	struct utmp *data;
	int rv;
	char buff[300] , aux[300];
	buff[0]='\0';
	switch(fork())
	{
		case -1:

			return "Fork failed";
			
		case 0://child process
			close(sockp[1]);
			data = getutent();
			int k = 0 , nrChar = 0;
			char msg[300];
			msg[0]='\0';
			while(data!=NULL)
			{
				++k;
				
				strncpy(aux, data->ut_user, 32);
				aux[32] = '\n';
				myTruncate(aux);
				if(strlen(aux) != 0)
				{
					strcat(msg, "user: ");
					strcat(msg, aux);
				}		
				strncpy(aux, data->ut_host, 255);
                                aux[254] = '\n';
                                myTruncate(aux);
                                if(strlen(aux) != 0)
				{
					strcat(msg, "host: ");
					strcat(msg, aux);
				}
				time_t timeInfo;
				timeInfo = data->ut_time;
				if( timeInfo )
				{
					strcat(msg, "Time Entry: ");
					strcat(msg, ctime(&timeInfo));
					strcat(msg, "\n");
				}
				data = getutent();
			}
			strcat(msg,"\0");
			write(sockp[0], msg , strlen(msg) +1);
			exit(0);	
		
		default://parent process
			close(sockp[0]);
			wait(&rv);	
			if (read(sockp[1], r, 299) < 0) return"[parinte]Err...read";
			return r;
	}
}

char* procInfoFnc(char s[], char r[])//this function uses pipes
{
	if(logged == 0 )
	{
		return "You have to be logged in to perform this action.\0";
	}
	char pid[20] , buffer[300];
	int i = 0;
	strcpy(pid , s+14);	
	while(pid[i]!='\0')
	{
		if(pid[i]>'0'&&pid[i]<'9')++i;
		else
		{
			return "Pid invalid";
		}
	}
	char path[30];
	path[0]='\0';
	strcat(path, "/proc/");
	strcat(path, pid);
	strcat(path, "/status");
	//Creating the pipe
	int p_fd[2];
	char buff[300];
	FILE *fp;
	if(pipe(p_fd) == -1)
	{
		return "ERR:Pipe creation failed.";
	}
	int s_nr , s_fd, rv; // s_ stands for the status file
	//Creating the child process
	switch(fork() )
	{
		case -1:
			return"fork() failed.";
		case 0: //Child Process 
			s_fd = open(path,O_RDONLY);
			s_nr = read(s_fd , buff, 300);
			if(s_nr == -1)
			{
				strcpy(r,"ERR: cannot open status file.");
				exit(0);
			}
			char *p = strtok(buff, "\n");
			if(p!=NULL)
			{
				r[0]='\0';
			}
			char msg[300];
			msg[0]='\n';
			while(p != NULL)
			{
				if (!strncmp(p,"Name",4) || !strncmp(p,"State",5) || !strncmp(p,"PPid",4) ||!strncmp(p,"Uid",3) 
						||!strncmp(p,"VmSize",6))
				{
					strcat(msg, p);
					strcat(msg, "\n");
				}
				p = strtok(NULL, "\n");
			}
			strcat(msg,"\0");
			write(p_fd[1] , msg , strlen(msg)+1 );//write in pipe
			
			exit(1);	
		default:
			//Parent Porcess
			if (wait (&rv) < 0)
			{
			  	return ("Wait() failed");
			}
			if(WEXITSTATUS(rv))read(p_fd[0] , r , 299 ) ;//read from pipe
			return r;
	}
}




char* logoutFnc()
{
	if ( logged == 0 )
	{
		return("You have to be logged in to logout.\0");
	}
	logged = 0;
	return "Logged out successfully.\0";
}

int quitFnc(char s[])
{
        logged = 0;
	char aux[300];
        char pid_s[300];//the pid of the client user 
        int i = 0, pid;
        while(s[i] !=':')++i;
        strcpy(pid_s, s+i+1);
	pid = atoi(pid_s);
	int rv;
	switch(fork())
	{
		case -1:
			perror("Fork() failed\n");
			return -1;
		case 0://child process
			kill(pid, SIGINT);
			exit(1);
		default://parent process
			wait(&rv);
	}	
	return 1;
}


int handlerComenzi(char s[] , char r[] )//s[] e comanda primita si r e raspunsul de trimis 
{
	if(!strncmp(s, "login:", 6))
	{
		strcpy(r ,loginFnc(s, r));		
		return 1;
	}
	if(!strcmp(s, "get-logged-users"))
	{
		strcpy( r , loggedUsersFnc(s, r)) ;
		return 1;
	}	
	if(!strncmp(s, "get-proc-info:", 14))
	{
		strcpy ( r , procInfoFnc(s , r));
		if(!strncmp(r, "Default", 7))
		{
			strcpy(r,"Could not find a process with that pid.\n");
		}
		return 1;
	}
	if(!strcmp(s, "logout"))
	{
		strcpy(r,logoutFnc(r) );
		return 1;
	}
	if(!strncmp(s, "quit:", 5))
	{
		quitFnc(s);
		return 1;
	}
	return -1;
}
void createUserConfigFile()
{
	int cfg_fd;
	char users[]="alexandru lazar constantin alc user admin root";
	cfg_fd = open(CONFIG_NAME, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0644);
        if(write(cfg_fd, users, strlen(users)) == -1)
        {
                perror("Eroare la crearea fisierului USR_CONFIG\n" );
        }
}

int main()
{
	char s[300] , r[300];
        int num, fd, r_fd , r_num; //r_ stands for response
        
        mknod(FIFO_NAME, S_IFIFO | 0666, 0);
	
	createUserConfigFile();//this creates the usr_CONFIG file and populates it.
	//the server uses usr_CONFIG file to validate the users in the login function. 
	
	printf("Astept cientul\n");
        fd = open(FIFO_NAME, O_RDONLY);
	
	r_fd = open(RESPONSE_FIFO_NAME, O_WRONLY);

        printf("Clientul este conectat:\n");

        do 
	{
            if ((num = read(fd, s, 300)) == -1)
                perror("Eroare la citirea din FIFO!");
            else 
	    {
                s[num] = '\0';
                if(s[0]!='\0')printf("Comanda citita: %s \n", s);
        	strcpy(r,"Default response.");
		handlerComenzi(s, r);
		r[strlen(r)+1]='\0';
		r_num = write ( r_fd , r , strlen(r)+1 );
	    }
        }while (num > 0);
	close(fd);
	close(r_fd);
	return 0;
}

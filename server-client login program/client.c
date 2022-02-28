#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "my_FIFO"
#define	RESPONSE_FIFO_NAME "my_response_FIFO"

int validate(char s[] )
{
	if ( !strcmp(s,"get-logged-users") || 
		!strcmp(s,"logout")) return 1;
	if (!strncmp ( s , "login", 5) || !strncmp(s,"get-proc-info",13) )
	{
		char aux[100];
		int i = 0 , l = strlen(s);
		while(i < l && s[i]=='-'|| (s[i] >'a' && s[i] <'z')) ++i;
		int j = i;
		while (j < l && s[j]==' ')++j;
		strcpy(aux, s+j);
		strcpy(s+i, aux);	
		if ( s[i] != ':' )return 0;
		i++;
		j = i;
		while (s[j]==' ')++j;
		strcpy(aux, s+j);
		strcpy(s+i, aux);
		return 1;
	}
	if(!strcmp(s,"quit"))
	{
		strcat(s, ":");
		int pid = getpid();
		char aux[10];
		sprintf(aux,"%d",pid);
		strcat(s,aux);
		return 1;
	}
	return 0;	
}


void myTruncate( char s[] )
{
	int l = strlen(s);
	while ( l > 1 &&( s[l-1] ==' ' || s[l -1] == '\n' ) ) --l;
	s[l] = 0;
	int i = 0;
	while(i < l && s[i] == ' ')++i;
	if ( i > 0 )
	{
		char aux[l + 1];
		strcpy(aux , s + i); 
		strcpy(s , aux);
	}
}




int main()
{
	char s[500] , r_s[500];
	int num , fd , r_fd , r_num ; //r_ stands for response
	
	printf("Astept serverul \n");

	fd = open(FIFO_NAME, O_WRONLY);

	mknod(RESPONSE_FIFO_NAME, S_IFIFO | 0666, 0);//!!!!!!!!!!!!!!!!modifica si copieaza in server
	r_fd = open(RESPONSE_FIFO_NAME, O_RDONLY);
	//verifica daca s-au creeat conexiunile
		
	printf("Serverul conectat\n");
	printf("Introduceti comenzile:\n");
	while (fgets(s, 298, stdin), !feof(stdin))
	{
		
		myTruncate(s);
		if(!validate(s))
		{
			printf("Comanda introdusa nu respecta protocolul\n");
		}
		else
		{
			num = write(fd,s,strlen(s));//trimit comanda catre server
			if (num == -1 )
			{
				perror("Problema la scriere in FIFO\n");
			}
			else
			{	
				r_num = read (r_fd , r_s , 300 );//primesc raspunsul de la comanda de la server
			        if (r_num == -1 )
				{
					perror("Problema la citirea din R_FIFO\n");			
				}	
				else
				{
					printf("[%d Octeti]:",r_num);
					r_s[r_num] = '\0' ;
					r_s[r_num+1] = 0;
					printf("%s\n",r_s);//afisez raspunsul primit
				}
			}
		}
	}
	return 0;
}

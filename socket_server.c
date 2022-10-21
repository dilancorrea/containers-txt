#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include<stdbool.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

struct Node{
   char* container;
   char* status;
   struct Node *next;
};

typedef struct Node Node;

typedef struct List{
   Node *head;
}List;

void *makeContainer( void *message );
void *listContainers( );
void *stopContainer( void *message );
void *runContainer( void *message );
void *deleteContainer( void *message );
bool buscar( char *container );
void imprimir( );
void imprimir2( List *l );
bool insertar( char *container );
bool modificar( char *container, int option );

int main() {
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;  // https://github.com/torvalds/linux/blob/master/tools/include/uapi/linux/in.h
	char client_message[2000];
	
	// Create socket
    // AF_INET (IPv4 protocol) , AF_INET6 (IPv6 protocol) 
    // SOCK_STREAM: TCP(reliable, connection oriented)
    // SOCK_DGRAM: UDP(unreliable, connectionless)
    // Protocol value for Internet Protocol(IP), which is 0
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		printf("Could not create socket");
	}
	//puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	
	//Bind the socket to the address and port number specified
	if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	//puts("bind done");
	
	// Listen
    // It puts the server socket in a passive mode, where it waits for the client 
    // to approach the server to make a connection. The backlog, defines the maximum 
    // length to which the queue of pending connections for sockfd may grow. If a connection 
    // request arrives when the queue is full, the client may receive an error with an 
    // indication of ECONNREFUSED.
	// https://man7.org/linux/man-pages/man2/listen.2.html
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	
	while(1) {
		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if(client_sock < 0) {
			perror("accept failed");
			return 1;
		}
		puts("Connection accepted");
        memset(client_message, 0, 2000);
        recv(client_sock, client_message, 2000, 0);
        char *token = strtok( client_message, " \n" );
        char commands[ 3 ][ 200 ];
        int i = 0;
        while( token != NULL )
        {
           strcpy( commands[ i ], token );
           i = i + 1;
           token = strtok( NULL, " \n" );
        }
        pthread_t tid;
        if( strcmp( commands[ 0 ], "make" ) == 0 )
        {
           pthread_create( &tid, NULL, makeContainer, ( void* ) commands[ 1 ] );
           pthread_join( tid, NULL );
        }
        else if( strcmp( commands[ 0 ], "list" ) == 0 )
        {
           pthread_create( &tid, NULL, listContainers, NULL );
           pthread_join( tid, NULL );
        }
        else if( strcmp( commands[ 0 ], "stop" ) == 0 )
        {
           pthread_create( &tid, NULL, stopContainer, ( void* ) commands[ 1 ] );
           pthread_join( tid, NULL );
        }
        else if( strcmp( commands[ 0 ], "run" ) == 0 )
        {
           pthread_create( &tid, NULL, runContainer, ( void* ) commands[ 1 ] );
           pthread_join( tid, NULL );
        }
        else if( strcmp( commands[ 0 ], "delete" ) == 0 )
        {
           pthread_create( &tid, NULL, deleteContainer, ( void* ) commands[ 1 ] );
           pthread_join( tid, NULL );
        }
    }
	exit( 0 );
}

bool buscar( char *container )
{
   //pthread_mutex_lock( &mutex1 );
   FILE *fin;
   char linea[ 30 ];
   fin = fopen( "containers.txt", "r+" );
   while( fgets( linea, 30, fin ) )
   {
      char *token = strtok( linea, "\n " );
      char commands[ 2 ][ 200 ];
      int i = 0;
      while( token != NULL )
      {
         strcpy( commands[ i ], token );
         i = i + 1;
         token = strtok( NULL, "\n " );
      }
      if( strcmp( commands[ 0 ], container ) == 0 )
      {
         fclose( fin );
         return true; //Existe el contenedor.
      }
   }
   fclose( fin );
   //pthread_mutex_unlock( &mutex1 );
   return false; //No encontró el contenedor
}

void imprimir( )
{
   pthread_mutex_lock( &mutex1 );
   FILE *fin;
   fin = fopen( "containers.txt", "r+" );
   char linea[ 30 ];
   fgets( linea, 30, fin );
   while( !feof( fin ) )
   {
      printf( "%s", linea );
      fgets( linea, 30, fin );
   }
   pthread_mutex_unlock( &mutex1 );
   fclose( fin );
}

void imprimir2( List *l )
{
   FILE *fin;
   fin = fopen( "containers.txt", "w" );
   Node *node = l->head;
   while( node != NULL )
   {
      fprintf( fin, "%s %s\n", node->container, node->status );
      node = node->next;
   }
   free( node );
   fclose( fin );
}

bool insertar( char *container )
{
   pthread_mutex_lock( &mutex1 );
   if( buscar( container ) == 1 )
   {
      printf( "Container already exists.\n" );
      return false;
   }
   else
   {
      FILE *fin;
      fin = fopen( "containers.txt", "r+" );
      char linea[ 30 ];
      while( fgets( linea, 30, fin ) )
         continue;
      char* temp = ( char * ) malloc( sizeof( char ) * 200 );
      strcpy( temp, container );
      strcat( temp, " running\n" );
      fputs( temp, fin );
      fclose( fin );
      return true;
   }
   pthread_mutex_unlock( &mutex1 );
}

bool modificar( char *container, int option )
{
   pthread_mutex_lock( &mutex1 );
   List *l;
   Node *prev;
   l = ( List * ) malloc( sizeof( List ) );
   l->head = NULL;
   FILE *fin;
   char linea[ 30 ];
   char commands[ 2 ][ 200 ];
   fin = fopen( "containers.txt", "r+" );
   bool flag = true, flag2 = false, flag3 = false;
   while( fgets( linea, 30, fin ) )
   {
      Node *node = ( Node * ) malloc( sizeof( Node ) );
      node->container = ( char * ) malloc( sizeof( char ) * 200 );
      node->status = ( char * ) malloc( sizeof( char ) * 200 );
      char *token = strtok( linea, "\n " );
      int i = 0;
      while( token != NULL )
      {
         strcpy( commands[ i ], token );
         i = i + 1;
         token = strtok( NULL, "\n " );
      }
      strcpy( node->container, commands[ 0 ] );
      strcpy( node->status, commands[ 1 ] );
      //node->container = commands[ 0 ];
      //node->status = commands[ 1 ];
      node->next = NULL;
      if( strcmp( commands[ 0 ], container ) == 0 ) 
      {
          //option 1: correr un contenedor detenido.
          //option 2: detener un contenedor corriendo.
          if( option == 1 && strcmp( commands[ 1 ], "stop" ) == 0 ) //El contenedor esta stop.
          {
             node->status = "running";
             flag2 = true;
          }
          else if( option == 1 && strcmp( commands[ 1 ], "running" ) == 0 )
          {
             printf( "Container already running.\n" );
             return false;
          }
	  else if( option == 2 && strcmp( commands[ 1 ], "running" ) == 0 )
          {
             node->status = "stop";
             flag2 = true;
          }
          else if( option == 2 && strcmp( commands[ 1 ], "stop" ) == 0  )
          {
             printf( "Container already stop.\n" );
             return false;
          }
          else if( option == 3 )
          {
             flag3 = true;
             flag2 = true;
          }
      }
      if( flag == true && flag3 == false )
      {
         l->head = node;
         prev = node;
         flag = false;
      }
      else if( flag3 == false )
      {
         prev->next = node;
         prev = node;
      }
      if( flag3 == true )
         flag3 = false;
   }
   fclose( fin );
   if( flag2 == true )
   {
      imprimir2( l );
      return true;
   }
   else
      printf( "Container doent exists.\n" );
      
   pthread_mutex_unlock( &mutex1 );
   return false;
}


void *makeContainer( void *message )
{
   char *name = (char *) message;
   pid_t pid = fork();
   if( pid == 0 )
      if( insertar( name ) == 1 )
         execlp("docker", "docker", "run", "-di", "--name", name, "ubuntu:latest", "/bin/bash", NULL );
   pthread_exit( 0 );
}

void *listContainers( )
{
   imprimir();
   pthread_exit( 0 );
}

void *stopContainer( void *message )
{
   char *name = (char *) message;
   pid_t pid = fork();
   if( pid == 0 )
      if( modificar( name, 2 ) == 1 )
         execlp("docker", "docker", "stop", name, NULL);
   pthread_exit( 0 );
}


void *runContainer( void *message )
{
   char *name = (char *) message;
   pid_t pid = fork();
   if( pid == 0 )
      if( modificar( name, 1 ) == 1 )
         execlp("docker", "docker", "stop", name, NULL);
   pthread_exit( 0 );
}

void *deleteContainer( void *message )
{
   char *name = (char *) message;
   pid_t pid = fork();
   if( pid == 0 )
      if( modificar( name, 3 ) == 1 )
         execlp("docker", "docker", "rm", name, NULL);
   pthread_exit( 0 );
}

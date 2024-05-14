#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif


char client_address_string[INET6_ADDRSTRLEN];  //String voor IP address van Client.
int initialization();
int connection( int internet_socket , FILE *filePointer);
void execution( int internet_socket ,char client_address_string );
void cleanup( int internet_socket, int client_internet_socket , int HTTP_socket );
int initialization_HTTP();
 
int main( int argc, char * argv[] )
{
	FILE *filePointer =fopen( "data.log", "w" );
	
	OSInit();
    int *client_internet_socket;
	int internet_socket = initialization();
	int HTTP_socket = initialization_HTTP();
while(1)
{
	int client_internet_socket = connection( internet_socket ,filePointer );
	
	execution( client_internet_socket , client_address_string);
}

    fclose(filePointer);
	cleanup( internet_socket, client_internet_socket , HTTP_socket);
	OSCleanup();

	return 0;
}

//----------------------------------------------------------------------------------
int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "22", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				perror( "bind" );
				close( internet_socket );
			}
			else
			{
				
				int listen_return = listen( internet_socket, 1 );
				if( listen_return == -1 )
				{
					close( internet_socket );
					perror( "listen" );
				}
				else
				{
					break;
				}
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

//-----------------------------------------------------------------------------

int initialization_HTTP()
{
struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_INET;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	int getaddrinfo_return = getaddrinfo( "ip-api.com", "80", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int connect_return = connect( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

//----------------------------------------------------------------------------------


int connection( int internet_socket , FILE *filePointer)
{
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	int client_socket = accept( internet_socket, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
	if( client_socket == -1 )
	{
		perror( "accept" );
		close( internet_socket );
		exit( 3 );
	}
	
	
	    //get IP address from client
        if (client_internet_address.ss_family == AF_INET) // IPv4 address
		{
            struct sockaddr_in* s = (struct sockaddr_in*)&client_internet_address;
            inet_ntop(AF_INET, &s->sin_addr, client_address_string, sizeof(client_address_string));
			fputs("IPv4 Adress : " , filePointer);
			fputs( client_address_string , filePointer);
			fputs("\n" , filePointer);
        } 
		
		else   // IPv6 address
		{   
            struct sockaddr_in6* s = (struct sockaddr_in6*)&client_internet_address;
            inet_ntop(AF_INET6, &s->sin6_addr, client_address_string, sizeof(client_address_string));
			fputs("IPv6 Adress : " , filePointer);
			fputs( client_address_string , filePointer);
			fputs("\n" , filePointer);
        }

        printf("Client IP address: %s\n", client_address_string);
		
 return client_socket;

}


//----------------------------------------------------------------------------------


void execution( int internet_socket ,char client_address_string )
{ 
	int number_of_bytes_received = 0;
	char buffer[1000];

	number_of_bytes_received = recv( internet_socket, buffer, ( sizeof buffer ) - 1, 0 );
	if( number_of_bytes_received == -1 )
	{
		perror( "recv" );
	}
		
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf( "Received : %s\n", buffer );
	}
	
	int number_of_bytes_send = 0;
	number_of_bytes_send = send( internet_socket,"SERVER" , 6, 0 );
	if( number_of_bytes_send == -1 )
	{
		perror( "send" );
	}
 
}


//----------------------------------------------------------------------------------


void cleanup( int internet_socket, int client_internet_socket ,int HTTP_socket )
{
	int shutdown_return = shutdown( client_internet_socket, SD_RECEIVE );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}
     shutdown_return = shutdown( HTTP_socket, SD_RECEIVE );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}
    close( HTTP_socket );
	close( client_internet_socket );
	close( internet_socket );
}

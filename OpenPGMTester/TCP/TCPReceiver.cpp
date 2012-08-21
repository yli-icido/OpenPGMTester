#include "stdafx.h"
#include "TCPReceiver.h"

#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )
#pragma warning ( disable: 4018 )
#pragma message (__FILE__ ": warning 4018 has been disableed" )

TCPReceiver::TCPReceiver() : 
mInitDone( false ),
mIsConnected( false ),
mIsToQuit( false ), 
mClientSocket( INVALID_SOCKET )
{
}

TCPReceiver::~TCPReceiver()
{
    shutdown();
}

int TCPReceiver::initVar()
{
    mInitDone = false;
    mIsConnected = false;
    mIsToQuit = false;
    mClientSocket = INVALID_SOCKET;
    return PGM_SUCCESS;
}

int TCPReceiver::init()
{
    int iResult;
    WSADATA wsaData;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return PGM_FAILURE;
    }
    mInitDone = true;
    return PGM_SUCCESS;
}

int TCPReceiver::connect()
{
    int retval = PGM_FAILURE;
    do 
    {
        if ( !mInitDone )
            break;

        // Declare an addrinfo object that contains a sockaddr structure and initialize these values
        struct addrinfo *result = NULL, *ptr = NULL, hints;

        ZeroMemory(&hints, sizeof (hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the local address and port to be used by the server
        retval = getaddrinfo(NULL, TCP_PORT.c_str(), &hints, &result);
        if (retval != 0) 
        {
            printf("getaddrinfo failed: %d\n", retval);
            WSACleanup();
            retval = PGM_FAILURE;
            break;
        }

        // Create a SOCKET for the server to listen for client connections
        SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listenSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            break;
        }

        // bind a socket that has already been created to an IP address and port. Client applications use the IP address and port to connect to the host network.
        // Setup the TCP listening socket
        retval = bind( listenSocket, result->ai_addr, (int)result->ai_addrlen );
        if (retval == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(listenSocket);
            WSACleanup();
            retval = PGM_FAILURE;
            break;
        }
        
        freeaddrinfo(result);


        if ( listen( listenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
            printf( "Listen failed with error: %ld\n", WSAGetLastError() );
            closesocket(listenSocket);
            WSACleanup();
            retval = PGM_FAILURE;
            break;
        }

        // Accept a client socket
        mClientSocket = accept(listenSocket, NULL, NULL);
        if ( mClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            retval = PGM_FAILURE;
            break;
        }

        // No longer need server socket
        closesocket(listenSocket);

        mIsConnected = true;
        retval = PGM_SUCCESS;
    } while ( false );

    return retval;
}

int TCPReceiver::receive()
{
    int retval = PGM_FATAL;
    bool isTerminated = false;
    FILE* pFileToWrite = NULL;
    char fileToWrite[11];
    int rCounter = 0;
    char cCounter[3];

    LONG        lBytesRead;
    char* buffer = new char[ TCP_BUFFER_SIZE ];

    do 
    {
        if ( !mIsConnected ) break;

        lBytesRead = recv (mClientSocket, buffer, TCP_BUFFER_SIZE, 0);
        if (lBytesRead == 0)
        {
            fprintf(stdout, "connection closing...\n");
            isTerminated = true;
        }
        else if (lBytesRead == SOCKET_ERROR)
        {
            fprintf(stderr, "recv() failed: Error = %d\n", WSAGetLastError());
            isTerminated = true;
        }
        else if (( lBytesRead <= strlen( "start" ) ) && ( strncmp( buffer, "start", lBytesRead ) == 0 ))
        {
            printf ("start\n");
            if ( pFileToWrite != NULL )
            {
                fclose( pFileToWrite );
                pFileToWrite = NULL;
            }
            strcpy( fileToWrite, "tcp_received" );
            _itoa( rCounter, cCounter, 10 );
            pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
            rCounter++;
        }
        else if (( lBytesRead <= strlen( "end" ) ) && ( strncmp( buffer, "end", lBytesRead ) == 0 ))
        {
            printf("end\n");
            fclose( pFileToWrite );
            pFileToWrite = NULL;
        }
        else if (( lBytesRead <= strlen( "-q" ) ) &&  ( strncmp( buffer, "-q", lBytesRead ) == 0 ))
        {
            if ( pFileToWrite != NULL )
            {
                fclose( pFileToWrite );
                pFileToWrite = NULL;
            }
            isTerminated = true;
        }
        else if ( pFileToWrite != NULL )
        {
            fwrite( buffer, 1, lBytesRead, pFileToWrite );
        }
        else
        {
            fprintf(stderr, "output file not open\n");
        }

        retval = PGM_SUCCESS;

    } while ( !isTerminated );

    return retval;
}

int TCPReceiver::shutdown()
{
    // shutdown the send half of the connection since no more data will be sent
    int iResult = ::shutdown( mClientSocket, SD_SEND );
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(mClientSocket);
        WSACleanup();
    }

    // cleanup
    closesocket(mClientSocket);
    WSACleanup();
    return PGM_SUCCESS;
}
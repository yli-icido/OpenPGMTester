#include "stdafx.h"
#include <string>
#include "TCPSender.h"

#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;

TCPSender::TCPSender() : 
mInitDone( false ),
mIsConnected( false ),
mIsToQuit( false ),
mSocket( NULL )
{
}

TCPSender::~TCPSender()
{
    shutdown();
}

int TCPSender::initVar()
{
    mInitDone = false;
    mIsConnected = false;
    mIsToQuit = false;
    mSocket = NULL;

    return PGM_SUCCESS;
}

int TCPSender::init()
{
    int retval = PGM_FAILURE;
    do 
    {
        WSADATA wsaData;
        int iResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed with error: %d\n", iResult);
            break;
        }

        mInitDone = true;
        retval = PGM_SUCCESS;

    } while ( false );
    return retval;
}

int TCPSender::connect()
{
    int retval = PGM_FAILURE;
    do 
    {
        if ( !mInitDone ) break;

        SOCKET connectSocket = INVALID_SOCKET;
        struct addrinfo *result = NULL, *ptr = NULL, hints;

        ZeroMemory( &hints, sizeof(hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int iResult = 0;
        bool isToQuite = false;
        printf( "please input the address / host name to connect\n" );
        printf( "   -q to quit\n" );
        string strUserInput;

        do 
        {
            getline( cin, strUserInput );
            if ( strUserInput == "-q" )
            {
                WSACleanup();
                isToQuite = true;
                break;
            }

            // Resolve the server address and port
            iResult = getaddrinfo(strUserInput.c_str(), TCP_PORT.c_str(), &hints, &result);
            if ( iResult != 0 )
            {
                printf("getaddrinfo failed with error: %d\n", iResult);
                printf("failed to connect re-enter the address to retry\n");
                printf("    -q to quit\n");
            }
            else if ( iResult == 0 )
                break;
        } while ( !isToQuite ) ;

        if ( isToQuite ) break;

        // Attempt to connect to an address until one succeeds
        for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
        {
            // Create a SOCKET for connecting to server
            connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (connectSocket == INVALID_SOCKET) 
            {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                break;
            }

            // Connect to server.
            iResult = ::connect( connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR)
            {
                closesocket(connectSocket);
                connectSocket = INVALID_SOCKET;
                continue;
            }
            mSocket = connectSocket;
            break;
        }

        freeaddrinfo(result);

        if (mSocket == INVALID_SOCKET) 
        {
            printf("Unable to connect to server!\n");
            WSACleanup();
            break;
        }

        retval = PGM_SUCCESS;
        mIsConnected = true;

    } while ( false );

    return retval;
}

int TCPSender::send()
{
    int retval = PGM_FAILURE;
    string userInput;
    int sCounter = 0;
    do 
    {
        // expect a file name
        cout << "please input the full file name to send" << endl;
        cout << "for more than one files, separate the file names with \";\"" << endl;
        cout << "   -q to quit" << endl;
        getline( cin, userInput );
        if ( userInput == "-q" )
        {
            retval = PGM_SUCCESS;
            break;
        }
        vector< string > filenames;
        filenames.push_back( "7z920-x64.msi" );
//         string separator(";");
//         retval = PGMUtils::intoTokens( userInput, separator, false, filenames );
//         if ( retval != PGM_SUCCESS )
//             break;

        FILE* pFileToSend = NULL;
        char* buffer = new char[ TCP_BUFFER_SIZE ];
        char cCounter[3];
        _itoa( sCounter, cCounter, 10 );
        char fileToWrite[10];
        strcpy( fileToWrite, "sent" );
        FILE* pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
        sCounter++;
        LONG        error;
        //:
        error = ::send (mSocket, "start", strlen( "start" ), NULL );
        if (error == SOCKET_ERROR)
        {
            fprintf (stderr, "send() failed: Error = %d\n", WSAGetLastError());
            retval = PGM_FATAL;
            break;
        }

        // send a "start" indicate a transfer session started

        for ( size_t i = 0; i < filenames.size(); i++ )
        {
            pFileToSend = fopen( filenames[i].c_str(), "rb" );
            if (pFileToSend == NULL) 
            {
                cerr << "Error opening file: " << filenames[i] << endl;
                continue;
            }
            fseek( pFileToSend, 0, SEEK_END );
            size_t fileSize = ftell( pFileToSend );
            size_t curPos = 0;
            size_t sizeToRead = TCP_BUFFER_SIZE;
            rewind( pFileToSend );
            size_t readResult = 0;
            while ( !feof( pFileToSend ) && ( curPos < fileSize ) )
            {
                if ( fileSize - curPos < TCP_BUFFER_SIZE )
                {
                    sizeToRead = fileSize - curPos;
                }

                readResult = fread( buffer, 1, sizeToRead, pFileToSend );
                if ( readResult != sizeToRead )
                {
                    cerr << "error reading file at: " << ftell(pFileToSend) << endl;
                    cerr << "sizeToRead:" << sizeToRead << ", actual read:" << readResult << endl;
                    cerr << "error code: " << ferror( pFileToSend ) << ", eof:" << feof( pFileToSend) << endl;
                }
                curPos += readResult;
                error = ::send ( mSocket, buffer, readResult, NULL );
                if (error == SOCKET_ERROR)
                {
                    fprintf (stderr, "send() failed: Error = %d\n", WSAGetLastError());
                    retval = PGM_FATAL;
                    break;
                }

                fwrite( buffer, 1, readResult, pFileToWrite );
            }
            fclose( pFileToSend );
            pFileToSend = NULL;
        }
        // send a "end" to indicate transfer session end
        error = ::send( mSocket, "end", strlen( "end" ), NULL );
        fclose( pFileToWrite );

    } while ( false );
    return retval;
}

int TCPSender::shutdown()
{
    int iResult = 0;
    // shutdown the connection since no more data will be sent
    iResult = ::shutdown(mSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(mSocket);
        WSACleanup();
        return PGM_FAILURE;
    }

    char recvbuf[TCP_BUFFER_SIZE];
    int recvbuflen = TCP_BUFFER_SIZE;
    // Receive until the peer closes the connection
    do 
    {

        iResult = recv(mSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(mSocket);
    WSACleanup();

    return PGM_SUCCESS;
}

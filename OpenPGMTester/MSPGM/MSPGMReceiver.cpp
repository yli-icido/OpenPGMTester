#include "stdafx.h"

#include "MSPGMReceiver.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
using namespace std;


MSPGMReceiver::MSPGMReceiver() : 
mInitDone( false ),
mIsToQuit( false ),
mNetwork( MSPGM_MULTICAST_ADDRESS ),
mPort( MSPGM_DATA_DESTINATION_PORT ),
mClientSocket( NULL )
{
}

MSPGMReceiver::~MSPGMReceiver()
{
    shutdown();
}

int MSPGMReceiver::initVar()
{
    mInitDone = false;
    mIsToQuit = false;
    mNetwork = MSPGM_MULTICAST_ADDRESS;
    mPort = MSPGM_DATA_DESTINATION_PORT;
    mClientSocket = NULL;
    return PGM_SUCCESS;
}

void MSPGMReceiver::usage()
{
}

int MSPGMReceiver::init()
{
    int retval = PGM_FAILURE;
    do 
    {
        WSADATA wsaData;

        /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
        WORD wVersionRequested = MAKEWORD(2, 2);

        int err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0) {
            /* Tell the user that we could not find a usable */
            /* Winsock DLL.                                  */
            printf("WSAStartup failed with error: %d\n", err);
            retval = PGM_FAILURE;
            break;
        }

        /* Confirm that the WinSock DLL supports 2.2.*/
        /* Note that if the DLL supports versions greater    */
        /* than 2.2 in addition to 2.2, it will still return */
        /* 2.2 in wVersion since that is the version we      */
        /* requested.                                        */

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            /* Tell the user that we could not find a usable */
            /* WinSock DLL.                                  */
            printf("Could not find a usable version of Winsock.dll\n");
            WSACleanup();
            retval = PGM_FAILURE;
            break;
        }
        else
            printf("The Winsock 2.2 dll was found okay\n");

        mInitDone = true;
//         checkProtocols();
        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int MSPGMReceiver::checkProtocols()
{
    int retval = PGM_FAILURE;
    do 
    {
        if ( !mInitDone ) break;

        //-----------------------------------------
        // Declare and initialize variables
        int iResult = 0;

        int iError = 0;
        INT iNuminfo = 0;

        int i;

        // Allocate a 16K buffer to retrieve all the protocol providers
        DWORD dwBufferLen = 16384;

        LPWSAPROTOCOL_INFO lpProtocolInfo = NULL;

        // variables needed for converting provider GUID to a string
        int iRet = 0;
        WCHAR GuidString[40] = { 0 };

        lpProtocolInfo = (LPWSAPROTOCOL_INFO) MALLOC(dwBufferLen);
        if (lpProtocolInfo == NULL) {
            wprintf(L"Memory allocation for providers buffer failed\n");
            WSACleanup();
            return 1;
        }

        iNuminfo = WSAEnumProtocols(NULL, lpProtocolInfo, &dwBufferLen);
        if (iNuminfo == SOCKET_ERROR) {
            iError = WSAGetLastError();
            if (iError != WSAENOBUFS) {
                wprintf(L"WSAEnumProtocols failed with error: %d\n", iError);
                if (lpProtocolInfo) {
                    FREE(lpProtocolInfo);
                    lpProtocolInfo = NULL;
                }
                WSACleanup();
                return 1;
            } else {
                wprintf(L"WSAEnumProtocols failed with error: WSAENOBUFS (%d)\n",
                    iError);
                wprintf(L"  Increasing buffer size to %d\n\n", dwBufferLen);
                if (lpProtocolInfo) {
                    FREE(lpProtocolInfo);
                    lpProtocolInfo = NULL;
                }
                lpProtocolInfo = (LPWSAPROTOCOL_INFO) MALLOC(dwBufferLen);
                if (lpProtocolInfo == NULL) {
                    wprintf(L"Memory allocation increase for buffer failed\n");
                    WSACleanup();
                    return 1;
                }
                iNuminfo = WSAEnumProtocols(NULL, lpProtocolInfo, &dwBufferLen);
                if (iNuminfo == SOCKET_ERROR) {
                    iError = WSAGetLastError();
                    wprintf(L"WSAEnumProtocols failed with error: %d\n", iError);
                    if (lpProtocolInfo) {
                        FREE(lpProtocolInfo);
                        lpProtocolInfo = NULL;
                    }
                    WSACleanup();
                    return 1;
                }

            }
        }

        wprintf(L"WSAEnumProtocols succeeded with protocol count = %d\n\n",
            iNuminfo);
        for (i = 0; i < iNuminfo; i++) {
            wprintf(L"Winsock Catalog Provider Entry #%d\n", i);
            wprintf
                (L"----------------------------------------------------------\n");
            wprintf(L"Entry type:\t\t\t ");
            if (lpProtocolInfo[i].ProtocolChain.ChainLen == 1)
                wprintf(L"Base Service Provider\n");
            else
                wprintf(L"Layered Chain Entry\n");

            wprintf(L"Protocol:\t\t\t %ws\n", lpProtocolInfo[i].szProtocol);

            iRet =
                StringFromGUID2(lpProtocolInfo[i].ProviderId,
                (LPOLESTR) & GuidString, 39);
            if (iRet == 0)
                wprintf(L"StringFromGUID2 failed\n");
            else
                wprintf(L"Provider ID:\t\t\t %ws\n", GuidString);

            wprintf(L"Catalog Entry ID:\t\t %u\n",
                lpProtocolInfo[i].dwCatalogEntryId);

            wprintf(L"Version:\t\t\t %d\n", lpProtocolInfo[i].iVersion);

            wprintf(L"Address Family:\t\t\t %d\n",
                lpProtocolInfo[i].iAddressFamily);
            wprintf(L"Max Socket Address Length:\t %d\n",
                lpProtocolInfo[i].iMaxSockAddr);
            wprintf(L"Min Socket Address Length:\t %d\n",
                lpProtocolInfo[i].iMinSockAddr);

            wprintf(L"Socket Type:\t\t\t %d\n", lpProtocolInfo[i].iSocketType);
            wprintf(L"Socket Protocol:\t\t %d\n", lpProtocolInfo[i].iProtocol);
            wprintf(L"Socket Protocol Max Offset:\t %d\n",
                lpProtocolInfo[i].iProtocolMaxOffset);

            wprintf(L"Network Byte Order:\t\t %d\n",
                lpProtocolInfo[i].iNetworkByteOrder);
            wprintf(L"Security Scheme:\t\t %d\n",
                lpProtocolInfo[i].iSecurityScheme);
            wprintf(L"Max Message Size:\t\t %u\n", lpProtocolInfo[i].dwMessageSize);

            wprintf(L"ServiceFlags1:\t\t\t 0x%x\n",
                lpProtocolInfo[i].dwServiceFlags1);
            wprintf(L"ServiceFlags2:\t\t\t 0x%x\n",
                lpProtocolInfo[i].dwServiceFlags2);
            wprintf(L"ServiceFlags3:\t\t\t 0x%x\n",
                lpProtocolInfo[i].dwServiceFlags3);
            wprintf(L"ServiceFlags4:\t\t\t 0x%x\n",
                lpProtocolInfo[i].dwServiceFlags4);
            wprintf(L"ProviderFlags:\t\t\t 0x%x\n",
                lpProtocolInfo[i].dwProviderFlags);

            wprintf(L"Protocol Chain length:\t\t %d\n",
                lpProtocolInfo[i].ProtocolChain.ChainLen);

            wprintf(L"\n");
        }

        if (lpProtocolInfo) {
            FREE(lpProtocolInfo);
            lpProtocolInfo = NULL;
        }

        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int MSPGMReceiver::connect()
{
    int retval = PGM_FAILURE;
    if ( !mInitDone ) 
    {
        return PGM_FAILURE;
    }

    do
    {
        SOCKADDR_IN   salocal, sasession;
        int           sasessionsz;

        SOCKET sock = socket (AF_INET, SOCK_RDM, IPPROTO_RM);
        if ( sock == INVALID_SOCKET )
        {
            fprintf (stderr, "socket() failed: Error = %d\n", WSAGetLastError());
            break;
        }

        //
        // The bind port (dwSessionPort) specified should match that
        // which the sender specified in the connect call
        //
        salocal.sin_family = AF_INET;
        salocal.sin_port   = htons( mPort );    
        salocal.sin_addr.s_addr = inet_addr( MSPGM_MULTICAST_ADDRESS.c_str() );
        int errCode = bind (sock, (SOCKADDR *)&salocal, sizeof(salocal));
        if ( errCode == SOCKET_ERROR )
        {
            fprintf (stderr, "bind() failed: Error = %d\n", WSAGetLastError());
            break;
        }

        //
        // Set all relevant receiver socket options here
        //
        errCode = listen (sock, 10);
        if ( errCode == SOCKET_ERROR )
        {
            fprintf (stderr, "listen() failed: Error = %d\n", WSAGetLastError());
            break;
        }
        
        ULONG localif = inet_addr("10.0.12.10");

        setsockopt(sock, IPPROTO_RM, RM_ADD_RECEIVE_IF, (char *)&localif, sizeof(localif));

        sasessionsz = sizeof(sasession);
        mClientSocket = accept (sock, (SOCKADDR *)&sasession, &sasessionsz);
        if ( mClientSocket ==  INVALID_SOCKET )
        {
            fprintf (stderr, "accept() failed: Error = %d\n", WSAGetLastError());
            break;
        }

        //
        // accept will return the client socket and we are now ready
        // to receive data on the new socket!
        //

        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int MSPGMReceiver::receive()
{
    int retval = PGM_FATAL;
    bool isTerminated = false;
    FILE* pFileToWrite = NULL;
    char fileToWrite[11];
    int rCounter = 0;
    char cCounter[3];

    LONG        lBytesRead;
    char* buffer = new char[ MSPGM_BUFFER_SIZE ];
    do 
    {
        lBytesRead = recv (mClientSocket, buffer, MSPGM_BUFFER_SIZE, 0);
        if (lBytesRead == 0)
        {
            fprintf(stdout, "Session was terminated\n");
        }
        else if (lBytesRead == SOCKET_ERROR)
        {
            fprintf(stderr, "recv() failed: Error = %d\n", WSAGetLastError());
        }
        else if (( lBytesRead <= strlen( "start" ) ) && ( strncmp( buffer, "start", lBytesRead ) == 0 ))
        {
            printf ("start\n");
            if ( pFileToWrite != NULL )
            {
                fclose( pFileToWrite );
                pFileToWrite = NULL;
            }
            strcpy( fileToWrite, "received" );
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

int MSPGMReceiver::shutdown()
{
    WSACleanup();
    return PGM_SUCCESS;
}

// take the complete option string, analyse if the user input option is valid
int MSPGMReceiver::analyseOptions( string& options )
{
    int retval = PGM_FAILURE;
    do
    {

        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int MSPGMReceiver::verifyOptions( std::map< char, std::string >& options )
{
    int retval = PGM_FAILURE;
    retval = PGM_SUCCESS;

    return retval;
}
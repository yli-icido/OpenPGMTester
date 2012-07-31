#include "stdafx.h"

#include "MSPGMSender.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

using namespace std;


MSPGMSender::MSPGMSender():
mInitDone( false ),
mIsConnected( false ),
mIsToQuit( false ),
mNetwork( MSPGM_MULTICAST_ADDRESS ),
mPort( MSPGM_DEFAULT_DATA_DESTINATION_PORT ),
mSocket( NULL )
{
}

MSPGMSender::~MSPGMSender()
{
    shutdown();
}

int MSPGMSender::initVar()
{
    mInitDone = false;
    mIsConnected = false;
    mIsToQuit = false;
    mNetwork = MSPGM_MULTICAST_ADDRESS;
    mPort = MSPGM_DEFAULT_DATA_DESTINATION_PORT;
    mSocket = NULL;
    return PGM_SUCCESS;
}

int MSPGMSender::init()
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
        checkProtocols();
        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int MSPGMSender::shutdown()
{
    WSACleanup();
    return PGM_SUCCESS;
}

int MSPGMSender::connect()
{
    int retval = PGM_FAILURE;
    if ( !mInitDone ) 
    {
        cerr << "pgm not initialized" << endl;
        return PGM_FATAL;
    }
    string userInput;
    int retryCnt = 0;

    do
    {
        retval = createSocket();
        if ( retval == PGM_FATAL )
            break;
        else if ( retval != PGM_SUCCESS )
        {
            if ( retryCnt < 5 )
            {
                retryCnt++;
                initVar();
                continue;
            }
            else
                break;
        }
    } while ( (retval != PGM_SUCCESS) && (mIsToQuit == false) );
    
    return retval;
}

int MSPGMSender::send()
{
    int retval = PGM_FAILURE;
    string userInput;
    int sCounter = 0;
    for (;;)
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
        filenames.push_back( "C:\\Users\\yli\\Downloads\\mouse.sso" );
//         string separator(";");
//         retval = PGMUtils::intoTokens( userInput, separator, false, filenames );
//         if ( retval != PGM_SUCCESS )
//             break;

        FILE* pFileToSend = NULL;
        char* buffer = new char[ MSPGM_BUFFER_SIZE ];
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
            size_t sizeToRead = MSPGM_BUFFER_SIZE;
            rewind( pFileToSend );
            size_t readResult = 0;
            while ( !feof( pFileToSend ) && ( curPos < fileSize ) )
            {
                if ( fileSize - curPos < MSPGM_BUFFER_SIZE )
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
    }

    return retval;
}

int MSPGMSender::createSocket()
{
    int retval = PGM_FAILURE;
    do 
    {
        SOCKADDR_IN   salocal, sasession;

        mSocket = socket (AF_INET, SOCK_RDM, IPPROTO_RM);
        if ( mSocket == INVALID_SOCKET )
        {
            fprintf (stderr, "socket() failed: Error = %d\n", WSAGetLastError());
            break;
        }

        // Set the exclusive address option
        int iOptval = 1;
        int errCode = 0;
//         errCode = setsockopt(mSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &iOptval, sizeof (iOptval));
//         if (errCode == SOCKET_ERROR) 
//         {
//             wprintf(L"setsockopt for SO_EXCLUSIVEADDRUSE failed with error: %ld\n", WSAGetLastError());
//         }
// 
//         errCode = setsockopt( mSocket, SOL_SOCKET, SO_BROADCAST, ( char* )&iOptval, sizeof(iOptval) );
//         if (errCode == SOCKET_ERROR)
//         {
//             wprintf(L"bind failed with error %u\n", WSAGetLastError());
//             break;
//         }

        salocal.sin_family = AF_INET;
        salocal.sin_port   = htons (0);    // Port is ignored here
        salocal.sin_addr.s_addr = htonl (INADDR_ANY);
        errCode = ::bind (mSocket, (SOCKADDR *)&salocal, sizeof(salocal));
        if ( errCode == SOCKET_ERROR )
        {
            int iError = WSAGetLastError();
            fprintf (stderr, "bind() failed: Error = %d\n", iError);
            if (iError == WSAEACCES)
                fprintf(stderr, "bind failed with WSAEACCES (access denied)\n");
            else
                fprintf(stderr, "bind failed with error: %ld\n", iError);

        }
        else
        {
            fprintf(stderr, "bind on socket with SO_EXCLUSIVEADDRUSE succeeded to port: %ld\n", salocal.sin_port);
        }

        //
        // Set all relevant sender socket options here
        //

        //
        // Now, connect <entity type="hellip"/>
        // Setting the connection port (dwSessionPort) has relevance, and
        // can be used to multiplex multiple sessions to the same
        // multicast group address over different ports
        //
        sasession.sin_family = AF_INET;
        sasession.sin_port   = htons ( mPort );
        sasession.sin_addr.s_addr = inet_addr ( MSPGM_MULTICAST_ADDRESS.c_str() );
        errCode = ::connect (mSocket, (SOCKADDR *)&sasession, sizeof(sasession));
        if ( errCode == SOCKET_ERROR )
        {
            fprintf (stderr, "connect() failed: Error = %d\n", WSAGetLastError());
            break;
        }

        //
        // We're now ready to send data!
        //
        mIsConnected = true;
        retval = PGM_SUCCESS;

    } while ( false );

    if ( retval != PGM_SUCCESS )
    {
        closesocket( mSocket );
        WSACleanup();
    }

    return retval;
}

void MSPGMSender::usage()
{
}

// take the complete option string, analyse if the user input option is valid
int MSPGMSender::analyseOptions( string& options )
{
    const string supportedOptions("nsprfKNlih?q");
    int retval = PGM_FAILURE;
    do
    {
        map< char, string > optionPairs;

//         retval = PGMUtils::intoOptions( options, optionPairs );
        if ( retval != PGM_SUCCESS )
        {
            break;
        }

        map< char, string >::iterator itr = optionPairs.begin();
        while ( itr != optionPairs.end() )
        {
            string optionValue = itr->second;
            char optionSwitch = itr->first;
            switch ( optionSwitch )
            {

            case 'q':
                mIsToQuit = true;
                break;
            default:
                cout << "wrong parameter: -" << optionSwitch << " " << optionValue << endl;
                usage();
                break;
            }

            itr++;
        }

        retval = verifyOptions( optionPairs );
        if ( retval != PGM_SUCCESS )
        {
            break;
        }

        retval = PGM_SUCCESS;

    } while ( false );
    
    return retval;
}

int MSPGMSender::verifyOptions( std::map< char, std::string >& options )
{
    int retval = PGM_FAILURE;
    retval = PGM_SUCCESS;

    return retval;
}

int MSPGMSender::checkProtocols()
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
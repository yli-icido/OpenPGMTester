#include "stdafx.h"
#include "AceMcastSender.h"
#include <iostream>
#include <string>

using namespace std;

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

AceMcastSender::AceMcastSender() :
mInitDone( false ),
mIsConnected( false ),
mMcastAddr( ACEMCAST_DATA_DESTINATION_PORT, ACEMCAST_MULTICAST_GROUP.c_str() ),
mMcastSock( (ACE_SOCK_Dgram_Mcast::options) ( ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO ) )
{
}

AceMcastSender::~AceMcastSender()
{
    shutdown();
}

int AceMcastSender::init()
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

int AceMcastSender::connect()
{
    int retval = PGM_FAILURE;
    do 
    {
        if ( !mInitDone ) break;

        int errCode = mMcastSock.join( mMcastAddr );
        if ( errCode == -1 )
        {
            break;
            printf("AceMcastSender::connect() failed join group\n");
        }

        mIsConnected = true;
        retval = PGM_SUCCESS;
    } while ( false );
    return retval;
}

int AceMcastSender::send()
{
    int retval = PGM_FAILURE;
    string userInput;
    int sCounter = 0;
    char cCounter[3];
    char fileToWrite[50];
    bool bSendFile = false;
    vector< string > filenames;
    int sizeInMB = 100;
    size_t completeSizeToSend = 0;

    do 
    {
        cout << "entrer \"f\" to send a file" << endl;
        cout << "press enter to send memory block" << endl;
        cout << "   -q to quit" << endl;
        getline( cin, userInput );
        if ( userInput == "-q" )
        {
            retval = PGM_SUCCESS;
            break;
        }
        else if ( userInput == "f" )
        {
            bSendFile = true;
            filenames.clear();
            // expect a file name
            cout << "please input the full file name to send" << endl;
            cout << "not sending files? press enter" << endl;
            getline( cin, userInput );
            if ( userInput.empty() )
                filenames.push_back( "TortoiseGit-1.7.12.0-32bit.msi" );
            else
                filenames.push_back( userInput );
            //         string separator(";");
            //         retval = PGMUtils::intoTokens( userInput, separator, false, filenames );
            //         if ( retval != PGM_SUCCESS )
            //             break;
        }
        else 
        {
            bSendFile = false;
            // expect a file name
            cout << "enter a size to send in MB:" << endl;
            getline( cin, userInput );
            sizeInMB = atoi( userInput.c_str() );
        }

        FILE* pFileToSend = NULL;
        char* buffer = new char[ ACEMCAST_MESSAGE_LEN ];
        _itoa( sCounter, cCounter, 10 );
        strcpy( fileToWrite, "acemcast_sent" );
        FILE* pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
        sCounter++;
        LONG error;
        long lPackCounter = 0;

        // send a "start" indicate a transfer session started
        error = mMcastSock.send( "start", strlen( "start" ) );
        if ( error == -1 )
        {
            fprintf (stderr, "send() failed: Error = %d\n", error );
            retval = PGM_FATAL;
            break;
        }

        DWORD curTime = GetTickCount();
        if ( bSendFile )
        {
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
                size_t sizeToRead = ACEMCAST_MESSAGE_LEN;
                rewind( pFileToSend );
                size_t readResult = 0;
                while ( !feof( pFileToSend ) && ( curPos < fileSize ) )
                {
                    if ( fileSize - curPos < ACEMCAST_MESSAGE_LEN )
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
                    error = mMcastSock.send( buffer, sizeToRead );
                    if (error == -1)
                    {
                        fprintf (stderr, "send() failed: Error = %d\n", error );
                        retval = PGM_FATAL;
                        break;
                    }

                    fwrite( buffer, 1, readResult, pFileToWrite );
                }
                fclose( pFileToSend );
                pFileToSend = NULL;
            }
        }
        else 
        {
            completeSizeToSend = sizeInMB * 1024 * 1024;
            size_t curPos = 0;
            size_t sizeToRead = ACEMCAST_MESSAGE_LEN;
            while ( curPos < completeSizeToSend )
            {
                if ( completeSizeToSend - curPos < ACEMCAST_MESSAGE_LEN )
                {
                    sizeToRead = completeSizeToSend - curPos;
                }
                curPos += sizeToRead;
                char cPackCounter[10];
                _itoa( lPackCounter, cPackCounter, 10 );
                lPackCounter++;

                strcpy( buffer, cPackCounter );
                error = mMcastSock.send( buffer, sizeToRead );
                if (error == -1)
                {
                    fprintf (stderr, "send() failed: Error = %d\n", error );
                    retval = PGM_FATAL;
                    break;
                }
            }
        }
        DWORD elapsedTime = GetTickCount() - curTime;
        // sleep 500 ms to make sure the end will not be lost 
        Sleep( ACEMCAST_DELAY_BEFORE_END );
        // send a "end" to indicate transfer session end
        error = mMcastSock.send( "end", strlen("end") );
        fprintf (stderr, "total bytes: %d, pack count: %d\n", completeSizeToSend, lPackCounter );
        double speedKBps = (double) sizeInMB / elapsedTime * 1000;
        fprintf( stderr, "time: %d ms, transfer speed: %.2f MBps\n", elapsedTime, speedKBps );
        fclose( pFileToWrite );

    } while ( true );

    return retval;
}

int AceMcastSender::shutdown()
{
    mMcastSock.leave( mMcastAddr );
    WSACleanup();

    return PGM_SUCCESS;
}

// int AceMcastSender::checkProtocols()
// {
//     int retval = PGM_FAILURE;
//     do 
//     {
//         if ( !mInitDone ) break;
// 
//         //-----------------------------------------
//         // Declare and initialize variables
//         int iResult = 0;
// 
//         int iError = 0;
//         INT iNuminfo = 0;
// 
//         int i;
// 
//         // Allocate a 16K buffer to retrieve all the protocol providers
//         DWORD dwBufferLen = 16384;
// 
//         LPWSAPROTOCOL_INFO lpProtocolInfo = NULL;
// 
//         // variables needed for converting provider GUID to a string
//         int iRet = 0;
//         WCHAR GuidString[40] = { 0 };
// 
//         lpProtocolInfo = (LPWSAPROTOCOL_INFO) MALLOC(dwBufferLen);
//         if (lpProtocolInfo == NULL) {
//             wprintf(L"Memory allocation for providers buffer failed\n");
//             WSACleanup();
//             return 1;
//         }
// 
//         iNuminfo = WSAEnumProtocols(NULL, lpProtocolInfo, &dwBufferLen);
//         if (iNuminfo == SOCKET_ERROR) {
//             iError = WSAGetLastError();
//             if (iError != WSAENOBUFS) {
//                 wprintf(L"WSAEnumProtocols failed with error: %d\n", iError);
//                 if (lpProtocolInfo) {
//                     FREE(lpProtocolInfo);
//                     lpProtocolInfo = NULL;
//                 }
//                 WSACleanup();
//                 return 1;
//             } else {
//                 wprintf(L"WSAEnumProtocols failed with error: WSAENOBUFS (%d)\n",
//                     iError);
//                 wprintf(L"  Increasing buffer size to %d\n\n", dwBufferLen);
//                 if (lpProtocolInfo) {
//                     FREE(lpProtocolInfo);
//                     lpProtocolInfo = NULL;
//                 }
//                 lpProtocolInfo = (LPWSAPROTOCOL_INFO) MALLOC(dwBufferLen);
//                 if (lpProtocolInfo == NULL) {
//                     wprintf(L"Memory allocation increase for buffer failed\n");
//                     WSACleanup();
//                     return 1;
//                 }
//                 iNuminfo = WSAEnumProtocols(NULL, lpProtocolInfo, &dwBufferLen);
//                 if (iNuminfo == SOCKET_ERROR) {
//                     iError = WSAGetLastError();
//                     wprintf(L"WSAEnumProtocols failed with error: %d\n", iError);
//                     if (lpProtocolInfo) {
//                         FREE(lpProtocolInfo);
//                         lpProtocolInfo = NULL;
//                     }
//                     WSACleanup();
//                     return 1;
//                 }
// 
//             }
//         }
// 
//         wprintf(L"WSAEnumProtocols succeeded with protocol count = %d\n\n",
//             iNuminfo);
//         for (i = 0; i < iNuminfo; i++) {
//             wprintf(L"Winsock Catalog Provider Entry #%d\n", i);
//             wprintf
//                 (L"----------------------------------------------------------\n");
//             wprintf(L"Entry type:\t\t\t ");
//             if (lpProtocolInfo[i].ProtocolChain.ChainLen == 1)
//                 wprintf(L"Base Service Provider\n");
//             else
//                 wprintf(L"Layered Chain Entry\n");
// 
//             wprintf(L"Protocol:\t\t\t %ws\n", lpProtocolInfo[i].szProtocol);
// 
//             iRet =
//                 StringFromGUID2(lpProtocolInfo[i].ProviderId,
//                 (LPOLESTR) & GuidString, 39);
//             if (iRet == 0)
//                 wprintf(L"StringFromGUID2 failed\n");
//             else
//                 wprintf(L"Provider ID:\t\t\t %ws\n", GuidString);
// 
//             wprintf(L"Catalog Entry ID:\t\t %u\n",
//                 lpProtocolInfo[i].dwCatalogEntryId);
// 
//             wprintf(L"Version:\t\t\t %d\n", lpProtocolInfo[i].iVersion);
// 
//             wprintf(L"Address Family:\t\t\t %d\n",
//                 lpProtocolInfo[i].iAddressFamily);
//             wprintf(L"Max Socket Address Length:\t %d\n",
//                 lpProtocolInfo[i].iMaxSockAddr);
//             wprintf(L"Min Socket Address Length:\t %d\n",
//                 lpProtocolInfo[i].iMinSockAddr);
// 
//             wprintf(L"Socket Type:\t\t\t %d\n", lpProtocolInfo[i].iSocketType);
//             wprintf(L"Socket Protocol:\t\t %d\n", lpProtocolInfo[i].iProtocol);
//             wprintf(L"Socket Protocol Max Offset:\t %d\n",
//                 lpProtocolInfo[i].iProtocolMaxOffset);
// 
//             wprintf(L"Network Byte Order:\t\t %d\n",
//                 lpProtocolInfo[i].iNetworkByteOrder);
//             wprintf(L"Security Scheme:\t\t %d\n",
//                 lpProtocolInfo[i].iSecurityScheme);
//             wprintf(L"Max Message Size:\t\t %u\n", lpProtocolInfo[i].dwMessageSize);
// 
//             wprintf(L"ServiceFlags1:\t\t\t 0x%x\n",
//                 lpProtocolInfo[i].dwServiceFlags1);
//             wprintf(L"ServiceFlags2:\t\t\t 0x%x\n",
//                 lpProtocolInfo[i].dwServiceFlags2);
//             wprintf(L"ServiceFlags3:\t\t\t 0x%x\n",
//                 lpProtocolInfo[i].dwServiceFlags3);
//             wprintf(L"ServiceFlags4:\t\t\t 0x%x\n",
//                 lpProtocolInfo[i].dwServiceFlags4);
//             wprintf(L"ProviderFlags:\t\t\t 0x%x\n",
//                 lpProtocolInfo[i].dwProviderFlags);
// 
//             wprintf(L"Protocol Chain length:\t\t %d\n",
//                 lpProtocolInfo[i].ProtocolChain.ChainLen);
// 
//             wprintf(L"\n");
//         }
// 
//         if (lpProtocolInfo) {
//             FREE(lpProtocolInfo);
//             lpProtocolInfo = NULL;
//         }
// 
//         retval = PGM_SUCCESS;
// 
//     } while ( false );
// 
//     return retval;
// }
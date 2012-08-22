#include "stdafx.h"
#include "AceMcastReceiver.h"
#include <iostream>
#include <string>
#include <algorithm>
#pragma warning ( disable: 4018 )
#pragma message (__FILE__ ": warning 4018 has been disableed" )

using namespace std;

AceMcastReceiver::AceMcastReceiver() : 
mInitDone( false ),
mIsConnected( false ),
mMcastAddr( ACEMCAST_DATA_DESTINATION_PORT, ACEMCAST_MULTICAST_GROUP.c_str() ),
mMcastSock( (ACE_SOCK_Dgram_Mcast::options) ( ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO ) )
{

}

AceMcastReceiver::~AceMcastReceiver()
{
    shutdown();
}

int AceMcastReceiver::init()
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

int AceMcastReceiver::connect()
{
    int retval = PGM_FAILURE;
    do 
    {
        if ( !mInitDone ) break;

        int errCode = mMcastSock.join( mMcastAddr );
        if ( errCode == -1 )
        {
            break;
            printf("AceMcastReceiver::connect() failed join group\n");
        }

        mIsConnected = true;
        retval = PGM_SUCCESS;
    } while ( false );
    return retval;
}

int AceMcastReceiver::receive()
{
    int rv = PGM_FAILURE;
    ACE_INET_Addr addr ;
    char* buff = new char[ ACEMCAST_MESSAGE_LEN ];
    int lBytesRead = 0;
    bool isTerminated = false;
    FILE* pFileToWrite = NULL;
    char fileToWrite[11];
    int rCounter = 0;
    char cCounter[3];
    bool bWriteToFile = false;
    char pPack[50];
    long lPackCounter = 0;
    vector< string > receivedPacks;
    string receivedPack;
    DWORD timeTick = 0;
    DWORD elapsedTime = 0;
    long lTotalReceived = 0;

    do 
    {
        if ( !mIsConnected ) break;

        lBytesRead = mMcastSock.recv( buff, ACEMCAST_MESSAGE_LEN, addr );
        if ( !bWriteToFile )
        {
            if ( lBytesRead <= 0 )
            {
                fprintf(stdout, "connection closing...\n");
                isTerminated = true;
            }
            else if (( lBytesRead <= strlen( "start" ) ) && ( strncmp( buff, "start", lBytesRead ) == 0 ))
            {
                fprintf( stdout, "start\n" );
                if ( lPackCounter != 0 )
                {
                    int curPack, nextPack;
                    int count = 0;
                    for ( size_t i = 0; i < receivedPacks.size() ; i++ )
                    {
                        curPack = atoi( receivedPacks[i].c_str() );
                        if ( i + 1 < receivedPacks.size() )
                        {                        
                            nextPack = atoi( receivedPacks[i + 1].c_str() );
                            if ( nextPack != curPack + 1 )
                            {
                                count += nextPack - curPack - 1;
                                fprintf( stdout, "packs between %d and %d are missing.\n", curPack, nextPack );
                            }
                        }
                    }
                    fprintf( stdout, "last pack: %d, total pack received: %d, total missing packs: %d\n", curPack, lPackCounter, count );
                    fprintf( stderr, "received bytes: %d\n", lTotalReceived );
                    lPackCounter = 0;
                    lTotalReceived = 0;
                    receivedPacks.clear();
                }
                timeTick = GetTickCount();
            }
            else if (( lBytesRead <= strlen( "end" ) ) && ( strncmp( buff, "end", lBytesRead ) == 0 ))
            {
                elapsedTime = GetTickCount() - timeTick - ACEMCAST_DELAY_BEFORE_END;
                fprintf( stdout, "end\n" );
                int count = 0;
                int incorrectOrdered = 0;
                int curPack = 0;
                int nextPack = atoi( receivedPacks[0].c_str() );
                vector<int> receivedNum;
                receivedNum.push_back( nextPack );
                for ( size_t i = 1; i < receivedPacks.size(); i++ )
                {
                    curPack = nextPack;
                    nextPack = atoi( receivedPacks[i].c_str() );
                    receivedNum.push_back( nextPack );
                    if ( curPack > nextPack )
                        incorrectOrdered++;
                }

                sort(receivedNum.begin(), receivedNum.end());

                for ( size_t i = 0; i < receivedNum.size() ; i++ )
                {
                    curPack = receivedNum[i];
                    if ( i + 1 < receivedNum.size() )
                    {                        
                        nextPack = receivedNum[i + 1];
                        if ( nextPack != curPack + 1 )
                        {
                            count += nextPack - curPack - 1;
                            fprintf( stdout, "packs between %d and %d are missing.\n", curPack, nextPack );
                        }
                    }
                }
                fprintf( stdout, "last pack: %d, total pack received: %d, total missing packs: %d\n", curPack, lPackCounter, count );
                double speedKBps = ((double)lTotalReceived / (1024 * 1024)) / elapsedTime * 1000;
                fprintf( stderr, "received bytes: %d, time: %d ms, transfer speed: %.2f MBps\n", lTotalReceived, elapsedTime, speedKBps );
                lTotalReceived = 0;
                lPackCounter = 0;
                receivedPacks.clear();
            }
            else
            {
                lPackCounter++;
                lTotalReceived += lBytesRead;
                receivedPack = buff;
                receivedPacks.push_back( receivedPack );
//                 strncpy( pPack, buff, 10 );
//                 fprintf( stdout, "pack no. %s\n", pPack );
            }
        }
        else 
        {
            if (lBytesRead <= 0)
            {
                fprintf(stdout, "connection closing...\n");
                isTerminated = true;
            }
            else if (( lBytesRead <= strlen( "start" ) ) && ( strncmp( buff, "start", lBytesRead ) == 0 ))
            {
                printf ("start\n");
                if ( pFileToWrite != NULL )
                {
                    fclose( pFileToWrite );
                    pFileToWrite = NULL;
                }
                strcpy( fileToWrite, "acemcast_received" );
                _itoa( rCounter, cCounter, 10 );
                pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
                rCounter++;
            }
            else if (( lBytesRead <= strlen( "end" ) ) && ( strncmp( buff, "end", lBytesRead ) == 0 ))
            {
                printf("end\n");
                fclose( pFileToWrite );
                pFileToWrite = NULL;
            }
            else if (( lBytesRead <= strlen( "-q" ) ) &&  ( strncmp( buff, "-q", lBytesRead ) == 0 ))
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
                fwrite( buff, 1, lBytesRead, pFileToWrite );
            }
            else
            {
                fprintf(stderr, "output file not open\n");
            }
        }

        rv = PGM_SUCCESS;

    } while ( !isTerminated );

    return rv;
}

int AceMcastReceiver::shutdown()
{
    mMcastSock.leave( mMcastAddr );
    WSACleanup();

    return PGM_SUCCESS;
}

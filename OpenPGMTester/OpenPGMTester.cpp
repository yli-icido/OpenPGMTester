// OpenPGMConsoleTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pgm/pgm.h"
#include "OpenPGMReceiver.h"
#include "OpenPGMSender.h"
#include "MSPGMReceiver.h"
#include "MSPGMSender.h"
#include "OpenPGMReliableSender.h"

#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;

int _tmain( int argc, _TCHAR* argv[] )
{
    int retval = EXIT_FAILURE;
    char userInputc[256];

    do 
    {
        fprintf (stderr, "type \"send\" to send packages\n");
        fprintf (stderr, "type \"receive\" to receive packages\n");
        fprintf (stderr, "type \"exit\" to exit\n");
        gets( userInputc );

        if ( strcmp( userInputc, "send" ) == 0 ) // send
        {
            do 
            {
                Sender* sender = new OpenPGMReliableSender();
                if ( sender == NULL ) break;

                if ( sender->init() != PGM_SUCCESS ) 
                    break;

                if ( sender->connect() != PGM_SUCCESS )
                    break;

                if ( sender->send() != PGM_SUCCESS )
                    break;

                delete sender;
                retval = EXIT_SUCCESS;

            } while ( false );
        }
        else if ( strcmp( userInputc, "receive" ) == 0 ) // receive
        {
            do 
            {
                Receiver* receiver = new OpenPGMReceiver();
                if ( receiver == NULL ) break;

                if ( receiver->init() != PGM_SUCCESS )
                    break;

                if ( receiver->connect() != PGM_SUCCESS )
                    break;

                if ( receiver->shutdown() != PGM_SUCCESS )
                    break;

                delete receiver;
                retval = EXIT_SUCCESS;

            } while ( false );
        }
        else if ( strcmp( userInputc, "exit" ) == 0 )
        {
            retval = EXIT_SUCCESS;
            break;
        }

    } while ( strcmp( userInputc, "exit" ) != 0 );

    return retval;
}

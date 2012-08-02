// OpenPGMConsoleTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pgm/pgm.h"
#include "Sender.h"
#include "Receiver.h"
#include "Factory.h"

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
            Sender* sender = NULL;
            do 
            {
                fprintf (stderr, "enter the number for the sender to create? \n");
                for ( int i = 0; i < SENDER_TYPES_CNT; i++ )
                {
                    fprintf ( stderr, "%d: %s\n", i, SENDER_TYPE_NAMES[i].c_str() );
                }
                gets( userInputc );

                sender = Factory::createSender( userInputc );
                if ( sender == NULL ) break;

                if ( sender->init() != PGM_SUCCESS ) 
                    break;

                if ( sender->connect() != PGM_SUCCESS )
                    break;

                if ( sender->send() != PGM_SUCCESS )
                    break;

                retval = EXIT_SUCCESS;

            } while ( false );
            delete sender;
        }
        else if ( strcmp( userInputc, "receive" ) == 0 ) // receive
        {
            Receiver* receiver = NULL;
            do 
            {
                fprintf (stderr, "enter the number for the receiver to create? \n");
                for ( int i = 0; i < RECEIVER_TYPES_CNT; i++ )
                {
                    fprintf ( stderr, "%d: %s\n", i, RECEIVER_TYPE_NAMES[i].c_str() );
                }
                gets( userInputc );
                receiver = Factory::createReceiver( userInputc );
                if ( receiver == NULL ) break;

                if ( receiver->init() != PGM_SUCCESS )
                    break;

                if ( receiver->connect() != PGM_SUCCESS )
                    break;

                if ( receiver->receive() != PGM_SUCCESS )
                    break;

                retval = EXIT_SUCCESS;

            } while ( false );
            delete receiver;
       }
        else if ( strcmp( userInputc, "exit" ) == 0 )
        {
            retval = EXIT_SUCCESS;
            break;
        }

    } while ( strcmp( userInputc, "exit" ) != 0 );

    return retval;
}

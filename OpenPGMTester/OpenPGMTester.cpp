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
            do 
            {
                fprintf (stderr, "which sender to create? \n");
                for ( int i = 0; i < SENDER_TYPES_NUM; i++ )
                {
                    fprintf ( stderr, "%s \n", SENDER_TYPES[i].c_str() );
                }
                gets( userInputc );
                string inputString( userInputc );

                Sender* sender = Factory::createSender( inputString );
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
                fprintf (stderr, "which receiver to create? \n");
                for ( int i = 0; i < RECEIVER_TYPES_NUM; i++ )
                {
                    fprintf ( stderr, "%s \n", RECEIVER_TYPES[i].c_str() );
                }
                gets( userInputc );
                string inputString( userInputc );
                Receiver* receiver = Factory::createReceiver( inputString );
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

// OpenPGMConsoleTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pgm/pgm.h"
#include "PGMReceiver.h"
#include "PGMSender.h"

using namespace std;

int _tmain( int argc, _TCHAR* argv[] )
{
    int retval = EXIT_FAILURE;
    if ( argc < 2 ) // command and parameter indicates it is sender or receiver 
    {
        _tprintf( _T("wrong parameters") );
    }
    else if ( _tcscmp( argv[1], _T("send") ) == 0 ) // send
    {
        do 
        {
            PGMSender* sender = new PGMSender();
            if ( sender == NULL ) break;

            if ( sender->init() != PGM_SUCCESS ) 
                break;

            if ( sender->connect() != PGM_SUCCESS )
                break;

            if ( sender->send() != PGM_SUCCESS )
                break;

            retval = EXIT_SUCCESS;

        } while ( false );
    }
    else if ( _tcscmp( argv[1], _T("receive") ) == 0 ) // receive
    {
        do 
        {
            PGMReceiver* receiver = new PGMReceiver();
            if ( receiver == NULL ) break;

            if ( receiver->init() != PGM_SUCCESS )
                break;

            if ( receiver->connect() != PGM_SUCCESS )
                break;

            //             if ( receiver->receive() != PGM_SUCCESS )
            //                 break;

            retval = EXIT_SUCCESS;

        } while ( false );
    }
    else
    {
        _tprintf( _T("please provide parameter: send or receiver") );
    }

    return retval;
}

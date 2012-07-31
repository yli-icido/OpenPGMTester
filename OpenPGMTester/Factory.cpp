#include "stdafx.h"
#include "OpenPGMReceiver.h"
#include "OpenPGMSender.h"
#include "MSPGMReceiver.h"
#include "MSPGMSender.h"
#include "OpenPGMReliableSender.h"

#include "Factory.h"


Sender* Factory::createSender( char* senderToCreate )
{
    int senderType = atoi( senderToCreate );
    switch ( senderType )
    {
    case SIMPLE_OPEN_PGM_SENDER:
        return new OpenPGMSender();
    case RELIABLE_OPEN_PGM_SENDER:
        return new OpenPGMReliableSender();
    case MS_PGM_SENDER:
        return new MSPGMSender();
    default:
        fprintf( stderr, "Error no match sender to create\n");
        return NULL;
    }
}

Receiver* Factory::createReceiver( char* receiverToCreate )
{
    int receiverType = atoi( receiverToCreate );
    switch ( receiverType )
    {
    case OPEN_PGM_RECEIVER:
        return new OpenPGMReceiver();
    case MS_PGM_RECEIVER:
        return new MSPGMReceiver();
    default:
        fprintf( stderr, "Error no match sender to create\n");
        return NULL;
    }
}
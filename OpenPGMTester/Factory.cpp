#include "stdafx.h"
#include "OpenPGMReceiver.h"
#include "OpenPGMSender.h"
#include "MSPGMReceiver.h"
#include "MSPGMSender.h"
#include "OpenPGMReliableSender.h"

#include "Factory.h"

using namespace std;

Sender* Factory::createSender( std::string& senderToCreate )
{
    if ( senderToCreate == SENDER_TYPES[0] )
    {
        return new OpenPGMSender();
    }
    else if ( senderToCreate == SENDER_TYPES[1] )
    {
        return new OpenPGMReliableSender();
    }
    else if ( senderToCreate == SENDER_TYPES[2] )
    {
        return new MSPGMSender();
    }
    else 
    {
        fprintf( stderr, "Error no match sender to create\n");
        return NULL;
    }
}

Receiver* Factory::createReceiver( std::string& receiverToCreate )
{
    if ( receiverToCreate == RECEIVER_TYPES[0] )
    {
        return new OpenPGMReceiver();
    }
    else if ( receiverToCreate == RECEIVER_TYPES[1] )
    {
        return new MSPGMReceiver();
    }
    else 
    {
        fprintf( stderr, "Error no match sender to create\n");
        return NULL;
    }
}
#ifndef FACTORY_H_
#define FACTORY_H_

#include "common.h"

class Factory
{
public:
    static Sender* createSender( char* senderToCreate );
    static Receiver* createReceiver( char* receiverToCreate );
};
#endif // SENDER_FACTORY_H_
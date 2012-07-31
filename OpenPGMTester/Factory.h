#ifndef FACTORY_H_
#define FACTORY_H_

#include <string>
#include <map>
#include "common.h"


class Factory
{
public:
    static Sender* createSender( std::string& senderToCreate );
    static Receiver* createReceiver( std::string& receiverToCreate );
};
#endif // SENDER_FACTORY_H_
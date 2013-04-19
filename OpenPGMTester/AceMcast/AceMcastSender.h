#ifndef ACEMCAST_SENDER_H_
#define ACEMCAST_SENDER_H_

#include "AceMcastConfig.h"
#include "Sender.h"

class AceMcastSender : public Sender
{
public:
    AceMcastSender();
    virtual ~AceMcastSender();

    virtual int     init();
    virtual int     connect();
    virtual int     send();
    virtual int     shutdown();

private:
     int     checkProtocols();

private:
    bool    mInitDone;
    bool    mIsConnected;
    ACE_INET_Addr   mMcastAddr;
    ACE_SOCK_Dgram_Mcast mMcastSock;
};
#endif // ACEMCAST_SENDER_H_
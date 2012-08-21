#ifndef ACEMCAST_RECEIVER_H_
#define ACEMCAST_RECEIVER_H_

#include "AceMcastConfig.h"
#include "Receiver.h"

class AceMcastReceiver : public Receiver
{
public:
    AceMcastReceiver();
    virtual ~AceMcastReceiver();

    virtual int     init();
    virtual int     connect();
    virtual int     receive();
    virtual int     shutdown();

private:
    bool    mInitDone;
    bool    mIsConnected;
    ACE_INET_Addr   mMcastAddr;
    ACE_SOCK_Dgram_Mcast mMcastSock;
};
#endif // ACEMCAST_RECEIVER_H_
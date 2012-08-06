#ifndef TCP_SENDER_H_
#define TCP_SENDER_H_

#include "TCPConfig.h"
#include "Sender.h"

class TCPSender : public Sender
{
public:
    TCPSender();
    ~TCPSender();

    int     init();
    int     connect();
    int     send();
    int     shutdown();

private:
    int     initVar();

private:
    bool            mInitDone;
    bool            mIsConnected;
    bool            mIsToQuit;
    SOCKET          mSocket;
};
#endif // TCP_SENDER_H_
#ifndef TCP_RECEIVER_H_
#define TCP_RECEIVER_H_

#include "TCPConfig.h"
#include "Receiver.h"

// the tcp server acts as receiver, which accepts the connection from the client, then receives and sends data
class TCPReceiver : public Receiver
{
public:
    TCPReceiver();
    virtual ~TCPReceiver();
    virtual int     init();
    virtual int     connect();
    virtual int     receive();
    virtual int     shutdown();

private: 
    int     initVar();

private:
    bool            mInitDone;
    bool            mIsConnected;
    bool            mIsToQuit;
    std::string     mPort;
    SOCKET          mClientSocket;
};
#endif // TCP_RECEIVER_H_
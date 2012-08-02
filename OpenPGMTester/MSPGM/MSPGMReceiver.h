#ifndef MSPMGRECEIVER_H_
#define MSPMGRECEIVER_H_

#include "mspgm_config.h"
#include "Receiver.h"

class MSPGMReceiver : public Receiver
{
public:
    MSPGMReceiver();
    ~MSPGMReceiver();

    int     init();
    int     connect();
    int     receive();
    int     shutdown();

private:
    int     initVar();
    int     checkProtocols();
    void    usage();
    int     analyseOptions( std::string& options );
    int     verifyOptions( std::map< char, std::string >& options );
    int     createSocket();

private:
    bool            mInitDone;
    bool            mIsToQuit;
    std::string     mNetwork;
    unsigned int    mPort;
    SOCKET          mClientSocket;
};
#endif // MSPMGRECEIVER_H_
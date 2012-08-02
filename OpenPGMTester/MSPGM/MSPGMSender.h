#ifndef MSPGMSENDER_H_
#define MSPGMSENDER_H_

#include <vector>
#include "mspgm_config.h"
#include "Sender.h"

class MSPGMSender : public Sender
{
public:
    MSPGMSender();
    ~MSPGMSender();

    int     init();
    int     connect();
    int     send();
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
    bool            mIsConnected;
    bool            mIsToQuit;
    std::string     mNetwork;
    unsigned int    mPort;
    SOCKET          mSocket;
};
#endif // MSPGMSENDER_H_
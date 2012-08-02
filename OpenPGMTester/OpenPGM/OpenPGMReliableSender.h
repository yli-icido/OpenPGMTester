#ifndef PGM_RELIABLE_SENDER_H_
#define PGM_RELIABLE_SENDER_H_

#include <vector>
#include "openpgm_config.h"
#include "Sender.h"

class OpenPGMReliableSender : public Sender
{
public:
    OpenPGMReliableSender();
    virtual ~OpenPGMReliableSender();

    virtual int     init();
    virtual int     connect();
    virtual int     send();
    virtual int     shutdown();
    bool            isToQuit() { return mIsToQuit; }

private:
    int     initVar();
    void    usage();
    int     analyseOptions( std::string& options );
    int     verifyOptions( std::map< char, std::string >& options );
    int     createSocket();
    int     createNakThread();

private:
    bool            mInitDone;
    bool            mIsToQuit;
    std::string     mNetwork;
    unsigned int    mPort;
    unsigned int    mUdpEncapPort;
    unsigned int    mMaxRte;
    BOOL            mUseFec;
    unsigned int    mRsK;
    unsigned int    mRsN;
    BOOL            mUseMulticastLoop;
    pgm_sock_t*     mSock;
    int             mMaxTpDu;
    int             mSqns;
    int             m_no_router_assist;
    int             m_send_only;
    int             m_ambient_spm;
    int             m_nonblocking;
    int             m_multicast_hops;
    int             m_dscp;
    size_t          mMaxODataRTE;
    HANDLE          mNakThread;
};
#endif // PGM_RELIABLE_SENDER_H_
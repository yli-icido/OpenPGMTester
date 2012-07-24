#ifndef PGMSENDER_H_
#define PGMSENDER_H_

#include <vector>

class PGMSender
{
public:
    PGMSender();
    ~PGMSender();

    int     init();
    int     connect();
    int     send();
    int     shutdown();

private:
    int     initVar();
    void    usage();
    int     analyseOptions( std::string& options );
    int     verifyOptions( std::map< char, std::string >& options );
    int     createSocket();

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
};
#endif // PGMSENDER_H_
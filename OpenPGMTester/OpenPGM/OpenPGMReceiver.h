#ifndef PMGRECEIVER_H_
#define PMGRECEIVER_H_

#include "openpgm_config.h"
#include "Receiver.h"

class OpenPGMReceiver : public Receiver
{
public:
    OpenPGMReceiver();
    virtual ~OpenPGMReceiver();

    virtual int     init();
    virtual int     connect();
    virtual int     receive() { return PGM_SUCCESS; }
    virtual int     shutdown();
    static BOOL    on_console_ctrl( DWORD dwCtrlType );

private:
    int     initVar();
    void    usage();
    int     analyseOptions( std::string& options );
    int     verifyOptions( std::map< char, std::string >& options );
    int     createSocket();
    int     onStartup();
    int     onData( const void* restrict data, const size_t len, const struct pgm_sockaddr_t* restrict from );

private:
    bool            mInitDone;
    bool            mIsToQuit;
    std::string     mNetwork;
    unsigned int    mPort;
    unsigned int    mUdpEncapPort;
    BOOL            mUsePgmcc;
    BOOL            mUseFec;
    unsigned int    mRsK;
    unsigned int    mRsN;
    BOOL            mUseMulticastLoop;
    static WSAEVENT sTerminateEvent;
    static BOOL     sIsTerminated;
    pgm_sock_t*     mSock;
    int             mMaxTpDu;
    int             mSqns;
    WSAEVENT        mWaitEvents[ PGM_RECV_SOCKET_READ_COUNT + 1 ];
    int             m_no_router_assist;
    int             m_recv_only;
    int             m_passive;
    int             m_peer_expiry;
    int             m_spmr_expiry;
    int             m_nak_bo_ivl;
    int             m_nak_rpt_ivl;
    int             m_nak_rdata_ivl;
    int             m_nak_data_retries;
    int             m_nak_ncf_retries;
    size_t          m_odata_max_rate;
    int             m_nonblocking;
    int             m_multicast_hops;
    int             m_dscp;		/* Expedited Forwarding PHB for network elements, no ECN. */

};
#endif // PMGRECEIVER_H_
#ifndef PMGRECEIVER_H_
#define PMGRECEIVER_H_


class PGMReceiver
{
public:
    PGMReceiver();
    ~PGMReceiver();

    int     init();
    int     connect();
    int     receive();
    int     shutdown();
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
};
#endif // PMGRECEIVER_H_
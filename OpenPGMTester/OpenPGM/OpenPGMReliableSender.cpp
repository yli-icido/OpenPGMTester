#include "stdafx.h"
#include <process.h>
#include "pgm/log.h"
#include "OpenPGMUtils.h"

#include "OpenPGMReliableSender.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;

struct Nak_Routine_Param
{
    pgm_sock_t* sock;
    OpenPGMReliableSender* sender;
};

unsigned __stdcall nak_routine( void* arg )
{
    int retval = PGM_SUCCESS;
    /* dispatch loop */
    Nak_Routine_Param* param = ( Nak_Routine_Param* )arg;
    pgm_sock_t* nak_sock = param->sock;
    OpenPGMReliableSender* sender = param->sender;
#ifndef _WIN32
    int fds;
    fd_set readfds;
#else
    SOCKET recv_sock, repair_sock, pending_sock;
    DWORD cEvents = PGM_SEND_SOCKET_READ_COUNT + 1;
    WSAEVENT waitEvents[ PGM_SEND_SOCKET_READ_COUNT + 1 ];
    DWORD dwTimeout, dwEvents;
    socklen_t socklen = sizeof (SOCKET);

    //     waitEvents[0] = terminateEvent;
    waitEvents[1] = WSACreateEvent();
    waitEvents[2] = WSACreateEvent();
    waitEvents[3] = WSACreateEvent();
    pgm_getsockopt (nak_sock, IPPROTO_PGM, PGM_RECV_SOCK, &recv_sock, &socklen);
    WSAEventSelect (recv_sock, waitEvents[1], FD_READ);
    pgm_getsockopt (nak_sock, IPPROTO_PGM, PGM_REPAIR_SOCK, &repair_sock, &socklen);
    WSAEventSelect (repair_sock, waitEvents[2], FD_READ);
    pgm_getsockopt (nak_sock, IPPROTO_PGM, PGM_PENDING_SOCK, &pending_sock, &socklen);
    WSAEventSelect (pending_sock, waitEvents[3], FD_READ);
#endif /* !_WIN32 */
    do {
        struct timeval tv;
        char buf[4064];
        pgm_error_t* pgm_err = NULL;
        const int status = pgm_recv (nak_sock, buf, sizeof(buf), 0, NULL, &pgm_err);
        switch (status) {
        case PGM_IO_STATUS_TIMER_PENDING:
            {
                socklen_t optlen = sizeof (tv);
                pgm_getsockopt (nak_sock, IPPROTO_PGM, PGM_TIME_REMAIN, &tv, &optlen);
                //                 pgm_getsockopt (mSock, IPPROTO_PGM, PGM_TIME_REMAIN, &tv, &optlen);
            }
            goto block;
        case PGM_IO_STATUS_RATE_LIMITED:
            {
                socklen_t optlen = sizeof (tv);
                //                 pgm_getsockopt (mSock, IPPROTO_PGM, PGM_RATE_REMAIN, &tv, &optlen);
                pgm_getsockopt (nak_sock, IPPROTO_PGM, PGM_RATE_REMAIN, &tv, &optlen);
            }
        case PGM_IO_STATUS_WOULD_BLOCK:
block:
#ifndef _WIN32
            fds = terminate_pipe[0] + 1;
            FD_ZERO(&readfds);
            FD_SET(terminate_pipe[0], &readfds);
            pgm_select_info (nak_sock, &readfds, NULL, &fds);
            fds = select (fds, &readfds, NULL, NULL, PGM_IO_STATUS_WOULD_BLOCK == status ? NULL : &tv);
#else
            dwTimeout = PGM_IO_STATUS_WOULD_BLOCK == status ? WSA_INFINITE : (DWORD)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
            dwEvents = WSAWaitForMultipleEvents (cEvents, waitEvents, FALSE, dwTimeout, FALSE);
            switch (dwEvents) {
        case WSA_WAIT_EVENT_0+1: WSAResetEvent (waitEvents[1]); break;
        case WSA_WAIT_EVENT_0+2: WSAResetEvent (waitEvents[2]); break;
        case WSA_WAIT_EVENT_0+3: WSAResetEvent (waitEvents[3]); break;
        default: break;
            }
#endif /* !_WIN32 */
            break;

        default:
            if (pgm_err) {
                fprintf (stderr, "%s\n", pgm_err->message ? pgm_err->message : "(null)");
                pgm_error_free (pgm_err);
                pgm_err = NULL;
            }
            if (PGM_IO_STATUS_ERROR == status)
                break;
        }
    } while ( !sender->isToQuit() );
#ifndef _WIN32
    return NULL;
#else
    WSACloseEvent (waitEvents[1]);
    WSACloseEvent (waitEvents[2]);
    WSACloseEvent (waitEvents[3]);
    _endthread();
    return 0;
#endif
    return retval;
}

OpenPGMReliableSender::OpenPGMReliableSender():
mInitDone( false ),
mIsToQuit( false ),
mNetwork( OPENPGM_MULTICAST_ADDRESS ),
mPort( OPENPGM_DATA_DESTINATION_PORT ),
mUdpEncapPort( OPENPGM_USE_UDP_ENCAP_PORT ),
mMaxRte( OPENPGM_MAX_RTE ),
mUseFec( FALSE ),
mRsK( OPENPGM_RS_K ),
mRsN( OPENPGM_RS_N ),
mUseMulticastLoop( OPENPGM_USE_MULTICAST_LOOP ),
mSock( NULL ),
mMaxTpDu( OPENPGM_MAX_TPDU ),
mSqns( OPENPGM_SQNS ), 
m_no_router_assist( OPENPGM_NO_ROUTER_ASSIST ),
m_send_only( OPENPGM_SEND_ONLY ),
m_ambient_spm( OPENPGM_AMBIENT_SPM ),
m_nonblocking( OPENPGM_SENDER_NON_BLOCKING ),
m_multicast_hops( OPENPGM_MULTICAST_HOPS ),
m_dscp( OPENPGM_DSCP ),		/* Expedited Forwarding PHB for network elements, no ECN. */
mMaxODataRTE( ( OPENPGM_MAX_RTE == 0 ) ? OPENPGM_MAX_ODATA_RTE : ( (OPENPGM_MAX_ODATA_RTE < OPENPGM_MAX_RTE) ? OPENPGM_MAX_ODATA_RTE : OPENPGM_MAX_RTE ) ),
mNakThread( INVALID_HANDLE_VALUE )
{
}

OpenPGMReliableSender::~OpenPGMReliableSender()
{
    shutdown();
}

int OpenPGMReliableSender::initVar()
{
    mInitDone = false;
    mIsToQuit = false;
    mNetwork = OPENPGM_MULTICAST_ADDRESS;
    mPort = OPENPGM_DATA_DESTINATION_PORT;
    mUdpEncapPort = OPENPGM_USE_UDP_ENCAP_PORT;
    mMaxRte = OPENPGM_MAX_RTE;
    mUseFec = FALSE;
    mRsK = OPENPGM_RS_K;
    mRsN = OPENPGM_RS_N;
    mUseMulticastLoop = OPENPGM_USE_MULTICAST_LOOP;
    mSock = NULL;
    mMaxTpDu = OPENPGM_MAX_TPDU;
    mSqns = OPENPGM_SQNS;
    m_no_router_assist = OPENPGM_NO_ROUTER_ASSIST;
    m_send_only = OPENPGM_SEND_ONLY;
    m_ambient_spm = OPENPGM_AMBIENT_SPM;
    m_nonblocking = OPENPGM_SENDER_NON_BLOCKING;
    m_multicast_hops = OPENPGM_MULTICAST_HOPS;
    m_dscp = OPENPGM_DSCP;
    mMaxODataRTE = OPENPGM_MAX_ODATA_RTE;
    mNakThread = INVALID_HANDLE_VALUE;
    return PGM_SUCCESS;
}

int OpenPGMReliableSender::init()
{
    pgm_error_t* pgm_err = NULL;

    setlocale (LC_ALL, "");

    if (!pgm_init (&pgm_err)) 
    {
        cerr << "Unable to start PGM engine: " << pgm_err->message << endl;
        pgm_error_free (pgm_err);
        return PGM_FATAL;
    }
    mInitDone = true;
    return PGM_SUCCESS;
}

int OpenPGMReliableSender::shutdown()
{
    /* cleanup */
    if ( mSock ) 
    {
        pgm_close (mSock, TRUE);
        mSock = NULL;
    }
    pgm_shutdown();
    return PGM_SUCCESS;
}

int OpenPGMReliableSender::connect()
{
    int retval = PGM_FAILURE;
    if ( !mInitDone ) 
    {
        cerr << "pgm not initialized" << endl;
        return PGM_FATAL;
    }
    string userInput;

    do
    {
        // read input
        usage();
        //         getline( cin, userInput );
        //         retval = analyseOptions( userInput );
        //         if ( ( retval == PGM_INVALID_PARAMS ) || ( retval == PGM_FAILURE ) )
        //         {
        //             initVar();
        //             continue;
        //         }
        //         else if ( retval == PGM_FATAL )
        //             break;

        retval = createSocket();
        if ( retval == PGM_FATAL )
            break;
        else if ( retval != PGM_SUCCESS )
        {
            initVar();
            continue;
        }

        retval = createNakThread();
        if ( retval != PGM_SUCCESS )
        {
            break;
        }

    } while ( (retval != PGM_SUCCESS) && (mIsToQuit == false) );

    return retval;
}

int OpenPGMReliableSender::send()
{
    int retval = PGM_FAILURE;
    string userInput;
    int sCounter = 0;
    for (;;)
    {
        // expect a file name
        cout << "please input the full file name to send" << endl;
        cout << "for more than one files, separate the file names with \";\"" << endl;
        cout << "   -q to quit" << endl;
        int status = PGM_IO_STATUS_NORMAL;
        mIsToQuit = false;
        getline( cin, userInput );
        if ( userInput == "-q" )
        {
            status = pgm_send (mSock, "-q", strlen( "-q" ) + 1, NULL);
            mIsToQuit = true;
            retval = PGM_SUCCESS;
            break;
        }
        vector< string > filenames;
        filenames.push_back( "C:\\Users\\yli\\Downloads\\mouse.sso" );
        //         string separator(";");
        //         retval = PGMUtils::intoTokens( userInput, separator, false, filenames );
        //         if ( retval != PGM_SUCCESS )
        //             break;

        FILE* pFileToSend = NULL;
        char* buffer = new char[ OPENPGM_BUFFER_SIZE ];
        char cCounter[3];
        _itoa( sCounter, cCounter, 10 );
        char fileToWrite[10];
        strcpy( fileToWrite, "sent" );
        FILE* pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
        sCounter++;

        // send a "start" indicate a transfer session started
        status = pgm_send( mSock, "start", strlen( "start" ) + 1, NULL );

        for ( size_t i = 0; i < filenames.size(); i++ )
        {
            pFileToSend = fopen( filenames[i].c_str(), "rb" );
            if (pFileToSend == NULL) 
            {
                cerr << "Error opening file: " << filenames[i] << endl;
                continue;
            }
            fseek( pFileToSend, 0, SEEK_END );
            size_t fileSize = ftell( pFileToSend );
            size_t curPos = 0;
            size_t sizeToRead = OPENPGM_BUFFER_SIZE;
            rewind( pFileToSend );
            size_t readResult = 0;
            while ( !feof( pFileToSend ) && ( curPos < fileSize ) )
            {
                if ( fileSize - curPos < OPENPGM_BUFFER_SIZE )
                {
                    sizeToRead = fileSize - curPos;
                }

                readResult = fread( buffer, 1, sizeToRead, pFileToSend );
                if ( readResult != sizeToRead )
                {
                    cerr << "error reading file at: " << ftell(pFileToSend) << endl;
                    cerr << "sizeToRead:" << sizeToRead << ", actual read:" << readResult << endl;
                    cerr << "error code: " << ferror( pFileToSend ) << ", eof:" << feof( pFileToSend) << endl;
                }
                curPos += readResult;

                fwrite( buffer, 1, readResult, pFileToWrite );
                status = pgm_send (mSock, buffer, readResult, NULL);
                if (PGM_IO_STATUS_NORMAL != status) 
                {
                    fprintf (stderr, "pgm_send() failed.\n");
                }
                //                 Sleep( 500 );
            }
            fclose( pFileToSend );
            pFileToSend = NULL;
        }
        // send a "end" to indicate transfer session end
        status = pgm_send (mSock, "end", strlen( "end" ) + 1, NULL);
        fclose( pFileToWrite );
    }

    return retval;
}

int OpenPGMReliableSender::createSocket()
{
    int retval = PGM_FAILURE;
    struct pgm_addrinfo_t* res = NULL;
    pgm_error_t* pgm_err = NULL;
    sa_family_t sa_family = AF_UNSPEC;

    /* parse network parameter into PGM socket address structure */
    if (!pgm_getaddrinfo( mNetwork.c_str(), NULL, &res, &pgm_err ) )
    {
        fprintf (stderr, "Parsing network parameter: %s\n", pgm_err->message);
        goto err_abort;
    }

    sa_family = res->ai_send_addrs[0].gsr_group.ss_family;

    if ( mUdpEncapPort )
    {
        if (!pgm_socket (&mSock, sa_family, SOCK_SEQPACKET, IPPROTO_UDP, &pgm_err)) {
            fprintf (stderr, "Creating PGM/UDP socket: %s\n", pgm_err->message);
            goto err_abort;
        }
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_UDP_ENCAP_UCAST_PORT, &mUdpEncapPort, sizeof(mUdpEncapPort));
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_UDP_ENCAP_MCAST_PORT, &mUdpEncapPort, sizeof(mUdpEncapPort));
    } else {
        if (!pgm_socket (&mSock, sa_family, SOCK_SEQPACKET, IPPROTO_PGM, &pgm_err)) {
            fprintf (stderr, "Creating PGM/IP socket: %s\n", pgm_err->message);
            goto err_abort;
        }
    }

    /* Use RFC 2113 tagging for PGM Router Assist */
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_IP_ROUTER_ALERT, &m_no_router_assist, sizeof(m_no_router_assist));

    pgm_drop_superuser();

    /* set PGM parameters */
    const int m_heartbeat_spm[] = { pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (1300),
        pgm_secs  (7),
        pgm_secs  (16),
        pgm_secs  (25),
        pgm_secs  (30) };
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SEND_ONLY, &m_send_only, sizeof(m_send_only));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MTU, &mMaxTpDu, sizeof(mMaxTpDu));
    pgm_setsockopt( mSock, IPPROTO_PGM, PGM_ODATA_MAX_RTE, &mMaxODataRTE, sizeof( mMaxODataRTE ) );
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TXW_SQNS, &mSqns, sizeof(mSqns));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TXW_MAX_RTE, &mMaxRte, sizeof(mMaxRte));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_AMBIENT_SPM, &m_ambient_spm, sizeof(m_ambient_spm));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_HEARTBEAT_SPM, &m_heartbeat_spm, sizeof(m_heartbeat_spm));

#ifdef I_UNDERSTAND_PGMCC_AND_FEC_ARE_NOT_SUPPORTED
    if (use_pgmcc) {
        struct pgm_pgmccinfo_t pgmccinfo;
        pgmccinfo.ack_bo_ivl		= pgm_msecs (50);
        pgmccinfo.ack_c			= 75;
        pgmccinfo.ack_c_p		= 500;
        pgm_setsockopt (sock, IPPROTO_PGM, PGM_USE_PGMCC, &pgmccinfo, sizeof(pgmccinfo));
    }
 
    if (mUseFec) {
        struct pgm_fecinfo_t fecinfo; 
        fecinfo.block_size		= mRsN;
        fecinfo.proactive_packets	= proactive_packets;
        fecinfo.group_size		= mRsK;
        fecinfo.ondemand_parity_enabled	= use_ondemand_parity;
        fecinfo.var_pktlen_enabled	= TRUE;
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_USE_FEC, &fecinfo, sizeof(fecinfo));
    }
#endif
    /* create global session identifier */
    struct pgm_sockaddr_t addr;
    memset (&addr, 0, sizeof(addr));
    addr.sa_port = mPort ? mPort : OPENPGM_DATA_DESTINATION_PORT;
    addr.sa_addr.sport = DEFAULT_DATA_SOURCE_PORT;
    if (!pgm_gsi_create_from_hostname (&addr.sa_addr.gsi, &pgm_err)) {
        fprintf (stderr, "Creating GSI: %s\n", pgm_err->message);
        goto err_abort;
    }

    /* assign socket to specified address */
    struct pgm_interface_req_t if_req;
    memset (&if_req, 0, sizeof(if_req));
    if_req.ir_interface = res->ai_recv_addrs[0].gsr_interface;
    if_req.ir_scope_id  = 0;
    if (AF_INET6 == sa_family) {
        struct sockaddr_in6 sa6;
        memcpy (&sa6, &res->ai_recv_addrs[0].gsr_group, sizeof(sa6));
        if_req.ir_scope_id = sa6.sin6_scope_id;
    }
    if (!pgm_bind3 (mSock,
        &addr, sizeof(addr),
        &if_req, sizeof(if_req),	/* tx interface */
        &if_req, sizeof(if_req),	/* rx interface */
        &pgm_err))
    {
        fprintf (stderr, "Binding PGM socket: %s\n", pgm_err->message);
        goto err_abort;
    }

    /* join IP multicast groups */
    unsigned i;
    for (i = 0; i < res->ai_recv_addrs_len; i++)
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_JOIN_GROUP, &res->ai_recv_addrs[i], sizeof(struct group_req));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SEND_GROUP, &res->ai_send_addrs[0], sizeof(struct group_req));
    pgm_freeaddrinfo (res);

    /* set IP parameters */
    const int multicast_loop = mUseMulticastLoop ? 1 : 0;

    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_HOPS, &m_multicast_hops, sizeof(m_multicast_hops));
    if (AF_INET6 != sa_family)
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TOS, &m_dscp, sizeof(m_dscp));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NOBLOCK, &m_nonblocking, sizeof(m_nonblocking));

    //     RM_SEND_WINDOW  window;
    //     int    rc;
    //     window.RateKbitsPerSec = RATE_KBITS_PER_SEC;
    //     window.WindowSizeInMSecs = WINDOW_SIZE_IN_MSECS;
    //     window.WindowSizeInBytes = (RATE_KBITS_PER_SEC/8) * WINDOW_SIZE_IN_MSECS;
    //     pgm_setsockopt(mSock, IPPROTO_RM, RM_RATE_WINDOW_SIZE, (char *)&window, sizeof(window));

    if (!pgm_connect (mSock, &pgm_err)) {
        fprintf (stderr, "Connecting PGM socket: %s\n", pgm_err->message);
        goto err_abort;
    }

    return PGM_SUCCESS;

err_abort:
    if (NULL != mSock) {
        pgm_close (mSock, FALSE);
        mSock = NULL;
    }
    if (NULL != res) {
        pgm_freeaddrinfo (res);
        res = NULL;
    }
    if (NULL != pgm_err) {
        pgm_error_free (pgm_err);
        pgm_err = NULL;
    }
    return PGM_FATAL;
}

int OpenPGMReliableSender::createNakThread()
{
    int retval = PGM_FAILURE;
    do 
    {
        /* expect warning on MinGW due to lack of native uintptr_t */
        
        mNakThread = (HANDLE)_beginthreadex (NULL, 0, &nak_routine, mSock, 0, NULL);
        const int save_errno = errno;
        if (0 == mNakThread)
        {
            fprintf (stderr, "Creating new thread: %s\n", strerror (save_errno));
        }

    } while ( false );
    
    return PGM_SUCCESS;
}


void OpenPGMReliableSender::usage()
{
    fprintf (stderr, "Usage: %s [options]\n");
    fprintf (stderr, "  -n <network>    : Multicast group or unicast IP address\n");
    fprintf (stderr, "  -s <port>       : IP port\n");
    fprintf (stderr, "  -p <port>       : Encapsulate PGM in UDP on IP port\n");
    fprintf (stderr, "  -r <rate>       : Regulate to rate bytes per second\n");
    fprintf (stderr, "  -c              : Enable PGMCC\n");
    fprintf (stderr, "  -f <type>       : Enable FEC: proactive, ondemand, or both\n");
    fprintf (stderr, "  -N <n>          : Reed-Solomon block size (255)\n");
    fprintf (stderr, "  -K <k>          : Reed-Solomon group size (8)\n");
    fprintf (stderr, "  -P <count>      : Number of pro-active parity packets (h)\n");
    fprintf (stderr, "  -l              : Enable multicast loopback and address sharing\n");
    fprintf (stderr, "  -i              : List available interfaces\n");
}

// take the complete option string, analyse if the user input option is valid
int OpenPGMReliableSender::analyseOptions( string& options )
{
    const string supportedOptions("nsprfKNlih?q");
    int retval = PGM_FAILURE;
    do
    {
        map< char, string > optionPairs;

        retval = OpenPGMUtils::intoOptions( options, optionPairs );
        if ( retval != PGM_SUCCESS )
        {
            break;
        }

        map< char, string >::iterator itr = optionPairs.begin();
        while ( itr != optionPairs.end() )
        {
            string optionValue = itr->second;
            char optionSwitch = itr->first;
            switch ( optionSwitch )
            {
            case 'n':	
                mNetwork = optionValue; 
                break;
            case 's':
                mPort = atoi( optionValue.c_str() );
                break;
            case 'p':
                mUdpEncapPort = atoi( optionValue.c_str() );
                break;
            case 'r':
                mMaxRte = atoi( optionValue.c_str() );
                break;
            case 'f':
                mUseFec = TRUE; 
                break;
            case 'K':
                mRsK = atoi( optionValue.c_str() );
                break;
            case 'N':
                mRsN = atoi( optionValue.c_str() );
                break;
            case 'l':
                mUseMulticastLoop = TRUE;
                break;
            case 'i':
                pgm_if_print_all();
                break;
            case 'h':
            case '?':
                usage();
                break;
            case 'q':
                mIsToQuit = true;
                break;
            default:
                cout << "wrong parameter: -" << optionSwitch << " " << optionValue << endl;
                usage();
                break;
            }

            itr++;
        }

        retval = verifyOptions( optionPairs );
        if ( retval != PGM_SUCCESS )
        {
            break;
        }

        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int OpenPGMReliableSender::verifyOptions( std::map< char, std::string >& options )
{
    int retval = PGM_FAILURE;
    if ( mUseFec && ( !mRsN || !mRsK ) ) 
    {
        cerr << "Invalid Reed-Solomon parameters RS(" << mRsN << ", " << mRsK << ")" << endl;
        usage ();
        retval = PGM_INVALID_PARAMS;
    }
    else
        retval = PGM_SUCCESS;

    return retval;
}

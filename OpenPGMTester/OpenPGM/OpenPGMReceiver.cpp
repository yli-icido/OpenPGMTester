#include "stdafx.h"
#include "pgm/log.h"
#include "OpenPGMUtils.h"

#include "OpenPGMReceiver.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;

WSAEVENT OpenPGMReceiver::sTerminateEvent = INVALID_HANDLE_VALUE;
BOOL     OpenPGMReceiver::sIsTerminated = FALSE;

OpenPGMReceiver::OpenPGMReceiver() : 
mInitDone( false ),
mIsToQuit( false ),
mNetwork( OPENPGM_MULTICAST_ADDRESS ),
mPort( DEFAULT_DATA_DESTINATION_PORT ),
mUdpEncapPort( OPENPGM_USE_UDP_ENCAP_PORT ),
mUsePgmcc( FALSE ),
mUseFec( FALSE ),
mRsK( OPENPGM_RS_K ),
mRsN( OPENPGM_RS_N ),
mUseMulticastLoop( OPENPGM_USE_MULTICAST_LOOP ),
mSock( NULL ), 
mMaxTpDu( OPENPGM_MAX_TPDU ),
mSqns( OPENPGM_SQNS ),
m_no_router_assist( OPENPGM_NO_ROUTER_ASSIST ),
m_recv_only( OPENPGM_RECEIVE_ONLY ),
m_passive( OPENPGM_PASSIVE ),
m_peer_expiry( OPENPGM_PEER_EXPIRY_SECS ),
m_spmr_expiry( OPENPGM_SPMR_EXPIRY_MSECS ),
m_nak_bo_ivl( OPENPGM_NAK_BO_IVL_MSECS ),
m_nak_rpt_ivl( OPENPGM_NAK_RPT_IVL_SECS ),
m_nak_rdata_ivl( OPENPGM_NAK_RDATA_IVL_SECS ),
m_nak_data_retries( OPENPGM_NAK_DATA_RETRIES ),
m_nak_ncf_retries( OPENPGM_NAK_NCF_RETRIES ),
m_odata_max_rate( ( OPENPGM_MAX_RTE == 0 ) ? OPENPGM_MAX_ODATA_RTE : ( (OPENPGM_MAX_ODATA_RTE < OPENPGM_MAX_RTE) ? OPENPGM_MAX_ODATA_RTE : OPENPGM_MAX_RTE ) ),
m_nonblocking( OPENPGM_RECEIVER_NON_BLOCKING ),
m_multicast_hops( OPENPGM_MULTICAST_HOPS ),
m_dscp( OPENPGM_DSCP )		/* Expedited Forwarding PHB for network elements, no ECN. */
{
    memset( mWaitEvents, -1, sizeof(mWaitEvents) );
}

OpenPGMReceiver::~OpenPGMReceiver()
{
    shutdown();
}

int OpenPGMReceiver::initVar()
{
    mInitDone = false;
    mIsToQuit = false;
    mNetwork = OPENPGM_MULTICAST_ADDRESS;
    mPort = DEFAULT_DATA_DESTINATION_PORT;
    mUdpEncapPort = OPENPGM_USE_UDP_ENCAP_PORT;
    mUsePgmcc = FALSE;
    mUseFec = FALSE;
    mRsK = OPENPGM_RS_K;
    mRsN = OPENPGM_RS_N;
    mUseMulticastLoop = OPENPGM_USE_MULTICAST_LOOP;
    sTerminateEvent = INVALID_HANDLE_VALUE;
    sIsTerminated = FALSE;
    mSock = NULL;
    mMaxTpDu = OPENPGM_MAX_TPDU;
    mSqns = OPENPGM_SQNS;
    m_no_router_assist = OPENPGM_NO_ROUTER_ASSIST;
    m_recv_only = OPENPGM_RECEIVE_ONLY;
    m_passive = OPENPGM_PASSIVE;
    m_peer_expiry = OPENPGM_PEER_EXPIRY_SECS;
    m_spmr_expiry = OPENPGM_SPMR_EXPIRY_MSECS;
    m_nak_bo_ivl = OPENPGM_NAK_BO_IVL_MSECS;
    m_nak_rpt_ivl = OPENPGM_NAK_RPT_IVL_SECS;
    m_nak_rdata_ivl = OPENPGM_NAK_RDATA_IVL_SECS;
    m_nak_data_retries = OPENPGM_NAK_DATA_RETRIES;
    m_nak_ncf_retries = OPENPGM_NAK_NCF_RETRIES;
    // if MAX_RTE is 0, use MAX_ODATA_RTE, if MAX_RTE is not 0, max odata rate should not be larger than MAX_RTE
    m_odata_max_rate =  ( OPENPGM_MAX_RTE == 0 ) ? OPENPGM_MAX_ODATA_RTE : ( (OPENPGM_MAX_ODATA_RTE < OPENPGM_MAX_RTE) ? OPENPGM_MAX_ODATA_RTE : OPENPGM_MAX_RTE ) ;
    m_nonblocking = OPENPGM_RECEIVER_NON_BLOCKING;
    m_multicast_hops = OPENPGM_MULTICAST_HOPS;
    m_dscp = OPENPGM_DSCP;		/* Expedited Forwarding PHB for network elements, no ECN. */

    return PGM_SUCCESS;
}

void OpenPGMReceiver::usage()
{
    fprintf (stderr, "Usage: receive [options]\n");
    fprintf (stderr, "  -n <network>    : Multicast group or unicast IP address\n");
    fprintf (stderr, "  -s <port>       : IP port\n");
    fprintf (stderr, "  -p <port>       : Encapsulate PGM in UDP on IP port\n");
    fprintf (stderr, "  -c              : Enable PGMCC\n");
    fprintf (stderr, "  -f <type>       : Enable FEC with either proactive or ondemand parity\n");
    fprintf (stderr, "  -K <k>          : Configure Reed-Solomon code (n, k)\n");
    fprintf (stderr, "  -N <n>\n");
    fprintf (stderr, "  -l              : Enable multicast loopback and address sharing\n");
    fprintf (stderr, "  -i              : List available interfaces\n");
}

int OpenPGMReceiver::init()
{
    pgm_error_t* pgm_err = NULL;

    setlocale (LC_ALL, "");

    if (!pgm_init (&pgm_err)) {
        fprintf (stderr, "Unable to start PGM engine: %s\n", pgm_err->message);
        pgm_error_free (pgm_err);
        return PGM_FATAL;
    }

    mInitDone = true;
    return PGM_SUCCESS;
}

int OpenPGMReceiver::connect()
{
    int retval = PGM_FAILURE;
    if ( !mInitDone ) 
    {
        fprintf(stderr, "pgm not initialized\n");
        return PGM_FATAL;
    }

    pgm_error_t* pgm_err = NULL;
//     char userInputc[256];
    do
    {
        // read input
        usage();
//         gets( userInputc );
//         retval = analyseOptions( userInput );
//         if ( ( retval == PGM_INVALID_PARAMS ) || ( retval == PGM_FAILURE ) )
//         {
//             initVar();
//             continue;
//         }
//         else if ( retval == PGM_FATAL )
//             break;

        sTerminateEvent = WSACreateEvent();
        SetConsoleCtrlHandler( ( PHANDLER_ROUTINE ) on_console_ctrl, TRUE );

        retval = onStartup();
        if ( retval != PGM_SUCCESS )
        {
            fprintf(stderr, "Startup failed\n");
            break;
        }

        SOCKET recv_sock, pending_sock;
        DWORD cEvents = PGM_RECV_SOCKET_READ_COUNT + 1;
        WSAEVENT waitEvents[ PGM_RECV_SOCKET_READ_COUNT + 1 ];
        socklen_t socklen = sizeof (SOCKET);

        waitEvents[0] = sTerminateEvent;
        waitEvents[1] = WSACreateEvent();
        waitEvents[2] = WSACreateEvent();

        pgm_getsockopt (mSock, IPPROTO_PGM, PGM_RECV_SOCK, &recv_sock, &socklen);
        WSAEventSelect (recv_sock, waitEvents[1], FD_READ);
        pgm_getsockopt (mSock, IPPROTO_PGM, PGM_PENDING_SOCK, &pending_sock, &socklen);
        WSAEventSelect (pending_sock, waitEvents[2], FD_READ);
        FILE* pFileToWrite = NULL;
        char fileToWrite[11];
        int rCounter = 0;
        char cCounter[3];

        do {
            struct timeval tv;
            DWORD dwTimeout, dwEvents;
			char* buffer = new char[ OPENPGM_BUFFER_SIZE ];
            size_t len;
            struct pgm_sockaddr_t from;
            socklen_t fromlen = sizeof (from);
            const int status = pgm_recvfrom (mSock,
                buffer,
                OPENPGM_BUFFER_SIZE,
                0,
                &len,
                &from,
                &fromlen,
                &pgm_err);
            switch (status) 
            {
            case PGM_IO_STATUS_NORMAL:
                onData (buffer, len, &from);
                if ( strcmp( buffer, "start" ) == 0 )
                {
					printf ("start\n");
                    if ( pFileToWrite != NULL )
                    {
                        fclose( pFileToWrite );
                        pFileToWrite = NULL;
                    }
                    strcpy( fileToWrite, "received" );
                    _itoa( rCounter, cCounter, 10 );
                    pFileToWrite = fopen( strcat( fileToWrite, cCounter ), "w" );
                    rCounter++;
                }
                else if ( strcmp( buffer, "end" ) == 0 )
                {
					printf("end\n");
                    fclose( pFileToWrite );
                    pFileToWrite = NULL;
                }
				else if ( strcmp( buffer, "-q" ) == 0 )
				{
					if ( pFileToWrite != NULL )
					{
						fclose( pFileToWrite );
						pFileToWrite = NULL;
					}
					sIsTerminated = TRUE;
				}
                else if ( pFileToWrite != NULL )
                {
                    fwrite( buffer, 1, len, pFileToWrite );
                }
                else
                {
                    fprintf(stderr, "output file not open\n");
                }
                break;
            case PGM_IO_STATUS_TIMER_PENDING:
                {
                    socklen_t optlen = sizeof (tv);
                    pgm_getsockopt (mSock, IPPROTO_PGM, PGM_TIME_REMAIN, &tv, &optlen);
                }
                // fall through
            case PGM_IO_STATUS_WOULD_BLOCK:
                /* select for next event */
                dwTimeout = PGM_IO_STATUS_WOULD_BLOCK == status ? WSA_INFINITE : (DWORD)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
                dwEvents = WSAWaitForMultipleEvents (cEvents, waitEvents, FALSE, dwTimeout, FALSE);
                switch (dwEvents) {
                case WSA_WAIT_EVENT_0+1: WSAResetEvent (waitEvents[1]); break;
                case WSA_WAIT_EVENT_0+2: WSAResetEvent (waitEvents[2]); break;
                default: break;
                }
                break;

            case PGM_IO_STATUS_RATE_LIMITED:
                {
                    socklen_t optlen = sizeof (tv);
                    pgm_getsockopt (mSock, IPPROTO_PGM, PGM_RATE_REMAIN, &tv, &optlen);
                }

            default:
				printf("status: %d\n", status);
                if (pgm_err) {
                    fprintf (stderr, "%s\n", pgm_err->message);
                    pgm_error_free (pgm_err);
                    pgm_err = NULL;
                }
                if (PGM_IO_STATUS_ERROR == status)
                    break;
            }
        } while ( !sIsTerminated );

        retval = PGM_SUCCESS;

    } while ( false );

    return retval;
}

int OpenPGMReceiver::shutdown()
{
    puts ("Message loop terminated, cleaning up.");

    /* cleanup */
    WSACloseEvent (mWaitEvents[0]);
    WSACloseEvent (mWaitEvents[1]);
    WSACloseEvent (mWaitEvents[2]);

    if (mSock) 
    {
        puts ("Destroying PGM socket.");
        pgm_close (mSock, TRUE);
        mSock = NULL;
    }

    puts ("PGM engine shutdown.");
    pgm_shutdown ();
    puts ("finished.");
    return PGM_SUCCESS;
}

// take the complete option string, analyse if the user input option is valid
int OpenPGMReceiver::analyseOptions( string& options )
{
    const string supportedOptions("snpcfKNlih?q");
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
                mPort = atoi (optionValue.c_str()); 
                break;
            case 'p':
                mUdpEncapPort = atoi (optionValue.c_str()); 
                break;
            case 'c':
                mUsePgmcc = TRUE; 
                break;
            case 'f':
                mUseFec = TRUE; 
                break;
            case 'K':
                mRsK = atoi (optionValue.c_str());
                break;
            case 'N':
                mRsN = atoi (optionValue.c_str());
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

int OpenPGMReceiver::verifyOptions( std::map< char, std::string >& options )
{
    int retval = PGM_FAILURE;
    if (mUseFec && ( !mRsN || !mRsK )) {
        fprintf (stderr, "Invalid Reed-Solomon parameters RS(%d,%d).\n", mRsN, mRsK);
        usage ();
        retval = PGM_INVALID_PARAMS;
    }
    else
        retval = PGM_SUCCESS;

    return retval;
}

BOOL OpenPGMReceiver::on_console_ctrl( DWORD dwCtrlType )
{
    printf ("on_console_ctrl (dwCtrlType:%lu)\n", (unsigned long)dwCtrlType);
    sIsTerminated = TRUE;
    WSASetEvent (sTerminateEvent);
    return TRUE;
}

int OpenPGMReceiver::onStartup()
{
    struct pgm_addrinfo_t* res = NULL;
    pgm_error_t* pgm_err = NULL;
    sa_family_t sa_family = AF_UNSPEC;

    /* parse network parameter into PGM socket address structure */
    if (!pgm_getaddrinfo (mNetwork.c_str(), NULL, &res, &pgm_err)) {
        fprintf (stderr, "Parsing network parameter: %s\n", pgm_err->message);
        goto err_abort;
    }

    sa_family = res->ai_send_addrs[0].gsr_group.ss_family;

    if ( mUdpEncapPort )
    {
        puts ("Create PGM/UDP socket.");
        if (!pgm_socket (&mSock, sa_family, SOCK_SEQPACKET, IPPROTO_UDP, &pgm_err)) 
        {
            fprintf (stderr, "Creating PGM/UDP socket: %s\n", pgm_err->message);
            goto err_abort;
        }
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_UDP_ENCAP_UCAST_PORT, &mUdpEncapPort, sizeof(mUdpEncapPort));
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_UDP_ENCAP_MCAST_PORT, &mUdpEncapPort, sizeof(mUdpEncapPort));
    }
    else 
    {
        puts ("Create PGM/IP socket.");
        if (!pgm_socket (&mSock, sa_family, SOCK_SEQPACKET, IPPROTO_PGM, &pgm_err)) 
        {
            fprintf (stderr, "Creating PGM/IP socket: %s\n", pgm_err->message);
            goto err_abort;
        }
    }

    /* Use RFC 2113 tagging for PGM Router Assist */
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_IP_ROUTER_ALERT, &m_no_router_assist, sizeof(m_no_router_assist));

    pgm_drop_superuser();

    /* set PGM parameters */
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_RECV_ONLY, &m_recv_only, sizeof(m_recv_only));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_PASSIVE, &m_passive, sizeof(m_passive));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MTU, &mMaxTpDu, sizeof(mMaxTpDu));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_RXW_SQNS, &mSqns, sizeof(mSqns));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_PEER_EXPIRY, &m_peer_expiry, sizeof(m_peer_expiry));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SPMR_EXPIRY, &m_spmr_expiry, sizeof(m_spmr_expiry));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_BO_IVL, &m_nak_bo_ivl, sizeof(m_nak_bo_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_RPT_IVL, &m_nak_rpt_ivl, sizeof(m_nak_rpt_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_RDATA_IVL, &m_nak_rdata_ivl, sizeof(m_nak_rdata_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_DATA_RETRIES, &m_nak_data_retries, sizeof(m_nak_data_retries));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_NCF_RETRIES, &m_nak_ncf_retries, sizeof(m_nak_ncf_retries));
 	pgm_setsockopt (mSock, IPPROTO_PGM, PGM_ODATA_MAX_RTE, &m_odata_max_rate, sizeof(m_odata_max_rate));

#ifdef I_UNDERSTAND_PGMCC_AND_FEC_ARE_NOT_SUPPORTED
    if (use_pgmcc) {
        struct pgm_pgmccinfo_t pgmccinfo;
        pgmccinfo.ack_bo_ivl 		= pgm_msecs (50);
        pgmccinfo.ack_c			= 75;
        pgmccinfo.ack_c_p		= 500;
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_USE_PGMCC, &pgmccinfo, sizeof(pgmccinfo));
    }
    if (use_fec) {
        struct pgm_fecinfo_t fecinfo;
        fecinfo.block_size		= rs_n;
        fecinfo.proactive_packets	= 0;
        fecinfo.group_size		= rs_k;
        fecinfo.ondemand_parity_enabled	= TRUE;
        fecinfo.var_pktlen_enabled	= FALSE;
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_USE_FEC, &fecinfo, sizeof(fecinfo));
    }
#endif

    /* create global session identifier */
    struct pgm_sockaddr_t addr;
    memset (&addr, 0, sizeof(addr));
    addr.sa_port = mPort ? mPort : DEFAULT_DATA_DESTINATION_PORT;
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
    for (unsigned i = 0; i < res->ai_recv_addrs_len; i++)
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

    if (!pgm_connect (mSock, &pgm_err)) {
        fprintf (stderr, "Connecting PGM socket: %s\n", pgm_err->message);
        goto err_abort;
    }

    puts ("Startup complete.");
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
    if (NULL != mSock) {
        pgm_close (mSock, FALSE);
        mSock = NULL;
    }
    return PGM_FATAL;
}

int OpenPGMReceiver::onData( const void* restrict data, const size_t len, const struct pgm_sockaddr_t* restrict from )
{
    /* protect against non-null terminated strings */
//     fprintf(stderr, "data received, size: %d\n", len);

    char tsi[PGM_TSISTRLEN];
//     const size_t buflen = MIN(sizeof(buf) - 1, len);
// #ifndef CONFIG_HAVE_SECURITY_ENHANCED_CRT
//     strncpy (buf, (const char*)data, buflen);
//     buf[buflen] = '\0';
// #else
//     strncpy_s (buf, buflen, (const char*)data, _TRUNCATE);
// #endif
    pgm_tsi_print_r (&from->sa_addr, tsi, sizeof(tsi));
    /* Microsoft CRT will crash on %zu */
	SYSTEMTIME sysTime;
	GetSystemTime( &sysTime );
	printf ("%lu bytes from %s, %2d:%2d:%2d:%d\n", (unsigned long)len, tsi, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
    return PGM_SUCCESS;
}


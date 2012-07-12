#include "stdafx.h"
#include "pgm/log.h"
#include "PGMUtils.h"

#include "PGMReceiver.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;

WSAEVENT PGMReceiver::sTerminateEvent = INVALID_HANDLE_VALUE;
BOOL     PGMReceiver::sIsTerminated = FALSE;

PGMReceiver::PGMReceiver() : 
mInitDone( false ),
mIsToQuit( false ),
mNetwork( PGM_MULTICAST_ADDRESS ),
mPort( 0 ),
mUdpEncapPort( 0 ),
mUsePgmcc( FALSE ),
mUseFec( FALSE ),
mRsK( 0 ),
mRsN( 0 ),
mUseMulticastLoop( FALSE ),
mSock( NULL ), 
mMaxTpDu( 1500 ),
mSqns( 100 )
{
    memset( mWaitEvents, -1, sizeof(mWaitEvents) );
}

PGMReceiver::~PGMReceiver()
{
    shutdown();
}

int PGMReceiver::initVar()
{
    mInitDone = false;
    mIsToQuit = false;
    mNetwork = PGM_MULTICAST_ADDRESS;
    mPort = 0;
    mUdpEncapPort = 0;
    mUsePgmcc = FALSE;
    mUseFec = FALSE;
    mRsK = 0;
    mRsN = 0;
    mUseMulticastLoop = false;
    sTerminateEvent = INVALID_HANDLE_VALUE;
    sIsTerminated = FALSE;
    mSock = NULL;
    mMaxTpDu = 1500;
    mSqns = 100;
    return PGM_SUCCESS;
}

void PGMReceiver::usage()
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

int PGMReceiver::init()
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

int PGMReceiver::connect()
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
			char* buffer = new char[ PACKAGE_SIZE ];
            size_t len;
            struct pgm_sockaddr_t from;
            socklen_t fromlen = sizeof (from);
            const int status = pgm_recvfrom (mSock,
                buffer,
                PACKAGE_SIZE,
                0,
                &len,
                &from,
                &fromlen,
                &pgm_err);
			printf("status: %d\n", status);
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

int PGMReceiver::shutdown()
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
int PGMReceiver::analyseOptions( string& options )
{
    const string supportedOptions("snpcfKNlih?q");
    int retval = PGM_FAILURE;
    do
    {
        map< char, string > optionPairs;

        retval = PGMUtils::intoOptions( options, optionPairs );
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

int PGMReceiver::verifyOptions( std::map< char, std::string >& options )
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

BOOL PGMReceiver::on_console_ctrl( DWORD dwCtrlType )
{
    printf ("on_console_ctrl (dwCtrlType:%lu)\n", (unsigned long)dwCtrlType);
    sIsTerminated = TRUE;
    WSASetEvent (sTerminateEvent);
    return TRUE;
}

int PGMReceiver::onStartup()
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
    const int no_router_assist = 0;
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_IP_ROUTER_ALERT, &no_router_assist, sizeof(no_router_assist));

    pgm_drop_superuser();

    /* set PGM parameters */
    const int recv_only = 1,
        passive = 0,
        peer_expiry = pgm_secs (300),
        spmr_expiry = pgm_msecs (250),
        nak_bo_ivl = pgm_msecs (50),
        nak_rpt_ivl = pgm_secs (2),
        nak_rdata_ivl = pgm_secs (2),
        nak_data_retries = 50,
        nak_ncf_retries = 50;

    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_RECV_ONLY, &recv_only, sizeof(recv_only));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_PASSIVE, &passive, sizeof(passive));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MTU, &mMaxTpDu, sizeof(mMaxTpDu));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_RXW_SQNS, &mSqns, sizeof(mSqns));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_PEER_EXPIRY, &peer_expiry, sizeof(peer_expiry));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SPMR_EXPIRY, &spmr_expiry, sizeof(spmr_expiry));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_BO_IVL, &nak_bo_ivl, sizeof(nak_bo_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_RPT_IVL, &nak_rpt_ivl, sizeof(nak_rpt_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_RDATA_IVL, &nak_rdata_ivl, sizeof(nak_rdata_ivl));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_DATA_RETRIES, &nak_data_retries, sizeof(nak_data_retries));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NAK_NCF_RETRIES, &nak_ncf_retries, sizeof(nak_ncf_retries));

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
    const int nonblocking = 1,
        multicast_loop = mUseMulticastLoop ? 1 : 0,
        multicast_hops = 16,
        dscp = 0x2e << 2;		/* Expedited Forwarding PHB for network elements, no ECN. */

    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));
    if (AF_INET6 != sa_family)
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TOS, &dscp, sizeof(dscp));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NOBLOCK, &nonblocking, sizeof(nonblocking));

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

int PGMReceiver::onData( const void* restrict data, const size_t len, const struct pgm_sockaddr_t* restrict from )
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


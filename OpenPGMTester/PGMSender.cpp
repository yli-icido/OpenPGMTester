#include "stdafx.h"
#include "pgm/log.h"
#include "PGMUtils.h"

#include "PGMSender.h"
#pragma message (__FILE__ ": warning 4996 has been disableed" )
#pragma warning ( disable: 4996 )

using namespace std;


PGMSender::PGMSender():
mInitDone( false ),
mIsToQuit( false ),
mNetwork( PGM_MULTICAST_ADDRESS ),
mPort( 0 ),
mUdpEncapPort( 0 ),
mMaxRte( 0 ),
mUseFec( FALSE ),
mRsK( 0 ),
mRsN( 0 ),
mUseMulticastLoop( FALSE ),
mSock( NULL ),
mMaxTpDu( 1500 ),
mSqns( 100 )
{
}

PGMSender::~PGMSender()
{
    shutdown();
}

int PGMSender::initVar()
{
    mInitDone = false;
    mIsToQuit = false;
    mNetwork = PGM_MULTICAST_ADDRESS;
    mPort = 0;
    mUdpEncapPort = 0;
    mMaxRte = 0;
    mUseFec = FALSE;
    mRsK = 0;
    mRsN = 0;
    mUseMulticastLoop = FALSE;
    mSock = NULL;
    mMaxTpDu = 1500;
    mSqns = 100;
    return PGM_SUCCESS;
}

int PGMSender::init()
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

int PGMSender::shutdown()
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

int PGMSender::connect()
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
    } while ( (retval != PGM_SUCCESS) && (mIsToQuit == false) );
    
    return retval;
}

int PGMSender::send()
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
        getline( cin, userInput );
        if ( userInput == "-q" )
        {
            status = pgm_send (mSock, "-q", strlen( "-q" ) + 1, NULL);
            retval = PGM_SUCCESS;
            break;
        }
        vector< string > filenames;
        string separator(";");
        retval = PGMUtils::intoTokens( userInput, separator, false, filenames );
        if ( retval != PGM_SUCCESS )
            break;

        FILE* pFileToSend = NULL;
        const int bufferSize = 4096;
        char buffer[ bufferSize ];
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
            size_t sizeToRead = bufferSize;
            rewind( pFileToSend );
            size_t readResult = 0;
            while ( !feof( pFileToSend ) && ( curPos < fileSize ) )
            {
                if ( fileSize - curPos < bufferSize )
                {
                    sizeToRead = fileSize - curPos;
                }

                readResult = fread( buffer, 1, sizeToRead, pFileToSend );
                if ( readResult != sizeToRead )
                {
                    cerr << "error reading file at: " << ftell(pFileToSend) << endl;
                    cerr << "error code: " << ferror( pFileToSend ) << ", eof:" << feof( pFileToSend) << endl;
                }
                curPos += readResult;

                fwrite( buffer, 1, readResult, pFileToWrite );
                status = pgm_send (mSock, buffer, readResult, NULL);
                if (PGM_IO_STATUS_NORMAL != status) 
                {
                    fprintf (stderr, "pgm_send() failed.\n");
                }
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

int PGMSender::createSocket()
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
    const int no_router_assist = 0;
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_IP_ROUTER_ALERT, &no_router_assist, sizeof(no_router_assist));

    pgm_drop_superuser();

    /* set PGM parameters */
    const int send_only = 1,
        ambient_spm = pgm_secs (30),
        heartbeat_spm[] = { pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (100),
        pgm_msecs (1300),
        pgm_secs  (7),
        pgm_secs  (16),
        pgm_secs  (25),
        pgm_secs  (30) };

    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SEND_ONLY, &send_only, sizeof(send_only));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MTU, &mMaxTpDu, sizeof(mMaxTpDu));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TXW_SQNS, &mSqns, sizeof(mSqns));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TXW_MAX_RTE, &mMaxRte, sizeof(mMaxRte));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_AMBIENT_SPM, &ambient_spm, sizeof(ambient_spm));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_HEARTBEAT_SPM, &heartbeat_spm, sizeof(heartbeat_spm));
    if (mUseFec) {
        struct pgm_fecinfo_t fecinfo; 
        fecinfo.block_size		= mRsN;
        fecinfo.proactive_packets	= 0;
        fecinfo.group_size		= mRsK;
        fecinfo.ondemand_parity_enabled	= TRUE;
        fecinfo.var_pktlen_enabled	= TRUE;
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_USE_FEC, &fecinfo, sizeof(fecinfo));
    }

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
    unsigned i;
    for (i = 0; i < res->ai_recv_addrs_len; i++)
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_JOIN_GROUP, &res->ai_recv_addrs[i], sizeof(struct group_req));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_SEND_GROUP, &res->ai_send_addrs[0], sizeof(struct group_req));
    pgm_freeaddrinfo (res);

    /* set IP parameters */
    const int blocking = 0,
        multicast_loop = mUseMulticastLoop ? 1 : 0,
        multicast_hops = 16,
        dscp = 0x2e << 2;		/* Expedited Forwarding PHB for network elements, no ECN. */

    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_LOOP, &multicast_loop, sizeof(multicast_loop));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));
    if (AF_INET6 != sa_family)
        pgm_setsockopt (mSock, IPPROTO_PGM, PGM_TOS, &dscp, sizeof(dscp));
    pgm_setsockopt (mSock, IPPROTO_PGM, PGM_NOBLOCK, &blocking, sizeof(blocking));

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

void PGMSender::usage()
{
    cout << "Usage send: [options] message" << endl;
    cout << "  -n <network>    : Multicast group or unicast IP address" << endl;
    cout << "  -s <port>       : IP port" << endl;
    cout << "  -p <port>       : Encapsulate PGM in UDP on IP port" << endl;
    cout << "  -r <rate>       : Regulate to rate bytes per second" << endl;
    cout << "  -f <type>       : Enable FEC with either proactive or ondemand parity" << endl;
    cout << "  -K <k>          : Configure Reed-Solomon code (n, k)" << endl;
    cout << "  -N <n>" << endl;
    cout << "  -l              : Enable multicast loopback and address sharing" << endl;
    cout << "  -i              : List available interfaces" << endl;
    cout << "  -q              : Quit" << endl;
}

// take the complete option string, analyse if the user input option is valid
int PGMSender::analyseOptions( string& options )
{
    const string supportedOptions("nsprfKNlih?q");
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

int PGMSender::verifyOptions( std::map< char, std::string >& options )
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

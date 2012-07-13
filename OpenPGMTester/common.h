#ifndef PGM_COMMON_H_
#define PGM_COMMON_H_

const int PGM_FATAL = -1;           // error cause program to abort
const int PGM_SUCCESS = 0;
const int PGM_FAILURE = 1;          // failed
const int PGM_INVALID_PARAMS = 2;

// both
const int PGM_BUFFER_SIZE = 1024;
const std::string PGM_MULTICAST_ADDRESS = ";224.0.12.136";
const bool USE_UDP_ENCAP_PORT = false;
const int MAX_RTE = 60*1000*1000;
const int RS_K = 0;
const int RS_N = 0;
const int MAX_TPDU = 1500;
const int SQNS = 100;
const int USE_MULTICAST_LOOP = 0;
const int MULTICAST_HOPS = 16;
const int NO_ROUTER_ASSIST = 0;
const int MAX_ODATA_RTE = 30*1000*1000; // mbits
const int DSCP = 0x2e << 2;

// sender only
const int SEND_ONLY = 1;
const int SENDER_NON_BLOCKING = 0;
const int AMBIENT_SPM = pgm_secs(30);

// receiver only 
const int RECEIVE_ONLY = 1;
const int PASSIVE = 0;
const int PEER_EXPIRY_SECS = pgm_secs(300);
const int SPMR_EXPIRY_MSECS = pgm_msecs(250);
const int NAK_BO_IVL_MSECS = pgm_msecs(50);
const int NAK_RPT_IVL_SECS = pgm_secs(2);
const int NAK_RDATA_IVL_SECS = pgm_secs(2);
const int NAK_DATA_RETRIES = 50;
const int NAK_NCF_RETRIES = 50;
const int RECEIVER_NON_BLOCKING = 1;


#endif // PGM_COMMON_H_
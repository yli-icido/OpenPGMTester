#ifndef PGM_COMMON_H_
#define PGM_COMMON_H_

const int PGM_FATAL = -1;           // error cause program to abort
const int PGM_SUCCESS = 0;
const int PGM_FAILURE = 1;          // failed
const int PGM_INVALID_PARAMS = 2;
const int PGM_BUFFER_SIZE = 1024;
const std::string PGM_MULTICAST_ADDRESS = ";224.0.12.136";
const int PGM_PORT = 7500;
const bool UDP_ENCAP_PORT = false;
const int MAX_TPDU = 1500;
const int SQNS = 100000;
const int MULTICAST_LOOP = 1;
const int MULTICAST_HOPS = 16;
const int NO_ROUTER_ASSIST = 0;
const int RECV_ONLY = 1;
const int PASSIVE = 1;
const int PEER_EXPIRY_SECS = 300;
const int SPMR_EXPIRY_MSECS = 250;
const int NAK_BO_IVL_MSECS = 50;
const int NAK_RPT_IVL_MSECS = 2000;
const int NAK_RDATA_IVL_MSECS = 2000;
const int NAK_DATA_RETRIES = 50;
const int NAK_NCF_RETRIES = 50;


#endif // PGM_COMMON_H_
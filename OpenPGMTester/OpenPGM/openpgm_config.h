#ifndef OPENPGM_CONFIG_H_
#define OPENPGM_CONFIG_H_

#include "pgm/pgm.h"
// both
const int OPENPGM_BUFFER_SIZE = 1024;
const std::string OPENPGM_MULTICAST_ADDRESS = ";234.5.6.7";
const int OPENPGM_DATA_DESTINATION_PORT = 5150;
const bool OPENPGM_USE_UDP_ENCAP_PORT = false;
const int OPENPGM_MAX_RTE = 40 * 1000 * 1000; /* very conservative rate, 2.5mb/s */
// const int RS_K = 0;         // for simple sender
// const int RS_N = 0;         // for simple sender
const int OPENPGM_RS_K = 8;      // for reliable sender
const int OPENPGM_RS_N = 255;    // for reliable sender
const int OPENPGM_MAX_TPDU = 1500;
const int OPENPGM_SQNS = 5* 1000 * 1000;
const int OPENPGM_USE_MULTICAST_LOOP = 0;
const int OPENPGM_MULTICAST_HOPS = 16;
const int OPENPGM_NO_ROUTER_ASSIST = 0;
const int OPENPGM_DSCP = 0x2e << 2;
// const UINT32 PGM_TXW_MAX_RATE = 75 * 1000 * 1000;
const int OPENPGM_MAX_ODATA_RTE = 1*1000*1000; // mbits

// const size_t RATE_KBITS_PER_SEC = 50 * 1000;
// const size_t WINDOW_SIZE_IN_MSECS = 60 * 1000;

// sender only
const int OPENPGM_SEND_ONLY = 1;
const int OPENPGM_SENDER_NON_BLOCKING = 1;
const int OPENPGM_AMBIENT_SPM = pgm_secs(30);
const bool OPENPGM_USE_PGMCC = false;
const bool OPENPGM_USE_FEC = false;

const bool OPENPGM_USE_ONDEMAND_PARITY = FALSE;
const int OPENPGM_PROACTIVE_PACKETS = 0;

// receiver only 
const int OPENPGM_RECEIVE_ONLY = 1;
const int OPENPGM_PASSIVE = 0;
const int OPENPGM_PEER_EXPIRY_SECS = pgm_secs(300);
const int OPENPGM_SPMR_EXPIRY_MSECS = pgm_msecs(250);
const int OPENPGM_NAK_BO_IVL_MSECS = pgm_msecs(50);
const int OPENPGM_NAK_RPT_IVL_SECS = pgm_secs(2);
const int OPENPGM_NAK_RDATA_IVL_SECS = pgm_secs(2);
const int OPENPGM_NAK_DATA_RETRIES = 50;
const int OPENPGM_NAK_NCF_RETRIES = 50;
const int OPENPGM_RECEIVER_NON_BLOCKING = 1;


#endif // OPENPGM_CONFIG_H_
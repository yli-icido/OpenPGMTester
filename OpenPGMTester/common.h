#ifndef PGM_COMMON_H_
#define PGM_COMMON_H_

const int PGM_FATAL = -1;           // error cause program to abort
const int PGM_SUCCESS = 0;
const int PGM_FAILURE = 1;          // failed
const int PGM_INVALID_PARAMS = 2;

enum SENDER_TYPES {
    SIMPLE_OPEN_PGM_SENDER = 0,
    RELIABLE_OPEN_PGM_SENDER,
    MS_PGM_SENDER,
    TCP_SENDER,
    SENDER_TYPES_CNT
};

enum RECEIVER_TYPES {
    OPEN_PGM_RECEIVER = 0,
    MS_PGM_RECEIVER,
    TCP_RECEIVER,
    RECEIVER_TYPES_CNT
};
const std::string SENDER_TYPE_NAMES[SENDER_TYPES_CNT] = { "simple_openpgm", "reliable_openpgm", "mspgm", "tcp" };
const std::string RECEIVER_TYPE_NAMES[RECEIVER_TYPES_CNT] = { "openpgm", "mspgm", "tcp" };

#endif // PGM_COMMON_H_
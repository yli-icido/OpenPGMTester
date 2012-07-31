#ifndef PGM_COMMON_H_
#define PGM_COMMON_H_

const int PGM_FATAL = -1;           // error cause program to abort
const int PGM_SUCCESS = 0;
const int PGM_FAILURE = 1;          // failed
const int PGM_INVALID_PARAMS = 2;

const int SENDER_TYPES_NUM = 3;
const int RECEIVER_TYPES_NUM = 2;
const std::string SENDER_TYPES[SENDER_TYPES_NUM] = { "simple_openpgm", "reliable_openpgm", "mspgm" };
const std::string RECEIVER_TYPES[RECEIVER_TYPES_NUM] = { "openpgm", "mspgm" };

#endif // PGM_COMMON_H_
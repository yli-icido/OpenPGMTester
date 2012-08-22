#ifndef ACEMCAST_CONFIG_H_
#define ACEMCAST_CONFIG_H_

#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Dgram_Mcast.h>
#include <ace/High_Res_Timer.h>
#include <winsock2.h>
#include <Wsrm.h>

//==============================================================================
// SUPPRESS ACE WARNINGS begin
#ifdef _WIN32

// ace produces too many warnings, this define suppresses these warnings,
// but must be defined at the beginning of all includes
#define _CRT_SECURE_NO_WARNINGS

#pragma warning (disable : 4996) // The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name
#pragma warning (disable : 4244) // Conversion from 'ssize_t' in 'int', possible loss of data
#pragma warning (disable : 4267) // Conversion from 'ssize_t' in 'int', possible loss of data

#endif

//==============================================================================

#ifdef __clang__

#pragma clang diagnostic ignored "-Wconstant-logical-operand"
#pragma clang diagnostic ignored "-Wself-assign"

#endif
// SUPPRESS ACE WARNINGS end
//==============================================================================

// both
const int ACEMCAST_MESSAGE_LEN = 1024;
const std::string ACEMCAST_MULTICAST_GROUP = "234.5.6.7";
const int ACEMCAST_DATA_DESTINATION_PORT = 5150;
const int ACEMCAST_DELAY_BEFORE_END = 500;

#endif // ACEMCAST_CONFIG_H_
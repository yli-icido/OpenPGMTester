#ifndef TCP_CONFIG_H_
#define TCP_CONFIG_H_

/*
 If an #include line is needed for the Windows.h header file, this should be preceded with the 
 #define WIN32_LEAN_AND_MEAN macro. For historical reasons, the Windows.h header defaults to including 
 the Winsock.h header file for Windows Sockets 1.1. The declarations in the Winsock.h header file will 
 conflict with the declarations in the Winsock2.h header file required by Windows Sockets 2.0. 
 The WIN32_LEAN_AND_MEAN macro prevents the Winsock.h from being included by the Windows.h header. 
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
// The Winsock2.h header file contains most of the Winsock functions, structures, and definitions. 
#include <winsock2.h>
/*
 The Ws2tcpip.h header file contains definitions introduced in the WinSock 2 Protocol-Specific Annex document for 
 TCP/IP that includes newer functions and structures used to retrieve IP addresses. 
*/
#include <ws2tcpip.h>
/*
 The Iphlpapi.h header file is required if an application is using the IP Helper APIs. When the Iphlpapi.h header file
 is required, the #include line for the Winsock2.h header this file should be placed before the #include line for 
 the Iphlpapi.h header file. 
*/
#include <iphlpapi.h>

/*
 Ensure that the build environment links to the Winsock Library file Ws2_32.lib. 
 Applications that use Winsock must be linked with the Ws2_32.lib library file. 
 The #pragma comment indicates to the linker that the Ws2_32.lib file is needed. 
*/

#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

const int TCP_RECEIVER_CNT = 1;
const std::string TCP_RECEIVER_ADDR[TCP_RECEIVER_CNT] = { "10.0.128.23" };
const std::string TCP_SENDER_ADDR = "10.0.128.2";

const std::string TCP_PORT = "27015";

const int TCP_BUFFER_SIZE = 1024;

#endif // TCP_CONFIG_H_
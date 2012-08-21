#ifndef PGM_COMMON_H_
#define PGM_COMMON_H_

const int PGM_FATAL = -2;           // error cause program to abort
const int PGM_FAILURE = -1;         // failed
const int PGM_SUCCESS = 0;
const int PGM_INVALID_PARAMS = 1;

enum SENDER_TYPES {
    SIMPLE_OPEN_PGM_SENDER = 0,
    RELIABLE_OPEN_PGM_SENDER,
    MS_PGM_SENDER,
    TCP_SENDER,
    ACE_SENDER,
    SENDER_TYPES_CNT
};

enum RECEIVER_TYPES {
    OPEN_PGM_RECEIVER = 0,
    MS_PGM_RECEIVER,
    TCP_RECEIVER,
    ACE_RECEIVER,
    RECEIVER_TYPES_CNT
};
const std::string SENDER_TYPE_NAMES[SENDER_TYPES_CNT] = { "simple_openpgm", "reliable_openpgm", "mspgm", "tcp", "ace" };
const std::string RECEIVER_TYPE_NAMES[RECEIVER_TYPES_CNT] = { "openpgm", "mspgm", "tcp", "ace" };

// Statements like:
// #pragma message(Reminder "Fix this problem!")
// Which will cause messages like:
// C:\Source\Project\main.cpp(47): Reminder: Fix this problem!
// to show up during compiles. Note that you can NOT use the
// words "error" or "warning" in your reminders, since it will
// make the IDE think it should abort execution. You can double
// click on these messages and jump to the line in question.
#define STRINGIZE( L ) #L
#define MAKESTRING( M, L ) M(L)
#define $LINE MAKESTRING( STRINGIZE, __LINE__ )
#define REMINDER __FILE__ "(" $LINE ") : Reminder: "
// usage: #pragma message(Reminder "Fix this problem!")

// #define DISABLE_WARNING( x )   #pragma warning ( disable: ( x ))
// #pragma message (__FILE__ ": warning 4018 has been disableed" )

#endif // PGM_COMMON_H_
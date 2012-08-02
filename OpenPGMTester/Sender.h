#ifndef SENDER_H_
#define SENDER_H_


class Sender
{
public:
    virtual int     init() = 0;
    virtual int     connect() = 0;
    virtual int     send() = 0;
    virtual int     shutdown() = 0;
};
#endif // SENDER_H_
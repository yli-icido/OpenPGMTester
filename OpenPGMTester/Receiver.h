#ifndef RECEIVER_H_
#define RECEIVER_H_


class Receiver
{
public:
    virtual int     init() = 0;
    virtual int     connect() = 0;
    virtual int     receive() = 0;
    virtual int     shutdown() = 0;
};
#endif // RECEIVER_H_
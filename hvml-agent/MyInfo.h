#include "HttpEcho.h"

#define INFO_MESSAGE_LEN    4096
class MyInfo : public IHttpInfo
{
public:
    MyInfo(int listen_port);
    char* GetHttpInfo (int* info_len);

private:
    char info_message_[INFO_MESSAGE_LEN];
};
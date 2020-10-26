#include "HttpEcho.h"

class MyInfo : public IHttpInfo
{
public:
	MyInfo(int listen_port);
	char* GetHttpInfo (int* info_len);

private:
	char info_message_[4096];
};
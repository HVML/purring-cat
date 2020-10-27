#include "HttpEcho.h"
#include "MyInfo.h"

MyInfo::MyInfo(int listen_port)
    : IHttpInfo(listen_port)
{
}

char* MyInfo::GetHttpInfo (int* info_len) {

    char info_format[] = "[ INFO_1: %d  INFO_2: %d  INFO_3: %d ]";
    snprintf(info_message_, INFO_MESSAGE_LEN, info_format,
        info_1_,
        info_2_,
        info_3_);

    *info_len = strlen(info_message_);
    return info_message_;
}

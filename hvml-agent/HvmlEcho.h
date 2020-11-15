// This file is a part of Purring Cat, a reference implementation of HVML.
//
// Copyright (C) 2020, <liuxinouc@126.com>.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef _hvml_echo_h_
#define _hvml_echo_h_

#include "HttpServer.h"

#define INFO_MESSAGE_LEN    4096
class HvmlEcho : public IHttpResponse
{
public:
    HvmlEcho(int listen_port);
    char* GetHttpResponse (int* info_len,
                           const char* request);

private:
    char info_message_[INFO_MESSAGE_LEN];
};

#endif //_hvml_echo_h_
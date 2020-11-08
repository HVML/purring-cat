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
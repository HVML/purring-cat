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

#include "ace/Log_Msg.h"
#include "HttpEcho.h"
#include "MyInfo.h"

int ACE_TMAIN(int argc, char* argv[])
{
    MyInfo info(20000);
    HttpEcho::StartServer(&info);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Start HttpEcho, PORT: 20000\n")));

    printf("Press 'q' and ENTER to Exit\n");

    while (1) {
        int c = getchar();

        if (c == (int)'q') {
            break;
        }
        else {
            printf("your input is %c\n", c);
        }
    }

    HttpEcho::StopServer();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Stop HttpEcho\n")));
    return 0;
}

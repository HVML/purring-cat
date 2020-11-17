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
#include "interpreter/ext_tools.h"
#include "HvmlRuntime.h"
#include "HttpServer.h"
#include "HvmlEcho.h"

#include <stdlib.h> // function system

#define LISTEN_PORT 20000

static void wait_for_quit();
int ACE_TMAIN(int argc, char* argv[])
{
    if (argc != 2) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("arguments error\n")));
        return 0;
    }

    const char *file_in = argv[1];
    const char *fin_ext = file_ext(file_in);
    if (0 != strnicmp(fin_ext, ".hvml", 5)) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("input file name error\n")));
        return 0;
    }

    FILE *hvml_in_f = fopen(file_in, "rb");
    if (! hvml_in_f) {
        E("failed to open input file: %s", file_in);
        return 1;
    }

    HvmlRuntime runtime(hvml_in_f);

    HvmlEcho http_echo(LISTEN_PORT, runtime);
    HttpServer::StartServer(&http_echo);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Start HttpEcho\n")));

    // start firefox
    char cmdline[64];
    snprintf(cmdline, sizeof cmdline, "firefox localhost:%d/index", LISTEN_PORT);
    system (cmdline); // system function would not return until Firefox shut down.

    // wait_for_quit();

    HttpServer::StopServer();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Stop HttpEcho\n\n")));
    return 0;
}

static void wait_for_quit()
{
    printf("Press 'q' and ENTER to Exit\n");

    while (1) {
        int c = getchar();
        if (c == (int)'q') {
            break;
        }
        else {
            printf("your input is %c\n", c);
            printf("Press 'q' and ENTER to Exit\n");
        }
    }
}
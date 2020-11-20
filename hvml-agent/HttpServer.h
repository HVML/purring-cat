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

#ifndef _http_echo_h_
#define _http_echo_h_

#include "ace/ACE.h"
//#include "ace/init_ace.h" //use this only in windows

#include "ace/Thread.h"
#include "ace/Synch.h"
#include "ace/Reactor.h"
#include "ace/OS_main.h"

#include "ace/Log_Msg.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/Get_Opt.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_sys_socket.h"
#include "ace/os_include/os_netdb.h"

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_time.h"
#include "ace/Task.h"
#include "ace/Containers.h"
#include "ace/Synch.h"
#include "ace/SString.h"
#include "ace/Method_Request.h"
#include "ace/Future.h"
#include "ace/Activation_Queue.h"
#include "ace/Condition_T.h"

#include "ace/OS_NS_unistd.h"
#include "ace/streams.h"

#define ECMO_MESSAGE_LEN_MAX 4096
class IHttpResponse
{
public:
    IHttpResponse (int port) { listen_port_ = port; };

    virtual char* GetHttpResponse (int* info_len,
                                   const char* request) = 0;

    int GetListenPort(void) { return listen_port_; };

    int info_1_;
    int info_2_;
    int info_3_;

private:
    int listen_port_;
};

class Http_Listener : public ACE_Event_Handler
{
public:
    Http_Listener (IHttpResponse* ihr);
    virtual ~Http_Listener (void);

    ACE_HANDLE get_handle (void) const;
    virtual int handle_input (ACE_HANDLE handle);
    virtual int handle_close (ACE_HANDLE handle,
                              ACE_Reactor_Mask close_mask);

    ACE_INET_Addr local_address_;
    ACE_SOCK_Acceptor acceptor_;

private:
    IHttpResponse* ihr_;
};

class Http_Handler : public ACE_Event_Handler
{
public:
    /// Default constructor
    Http_Handler (ACE_SOCK_Stream &s, IHttpResponse* ihr);

    virtual ACE_HANDLE  get_handle (void) const;
    virtual int handle_input (ACE_HANDLE handle);
    virtual int handle_close (ACE_HANDLE handle,
                              ACE_Reactor_Mask close_mask);

    ACE_SOCK_Stream stream_;

private:
    IHttpResponse* ihr_;
};


class HttpServer
{
public:
    static void StartServer (IHttpResponse* ihr);
    static void StopServer (void);
};

#endif //_http_echo_h_
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

#include "HTTPU/parse_http_request.h"
#include "HttpServer.h"

Http_Listener::Http_Listener (IHttpResponse* ihr)
    : local_address_ (ihr->GetListenPort())
    , acceptor_ (local_address_, 1)
    , ihr_ (ihr)
{
    this->reactor (ACE_Reactor::instance ());
    int result = this->reactor ()->register_handler
        (this, ACE_Event_Handler::ACCEPT_MASK);
    ACE_TEST_ASSERT (result == 0);
}

Http_Listener::~Http_Listener (void)
{
}

ACE_HANDLE
Http_Listener::get_handle (void) const
{
    return this->acceptor_.get_handle ();
}

int
Http_Listener::handle_input (ACE_HANDLE handle)
{
    ACE_DEBUG ((LM_DEBUG, "Network_Listener::handle_input handle = %d\n", handle));

    ACE_INET_Addr remote_address;
    ACE_SOCK_Stream stream;

    // Try to find out if the implementation of the reactor that we are
    // using requires us to reset the event association for the newly
    // created handle. This is because the newly created handle will
    // inherit the properties of the listen handle, including its event
    // associations.
    int reset_new_handle = this->reactor ()->uses_event_associations ();

    int result = this->acceptor_.accept (stream, // stream
                                        &remote_address, // remote address
                                        0, // timeout
                                        1, // restart
                                        reset_new_handle);  // reset new handler
    ACE_TEST_ASSERT (result == 0);

    ACE_DEBUG ((LM_DEBUG, "Remote connection from: "));
    remote_address.dump ();

    Http_Handler *handler = 0;
    ACE_NEW_RETURN (handler, Http_Handler (stream, ihr_), -1);

    return 0;
}

int
Http_Listener::handle_close (ACE_HANDLE handle,
                             ACE_Reactor_Mask)
{
    ACE_DEBUG ((LM_DEBUG, "Network_Listener::handle_close handle = %d\n", handle));
    this->acceptor_.close ();
    delete this;
    return 0;
}





Http_Handler::Http_Handler (ACE_SOCK_Stream &s, IHttpResponse* ihr)
    : stream_ (s)
    , ihr_ (ihr)
{
    this->reactor (ACE_Reactor::instance ());

    int result = this->reactor ()->register_handler (this, READ_MASK);
    ACE_TEST_ASSERT (result == 0);
}

ACE_HANDLE
Http_Handler::get_handle (void) const
{
    return this->stream_.get_handle ();
}

int
Http_Handler::handle_input (ACE_HANDLE handle)
{
    ACE_DEBUG ((LM_DEBUG, "Network_Handler::handle_input handle = %d\n", handle));

    char message[BUFSIZ];
    int result = this->stream_.recv (message, sizeof message);
    if (result > 0)
    {
        message[result] = 0;
        ACE_DEBUG ((LM_DEBUG, "Remote message: %s\n", message));

        Parse_HTTP_Request http_req(message);
        const char *url = http_req.url();
        ACE_DEBUG ((LM_DEBUG, "\nhttp_req: %s\n\n", url));

        const char echo_format[] = 
            "HTTP/1.0 200 OK\r\n"\
            "Content-Type: application/json\r\n"\
            "Vary: Accept-Encoding\r\n"\
            "Content-Length: %d\r\n"\
            "\r\n%s";

        char echo_message[ECMO_MESSAGE_LEN_MAX];
        int response_len;
        char* response = ihr_->GetHttpResponse(&response_len, url);
        if (response_len < 0) {
            return -1;
        }
        if (response_len > ECMO_MESSAGE_LEN_MAX - sizeof(echo_format)) {
            return -1;
        }

        int echo_len = snprintf(echo_message,
                                ECMO_MESSAGE_LEN_MAX,
                                echo_format,
                                response_len,
                                response);

        this->stream_.send(echo_message, echo_len);
        return -1;
    }
    else if (result == 0)
    {
        ACE_DEBUG ((LM_DEBUG, "Connection closed\n"));
        return -1;
    }
    else if (errno == EWOULDBLOCK)
    {
        return 0;
    }
    else
    {
        ACE_DEBUG ((LM_DEBUG, "Problems in receiving data, result = %d", result));
        return -1;
    }
}

int
Http_Handler::handle_close (ACE_HANDLE handle,
                            ACE_Reactor_Mask)
{
    ACE_DEBUG ((LM_DEBUG, "Network_Handler::handle_close handle = %d\n", handle));

    this->stream_.close ();
    delete this;

    //ACE_Reactor::end_event_loop ();

    return 0;
}






static ACE_THR_FUNC_RETURN
worker_thread (void* param)
{
    IHttpResponse* ihr = (IHttpResponse*)param;

    Http_Listener *listener = new Http_Listener(ihr);
    ACE_UNUSED_ARG (listener);

    ACE_Reactor::instance()->run_reactor_event_loop();
    return 0;
}

void HttpServer::StartServer (IHttpResponse* ihr)
{
    ACE_Thread_Manager::instance ()->spawn
        (worker_thread, (void*)ihr);
}

void HttpServer::StopServer (void)
{
    ACE_Reactor::instance()->end_reactor_event_loop();
    ACE_Thread_Manager::instance ()->wait ();
}

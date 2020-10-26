#pragma once

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


class IHttpInfo
{
public:
	IHttpInfo (int port) { listen_port_ = port; };

	virtual char* GetHttpInfo (int* info_len) = 0;

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
	Http_Listener (IHttpInfo* ihi);
	virtual ~Http_Listener (void);

	ACE_HANDLE get_handle (void) const;
	virtual int handle_input (ACE_HANDLE handle);
	virtual int handle_close (ACE_HANDLE handle,
							  ACE_Reactor_Mask close_mask);

	ACE_INET_Addr local_address_;
	ACE_SOCK_Acceptor acceptor_;

private:
	IHttpInfo* ihi_;
};

class Http_Handler : public ACE_Event_Handler
{
public:
	/// Default constructor
	Http_Handler (ACE_SOCK_Stream &s, IHttpInfo* ihi);

	virtual ACE_HANDLE  get_handle (void) const;
	virtual int handle_input (ACE_HANDLE handle);
	virtual int handle_close (ACE_HANDLE handle,
							  ACE_Reactor_Mask close_mask);

	ACE_SOCK_Stream stream_;

private:
	IHttpInfo* ihi_;
};


class HttpEcho
{
public:
	static void StartServer (IHttpInfo* ihi);
	static void StopServer (void);
};

# HOW TO COMPILE AND INSTALL ACE IN LINUX OS

Reference: https://www.cnblogs.com/hehehaha/p/6332409.html
Download the ACE from: http://download.dre.vanderbilt.edu/
Download the file：ACE-6.5.11.tar.bz2

# Linux version: CentOS 8

1. Download and uncompress the ACE-6.5.11.tar.bz2 file at path /opt/ACE
 tar xfv ACE-6.5.11.tar.bz2

It will generate a new folder ACE_wrappers.

Config the environment variables:
 vi /etc/profile

Add these export statements in the tail.
 export ACE_ROOT=/opt/ACE/ACE_wrappers
 export MPC_ROOT=$ACE_ROOT/MPC
 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ACE_ROOT/ace:$ACE_ROOT/lib:/usr/local/lib

Save profile and executive command:
 source /etc/profile

2. Compile the ACE:

 cd $ACE_ROOT/ace

Create a new file config.h, and add a sentence:
 #include "config-linux.h"

 cd $ACE_ROOT/include/makeinclude

Create a new file platform_macros.GNU, and add a sentence:
 include $(ACE_ROOT)/include/makeinclude/platform_linux.GNU

 cd $ACE_ROOT
 make & make install

After these steps, you can see many .so files in the $ACE_ROOT/lib directory.

3. Write a simple Hello World program

3.1. Create a new hello.cpp file

    #include "ace/Log_Msg.h"
    int ACE_TMAIN(int argc, int argv[])
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("Hello World!\n")));
        return 0;
    }

3.2. Create a hello.mpc file

    project(hello) : aceexe {
        exename = hello
        Source_Files {
            hello.cpp
        }
        Header_Files {
        }
    }

3.3. Use MPC utility to generate Makefile
    $ACE_ROOT/bin/mpc.pl -type make hello.mpc

At this time, you can see that a new file Makefile.hello is generated.

3.4. Just make it
    make -f Makefile.hello

3.5. Run
    ./hello

Hello World!


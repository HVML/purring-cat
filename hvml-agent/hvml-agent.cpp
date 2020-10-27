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

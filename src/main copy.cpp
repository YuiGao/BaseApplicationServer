#include <iostream>
#include "wslog.h"
#include "client/linux/handler/exception_handler.h"

#include "DataHandleCore.h"

bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,void */*context*/,bool succeeded)
{
    printf("Dump path: %s\n", descriptor.path());
    return succeeded;
}

int main() {
    setbuf(stdout, NULL);
    google_breakpad::MinidumpDescriptor descriptor("./errors");
    google_breakpad::ExceptionHandler eh(descriptor,NULL,dumpCallback,NULL,true,-1);
    printf("hello word!\n");
    WS_DEBUG("helo");
    DataHandleCore* core = new DataHandleCore();
    core->start();
    while(true)
    {
        sleep(1);
    }
  //  core->test();

    return 0;
}

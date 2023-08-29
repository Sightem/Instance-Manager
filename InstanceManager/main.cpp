#include "InstanceManager.h"

int main()
{
    winrt::init_apartment();

    InstanceManager app;
    app.Run();
    return 0;
}

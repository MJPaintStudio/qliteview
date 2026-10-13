
//#include "third_party/libuv/include/uv.h"
#include "v8/include/v8.h"

typedef struct uv_loop_s uv_loop_t;
typedef enum { UV_RUN_DEFAULT = 0, UV_RUN_ONCE, UV_RUN_NOWAIT } uv_run_mode;

bool g_isElectronMode = false;





std::shared_ptr<v8::TaskRunner> nodePlatformGetForegroundTaskRunner(v8::Isolate* isolate)
{
    return nullptr;
}
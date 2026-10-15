#include "ecos/log.h"
#include "ffinit.h"

int main(void)
{
    if (!load_filesystem()) {
        (void)ECOS_LOGW("flash-fatfs", "No existing filesystem; formatting was not requested");
        return 1;
    }

    (void)ECOS_LOGI("flash-fatfs", "FatFs volume is ready for file operations");
    return 0;
}

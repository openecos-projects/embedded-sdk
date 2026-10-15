#include "ecos/driver/sdram.h"

#include "ecos/hal/sdram.h"

ecos_err_t ecos_sdram_init(void)
{
    int result = hal_sdram_init();
    if (result >= 0)
        return ECOS_OK;
    return ecos_err_is_known(result) ? (ecos_err_t)result : ECOS_ERR_IO;
}

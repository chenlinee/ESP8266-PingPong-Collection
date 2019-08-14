
#include <stdint.h>

#include "sys.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果需要使用OS,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif

/* gettimeofday() and getpid() */
int seed_from_timestamp_and_pid(uint32_t *seed) {
    *seed = SysTick->VAL;
    return 0;
}

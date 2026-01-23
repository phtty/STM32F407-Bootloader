#ifndef DRIVERS_BSP_JUMP_JUMP2APP
#define DRIVERS_BSP_JUMP_JUMP2APP

#include <stdint.h>
#include "config_info.h"

// 最大崩溃次数
#define MAX_CRASH_COUNT 3
// 强制升级标志
#define FLAG_FORCE_UPGRADE 0xdead

__attribute__((naked)) void execute_jump(uint32_t app_addr);
uint8_t Is_App_Exist(uint32_t app_addr);

#endif // DRIVERS_BSP_JUMP_JUMP2APP
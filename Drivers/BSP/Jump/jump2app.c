#include "jump2app.h"
#include "crc.h"
#include "iwdg.h"

// 为了栈上操作不越界，跳转函数必须全程使用汇编
__attribute__((naked)) void execute_jump(uint32_t app_addr)
{
    __asm__ volatile(
        // 1. r0 现在存放的是app_addr
        // 2. 加载 App 的栈顶地址到r1(从[r0]加载)
        "ldr r1, [r0]       \n"
        // 3. 加载App的Reset_Handler地址到r2(从[r0, #4]加载)
        "ldr r2, [r0, #4]   \n"
        // 4. 更新MSP(主堆栈指针)
        "msr msp, r1        \n"
        // 5. 指令同步屏障
        "isb                \n"
        // 6. 跳转到Reset_Handler
        "bx r2              \n"
        :
        : "r"(app_addr) // 虽然标记了输入，但在naked函数中通常直接处理r0
        : "r1", "r2", "memory");
}

/**
 * @brief 判断 App 是否存在（通过检查栈顶地址是否在RAM区间）
 */
uint8_t Is_App_Exist(uint32_t app_addr)
{
    uint32_t stack_ptr = *(__IO uint32_t *)app_addr;
    if ((stack_ptr <= 0x20000000 + 0x20000) && (stack_ptr >= 0x20000000))
        return 1;

    return 0;
}



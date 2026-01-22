#ifndef CORE_INC_JUMP2APP_H
#define CORE_INC_JUMP2APP_H

#include <stdint.h>

// 地址与常量定义
#define ADDR_CONFIG_SECTOR 0x08004000
#define ADDR_RECOVERY_APP  0x08008000
#define ADDR_MAIN_APP      0x08040000

#define FLAG_FORCE_UPGRADE 0xdead
#define MAX_CRASH_COUNT    3
#define CONFIG_MAGIC       0x0d000721

// 网络配置
typedef struct {
    uint8_t use_dhcp;
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t gw[4];
    uint16_t port;
} NetConfig_t;

// main app信息
typedef struct {
    uint32_t size;  // Main App的字节长度
    uint32_t crc32; // Main App的CRC32值
    char version[32];
} FWInfo_t;

// 定义在0x08004000(Sector 1)，并设置4字节对齐
__attribute__((aligned(4))) typedef struct {
    uint32_t magic;      // 魔数，判断配置区是否有效
    uint32_t update_sta; // 升级状态机
    FWInfo_t app_info;   // main_app状态
    NetConfig_t net_cfg; // 网络配置
    uint32_t config_crc; // 本结构体自身的校验
} SysInfo_t;

typedef enum {
    updated  = 0,
    updating = 1,
    failed   = 2,
} UpdateSta_t;

#define CONFIG_ADDR       0x08004000
#define MAIN_APP_ADDR     0x08040000
#define RECOVERY_APP_ADDR 0x08008000
#define MAGIC_NUMBER      0x0d000721

__attribute__((naked)) void execute_jump(uint32_t app_addr);
uint8_t Is_App_Exist(uint32_t app_addr);
uint8_t Is_Config_Empty(SysInfo_t *info);

#endif // !CORE_INC_JUMP2APP_H
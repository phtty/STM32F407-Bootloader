#ifndef DRIVERS_BSP_CONFIG_CONFIG_INFO_H
#define DRIVERS_BSP_CONFIG_CONFIG_INFO_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// #define DEBUG_MODE 1

// 地址与常量定义
#define ADDR_CONFIG_SECTOR 0x08004000
#define ADDR_RECOVERY_APP  0x08008000
#define ADDR_MAIN_APP      0x08040000

#define CONFIG_MAGIC       0x0d000721

// 网络配置
// 字段语义（方案 B，2026-08-21，TCP/UDP 端口彻底分离，与主固件
// app_board_net_cfg_t 及 Recovery NetConfig_t 二进制兼容）：
//   port     = TCP 业务口（LDI/IAP 生态，出厂默认 0x2538=9528）
//   udp_port = CQ UDP 业务口（出厂默认 20103）
__attribute__((aligned(4))) typedef struct {
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gw[4];
    uint32_t port;
    uint32_t udp_port;
} NetConfig_t;

// main app信息
__attribute__((aligned(4))) typedef struct {
    uint32_t size;  // Main App的字节长度
    uint32_t crc32; // Main App的CRC32值
    char version[32];
} FWInfo_t;

// 应该定义在0x08004000
__attribute__((aligned(4))) typedef struct {
    uint32_t magic;      // 魔数，判断配置区是否有效
    uint32_t update_sta; // 升级状态机
    FWInfo_t app_info;   // main_app状态
    NetConfig_t net_cfg; // 网络配置
    uint32_t config_crc; // 本结构体自身的校验
} SysInfo_t;

// 布局锁定：与主固件 app_board_net_cfg.h / Recovery config_info.h 二进制兼容
// （C11+ 编译期生效；Keil AC5 C90 下自动跳过）
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <stddef.h>
_Static_assert(sizeof(NetConfig_t) == 20, "NetConfig 布局 20B");
_Static_assert(sizeof(FWInfo_t) == 40, "FWInfo 布局 40B");
_Static_assert(sizeof(SysInfo_t) == 72, "SysInfo 布局 72B");
_Static_assert(offsetof(SysInfo_t, magic) == 0, "magic 偏移 0");
_Static_assert(offsetof(SysInfo_t, update_sta) == 4, "update_sta 偏移 4");
_Static_assert(offsetof(SysInfo_t, app_info) == 8, "app_info 偏移 8");
_Static_assert(offsetof(SysInfo_t, net_cfg) == 48, "net_cfg 偏移 48");
_Static_assert(offsetof(SysInfo_t, config_crc) == 68, "config_crc 偏移 68");
#endif

// 升级状态机
typedef enum {
    updated  = 0, // 升级成功
    updating = 1, // 升级中
    failed   = 2, // 升级失败
} UpdateSta_t;

bool Is_Config_Empty(volatile const SysInfo_t *info);
void Init_Config_Info(SysInfo_t *info);
void Edit_Config_Info(SysInfo_t *info);
HAL_StatusTypeDef EraseConfigInfo(void);
HAL_StatusTypeDef WriteConfigInfo(SysInfo_t *info);

#endif // !DRIVERS_BSP_CONFIG_CONFIG_INFO_H
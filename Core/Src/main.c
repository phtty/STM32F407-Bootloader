/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "iwdg.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "jump2app.h"
#include "config_info.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_CRC_Init();
    MX_IWDG_Init();
    MX_RTC_Init();
    /* USER CODE BEGIN 2 */

    uint32_t jump_target = ADDR_RECOVERY_APP; // 默认Recovery

    // 判断是否增加崩溃计数
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        uint32_t crash_cnt = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) + 1;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, crash_cnt);
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }

    // 读取崩溃计数和强制升级标志
    uint32_t force_flag  = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
    uint32_t crash_count = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    SysInfo_t *pConfig   = (SysInfo_t *)ADDR_CONFIG_SECTOR;

    force_flag = FLAG_FORCE_UPGRADE;        // 测试用，为了进恢复模式
    if (force_flag == FLAG_FORCE_UPGRADE) { // 条件A: 强制升级标志
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0);
        jump_target = ADDR_RECOVERY_APP;
        /*
         * 将config info中的升级状态机写成升级失败
         */
        SysInfo_t config_info = {0};
        memcpy(&config_info, pConfig, sizeof(config_info));
        config_info.update_sta = failed;
        Edit_Config_Info(&config_info);

    } else if (crash_count > MAX_CRASH_COUNT) { // 条件B: 崩溃过频
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
        jump_target = ADDR_RECOVERY_APP;
        /*
         * 将config info中的升级状态机写成升级失败
         */
        SysInfo_t config_info = {0};
        memcpy(&config_info, pConfig, sizeof(config_info));
        config_info.update_sta = failed;
        Edit_Config_Info(&config_info);

    } else if (Is_Config_Empty(pConfig)) { // 条件C: 出厂烧录状态
        /*
         * 初始化config info
         */
        SysInfo_t config_info = {0};
        Init_Config_Info(&config_info);

        if (Is_App_Exist(ADDR_MAIN_APP))
            jump_target = ADDR_MAIN_APP; // 首次运行
        else
            jump_target = ADDR_RECOVERY_APP; // 无Main

    } else { // 条件D: 校验Config和Main App的CRC
        // 首先校验Config Info自身的CRC32
        uint32_t calc_cfg_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)pConfig, (sizeof(pConfig) - sizeof(pConfig->config_crc)) / 4);

        if ((pConfig->magic == CONFIG_MAGIC) && (calc_cfg_crc == pConfig->config_crc)) {
            // Config有效，校验Main App实体
            uint32_t app_word_len = (pConfig->app_info.size + 3) / 4;
            uint32_t calc_app_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)ADDR_MAIN_APP, app_word_len);

            // 先判断升级状态机，状态机有误则直接进recovery
            do {
                if (pConfig->update_sta != updated) { // 升级未完成，需要进recovery处理
                    jump_target = ADDR_RECOVERY_APP;
                    break; // 通过break while实现提前跳出
                }

                if (calc_app_crc == pConfig->app_info.crc32 && Is_App_Exist(ADDR_MAIN_APP)) { // 校验通过
                    jump_target = ADDR_MAIN_APP;
                    break; // 通过break while实现提前跳出

                } else { // App损坏
                    jump_target = ADDR_RECOVERY_APP;
                    /*
                     * 将config info中的升级状态机写成升级失败
                     */
                    SysInfo_t config_info = {0};
                    memcpy(&config_info, pConfig, sizeof(config_info));
                    config_info.update_sta = failed;
                    Edit_Config_Info(&config_info);

                    break; // 通过break while实现提前跳出
                }
            } while (false);
        }
    }

    // 跳转前的清理工作
    HAL_IWDG_Refresh(&hiwdg);

    HAL_RCC_DeInit();
    HAL_DeInit();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    // 确认跳转地址存在栈指针
    if (Is_App_Exist(jump_target)) {
        __disable_irq();
        execute_jump(jump_target);
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    for (;;) { // 如果跳转失败则死循环触发看门狗
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.LSIState       = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    for (;;);
    /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

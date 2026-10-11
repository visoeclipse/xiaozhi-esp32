#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "application.h"

#define TAG "main"

/**
 * ESP32 固件入口函数
 *
 * 启动流程：
 * 1. 初始化 NVS（Non-Volatile Storage）- 存储 WiFi 密码等持久化数据
 * 2. 获取 Application 单例并执行初始化（显示屏/音频/网络/MCP）
 * 3. 进入主事件循环（永不返回）
 */
extern "C" void app_main(void)
{
    // 初始化 NVS 闪存分区，用于保存 WiFi 配置等数据
    // 如果 NVS 版本不兼容或没有空闲页，需要先擦除再重新初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Application 是全局单例，管理所有子系统的生命周期
    auto& app = Application::GetInstance();
    app.Initialize();
    app.Run();  // 主事件循环，阻塞在此处直到设备重启
}

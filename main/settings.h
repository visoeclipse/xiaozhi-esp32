#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <nvs_flash.h>

/**
 * Settings - NVS 持久化存储的便捷封装
 *
 * 提供类型安全的方式读写 ESP32 的 Non-Volatile Storage。
 * 支持字符串、整数和布尔值。
 *
 * 使用示例：
 *   Settings settings("wifi", true);           // 打开命名空间 "wifi"，读写模式
 *   settings.SetString("ssid", "MyNetwork");   // 写入
 *   std::string ssid = settings.GetString("ssid");  // 读取
 *
 * 析构时自动提交（如果模式为读写且有修改）。
 */
class Settings {
public:
    /**
     * 构造函数
     * @param ns 命名空间名称
     * @param read_write 是否以读写模式打开（默认只读）
     */
    Settings(const std::string& ns, bool read_write = false);
    /// 析构时自动提交修改并关闭 NVS 句柄
    ~Settings();

    /// 读取字符串，不存在时返回默认值
    std::string GetString(const std::string& key, const std::string& default_value = "");
    /// 写入字符串（需要读写模式）
    void SetString(const std::string& key, const std::string& value);
    /// 读取 32 位整数
    int32_t GetInt(const std::string& key, int32_t default_value = 0);
    /// 写入 32 位整数
    void SetInt(const std::string& key, int32_t value);
    /// 读取布尔值
    bool GetBool(const std::string& key, bool default_value = false);
    /// 写入布尔值
    void SetBool(const std::string& key, bool value);
    /// 删除指定键
    void EraseKey(const std::string& key);
    /// 删除当前命名空间下所有键值对
    void EraseAll();

private:
    std::string ns_;
    nvs_handle_t nvs_handle_ = 0;
    bool read_write_ = false;
    bool dirty_ = false;
};

#endif

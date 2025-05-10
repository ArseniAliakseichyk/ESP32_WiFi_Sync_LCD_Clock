#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "lcd/lcd.h"

#define WIFI_SSID "Arseni_Wi-Fi"
#define WIFI_PASS "01123581321"
#define SYNC_INTERVAL_MIN 30

static const char *TAG = "CLOCK";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static char buf_hour[3] = "";
static char buf_min[3] = "";
static char buf_sec[3] = "";
static char old_buf_hour[3] = "  ";
static char old_buf_min[3] = "  ";
static char old_buf_sec[3] = "  ";

static bool wifi_active = false;
static bool time_synced = false;
static time_t last_sync_time = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying WiFi connection...");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP address");
    }
}

void wifi_connect()
{
    if (wifi_active) return;

    wifi_event_group = xEventGroupCreate();
    
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_active = true;

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           60000 / portTICK_PERIOD_MS);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to WiFi");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    }
}

void wifi_disconnect()
{
    if (!wifi_active) return;
    
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_event_loop_delete_default();
    esp_netif_deinit();
    vEventGroupDelete(wifi_event_group);
    wifi_active = false;
    ESP_LOGI(TAG, "WiFi disconnected and deinitialized");
}

bool sync_time()
{
    if (!wifi_active) return false;

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int max_retry = 10;

    while (timeinfo.tm_year < (2023 - 1900) && ++retry < max_retry)
    {
        ESP_LOGI(TAG, "Waiting for time sync (%d/%d)", retry, max_retry);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == max_retry)
    {
        ESP_LOGW(TAG, "Failed to get time");
        return false;
    }
    
    setenv("TZ", "GMT-2", 1);
    tzset();
    
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Local time: %s", strftime_buf);
    
    last_sync_time = now;
    return true;
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    lcd_init();
    lcd_clear_screen(0x0000);

    int center_x = LCD_WIDTH / 2;
    int center_y = LCD_HEIGHT / 2;
    lcd_draw_text_fast(":", center_x - 5, center_y, 0xFFFF, 0x0000);
    lcd_draw_text_fast(":", center_x + 22, center_y, 0xFFFF, 0x0000);

    strcpy(old_buf_hour, "  ");
    strcpy(old_buf_min, "  ");
    strcpy(old_buf_sec, "  ");

    wifi_connect();
    if (sync_time()) {
        time_synced = true;
        wifi_disconnect();
    }

    while (1)
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        if (time_synced && (now - last_sync_time) > (SYNC_INTERVAL_MIN * 60))
        {
            ESP_LOGI(TAG, "Time for re-sync");
            wifi_connect();
            if (sync_time()) {
                wifi_disconnect();
            }
        }

        snprintf(buf_hour, sizeof(buf_hour), "%02d", timeinfo.tm_hour);
        snprintf(buf_min, sizeof(buf_min), "%02d", timeinfo.tm_min);
        snprintf(buf_sec, sizeof(buf_sec), "%02d", timeinfo.tm_sec);

        int hour_x = center_x - 30;
        int min_x = center_x + 5;
        int sec_x = center_x + 40;
        int y_pos = center_y;

        if (strcmp(buf_hour, old_buf_hour) != 0) {
            lcd_draw_text_fast(old_buf_hour, hour_x, y_pos, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_hour, hour_x, y_pos, 0xFFFF, 0x0000);
            strcpy(old_buf_hour, buf_hour);
        }
        
        if (strcmp(buf_min, old_buf_min) != 0) {
            lcd_draw_text_fast(old_buf_min, min_x, y_pos, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_min, min_x, y_pos, 0xFFFF, 0x0000);
            strcpy(old_buf_min, buf_min);
        }
        
        if (strcmp(buf_sec, old_buf_sec) != 0) {
            lcd_draw_text_fast(old_buf_sec, sec_x, y_pos, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_sec, sec_x, y_pos, 0xFFFF, 0x0000);
            strcpy(old_buf_sec, buf_sec);
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
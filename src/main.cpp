#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include "BluetoothSerial.h"

long sleepTime = 0;
const int wakeup_sec = 360; // wake up after 10 seconds

// ============================= wakeup reason
enum WakeupStatus
{
  WAKEUP_MODEM,
  WAKEUP_POWER,
  WAKEUP_TOUCHPAD,
  WAKEUP_ULP,
  WAKEUP_TIMER,
  WAKEUP_BUTTON1,
  WAKEUP_BUTTON2
};

WakeupStatus operation = WakeupStatus::WAKEUP_POWER;

// Buttons to wake up controller used in function wakeup reason.
#define BUTTON_PIN1 12
#define BUTTON_PIN2 14

void check_wake_up();
void goSleep();

// ============================= wakeup reason end

void setup()
{

  Serial.begin(9600);
  Serial.println("Start controller ...");
  check_wake_up();
  sleepTime = millis();
}

void loop()
{

  if (millis() - sleepTime > 5000)
  {
    Serial.println("Going to sleep ...");
    goSleep();
  }

  Serial.println("Main loop running ...");
  delay(1000);
}

// Function to put the controller to sleep
void goSleep()
{

  pinMode(GPIO_NUM_15, INPUT);
  pinMode(GPIO_NUM_12, INPUT);
  pinMode(GPIO_NUM_14, INPUT);

  // disable WiFi and Bluetooth
  esp_wifi_stop();
  esp_bt_controller_disable();
  BluetoothSerial SerialBT;
  SerialBT.end();

  // Isolate RTC GPIO, to prevent leaks and power consumption
  // (example for GPIO_NUM_15, GPIO_NUM_12, GPIO_NUM_14, in case you not use them to wakeup)
  // rtc_gpio_isolate(GPIO_NUM_15);
  // rtc_gpio_isolate(GPIO_NUM_12);
  // rtc_gpio_isolate(GPIO_NUM_14);

  rtc_gpio_isolate(GPIO_NUM_33);

  // disable RTC peripherals, fast and slow memory, XTAL
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

  // enable timer to wake up the controller
  esp_sleep_enable_timer_wakeup(wakeup_sec * 1000000ULL);

  Serial.flush(); // flush all serial data
  delay(200);

  esp_deep_sleep_start(); // go to sleep in that place
}

/**
 * Wake up reason detector
 *
 */

boolean isWakeupReason(WakeupStatus reason)
{
  return operation == reason;
}

void check_wake_up()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  if (ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason)
  {
    Serial.println("Waked up from ext0.");
    operation = WakeupStatus::WAKEUP_MODEM;
  }

  if (ESP_SLEEP_WAKEUP_TOUCHPAD == wakeup_reason)
  {
    Serial.println("Waked up from touchpad.");
    operation = WakeupStatus::WAKEUP_TOUCHPAD;
  }

  if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason)
  {
    Serial.println("Waked up from timer.");
    operation = WakeupStatus::WAKEUP_TIMER;
  }

  if (ESP_SLEEP_WAKEUP_ULP == wakeup_reason)
  {
    Serial.println("Waked up from ulp.");
    operation = WakeupStatus::WAKEUP_ULP;
  }
  if (ESP_SLEEP_WAKEUP_EXT1 == wakeup_reason)
  {
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    unsigned int gpio_pin_waked_up = (log(GPIO_reason)) / log(2);
    if (gpio_pin_waked_up == BUTTON_PIN1)
    {
      Serial.print("Button 1 triggered ... ");
      operation = WakeupStatus::WAKEUP_BUTTON1;
    }

    if (gpio_pin_waked_up == BUTTON_PIN2)
    {
      Serial.println("Button 2 triggered ... ");
      operation = WakeupStatus::WAKEUP_BUTTON2;
    }
  }

  if (operation == WakeupStatus::WAKEUP_POWER)
  {
    Serial.println("Wake up on power on");
  }
}

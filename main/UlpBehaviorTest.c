#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hulp.h"
#include "sdkconfig.h"
#include "soc/rtc.h"
#include <stdio.h>
#include <string.h>

#define millis() (esp_timer_get_time() / 1000)

#define WAKE_BUTTON_PIN 2

#define ULP_ARR_SIZE 100
#define ULP_DELAY_SECS 10

#define DEEP_SLEEP_AFTER_SECS 60
#define DEEP_SLEEP_DURATION_SECS 60

static const char *TAG = "ulp-test";

struct UlpMmts {
  ulp_var_t idx;  // index for our measurement arrays
  ulp_var_t runs; // number of ULP runs
  ulp_var_t ticksLow[ULP_ARR_SIZE];
  ulp_var_t ticksMid[ULP_ARR_SIZE];
  ulp_var_t ticksHigh[ULP_ARR_SIZE];
};

RTC_DATA_ATTR struct UlpMmts ulpData;

// convert RTC time to microseconds
static uint64_t rtcTicksToUs(uint64_t ticks) {
  if(ticks == 0) {
    ticks = rtc_time_get();
  }
  const uint32_t cal = esp_clk_slowclk_cal_get();
  const uint64_t ticksLow = ticks & UINT32_MAX;
  const uint64_t ticksHigh = ticks >> 32;
  return ((ticksLow * cal) >> RTC_CLK_CAL_FRACT) + ((ticksHigh * cal) << (32 - RTC_CLK_CAL_FRACT));
}

// convert raw ULP tick data to microseconds
static uint32_t ulpTicksToUs(uint16_t ticksLow, uint16_t ticksMid, uint16_t ticksHigh) {
  uint64_t rawTicks = (((uint64_t)(ticksHigh & 0xFFFF)) << 32) | ((ticksMid & 0xFFFF) << 16) | (ticksLow & 0xFFFF);
  return rtcTicksToUs(rawTicks);
}

static void printUlpData() {
  ESP_LOGI(TAG, "Printing %d ULP datapoints", ulpData.idx.val);
  uint32_t lastMillis = 0;
  for(int i = 0; i < ulpData.idx.val; i++) {
    uint32_t ulpMillis = ulpTicksToUs(ulpData.ticksLow[i].val, ulpData.ticksMid[i].val, ulpData.ticksHigh[i].val) / 1000;
    uint32_t delta = ulpMillis - lastMillis;
    ESP_LOGI(TAG, "ULP Data Index: %3d || Time: %8ums - %5.1fs || Delta: %8ums - %5.1fs", i, ulpMillis, (ulpMillis / 1000.0), delta, (delta / 1000.0));
    lastMillis = ulpMillis;
  }
}

static void enterDeepSleep() {
  ESP_LOGI(TAG, "Entering deep sleep for %ds", DEEP_SLEEP_DURATION_SECS);
  // Commenting out the ext0 wakeup source below restores expected
  // ULP delay timing
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_BUTTON_PIN, 0);
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION_SECS * 1000 * 1000); // arg value in microseconds
  esp_deep_sleep_start();
}

void initUlp(unsigned int delaySecs) {

  enum {
    LBL_RESET_IDX,
    LBL_INCREMENT_IDX,
  };

  const ulp_insn_t program[] = {
    // increment runs count
    I_MOVI(R1, 0),               // set R1 to zero
    I_GET(R0, R1, ulpData.runs), // load runs into R0
    I_ADDI(R0, R0, 1),           // R0 + 1
    I_PUT(R0, R1, ulpData.runs), // put R0 to runs var

    // load data array index offset into R2
    I_MOVI(R2, 0),              // set R2 to zero
    I_GET(R2, R2, ulpData.idx), // load index into R2

    // update ticks
    M_UPDATE_TICKS(),

    // write our tick values
    I_RD_TICKS_REG(0),            // read low ticks into R0
    I_MOVO(R1, ulpData.ticksLow), // load ticksLow into R1
    I_ADDR(R3, R1, R2),           // set R3 to be R1 + R2 (array offset)
    I_ST(R0, R3, 0),              // store value in R0 into address in R3

    I_RD_TICKS_REG(16),           // read middle ticks into R0
    I_MOVO(R1, ulpData.ticksMid), // load ticksMid into R1
    I_ADDR(R3, R1, R2),           // set R3 to be R1 + R2 (array offset)
    I_ST(R0, R3, 0),              // store value in R0 into address in R3

    I_RD_TICKS_REG(32),            // read high ticks into R0
    I_MOVO(R1, ulpData.ticksHigh), // load ticksHigh into R1
    I_ADDR(R3, R1, R2),            // set R3 to be R1 + R2 (array offset)
    I_ST(R0, R3, 0),               // store value in R0 into address in R3

    // branch to increment index offset, or reset if we're at the end, then
    // halt
    I_MOVR(R0, R2),                           // our offset is already in R2, but we need it in R0
    M_BGE(LBL_RESET_IDX, (ULP_ARR_SIZE - 1)), // branch to LBL_RESET_IDX if R0 >= (ULP_ARR_SIZE - 1)
    M_BX(LBL_INCREMENT_IDX),                  // else branch to label LBL_INCREMENT_IDX

    // branch reset index
    M_LABEL(LBL_RESET_IDX), I_MOVI(R2, 0), // set R2 to 0
    I_PUT(R2, R2, ulpData.idx),            // and set the offset to the value in R2
    I_HALT(),                              // halt

    // branch increment index
    M_LABEL(LBL_INCREMENT_IDX), I_MOVI(R2, 0), // set R2 to 0
    I_GET(R0, R2, ulpData.idx),                // read our offset into R0
    I_ADDI(R0, R0, 1),                         // increment R0
    I_PUT(R0, R2, ulpData.idx),                // store R0 value into offset var
    I_HALT(),                                  // halt
  };

  ESP_LOGI(TAG, "Starting ULP program with %ds delay...", delaySecs);

  int ulpProgDelay = delaySecs * 1000ULL * 1000;
  // alternate way to start ULP using HULP lib (no change in behavior)
  //ESP_ERROR_CHECK(hulp_ulp_load(program, sizeof(program), ulpProgDelay, 0));
  //ESP_ERROR_CHECK(hulp_ulp_run(0));
  size_t numWords = sizeof(program) / sizeof(ulp_insn_t);
  ESP_ERROR_CHECK(ulp_process_macros_and_load(0, program, &numWords));
  ESP_ERROR_CHECK(ulp_set_wakeup_period(0, ulpProgDelay));
  ESP_ERROR_CHECK(ulp_run(0));

  ESP_LOGI(TAG, "ULP started");
}

void app_main(void) {

  gpio_reset_pin(WAKE_BUTTON_PIN);
  gpio_set_direction(WAKE_BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(WAKE_BUTTON_PIN, GPIO_PULLUP_ONLY);

  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  switch(wakeupReason) {
  case ESP_SLEEP_WAKEUP_EXT0:
  case ESP_SLEEP_WAKEUP_EXT1:
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    ESP_LOGI(TAG, "Booting due to deep sleep external interrput %d wakeup", wakeupReason);
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    ESP_LOGI(TAG, "Booting due to deep sleep timer wakeup");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    ESP_LOGI(TAG, "Booting due to deep sleep ULP wakeup");
    break;
  default:
    ESP_LOGI(TAG, "Booting from cold start");
    // start the ULP program
    initUlp(ULP_DELAY_SECS);
    break;
  }

  unsigned int lastRuns = 0;
  unsigned long deepSleepTime = millis() + (DEEP_SLEEP_AFTER_SECS * 1000);
  for(;;) {
    if(lastRuns < ulpData.runs.val) {
      ESP_LOGI(TAG, "Have new ULP data! Runs: %d", ulpData.runs.val);
      printUlpData();
      lastRuns = ulpData.runs.val;
    }

    if(millis() >= deepSleepTime) {
      enterDeepSleep();
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

# ESP32 ULP timing with ext0 wake source problem example 

This example project demonstrates some strange ESP32 ULP wakeup period behavior that I've been experiencing.

## Environment

- Development Kit: none
- Kit version (for WroverKit/PicoKit/DevKitC):
- Module or chip used: ESP32-WROOM-32E
- IDF version (run ``git describe --tags`` to find it): v5.0-dev-517-g68cf4ef2be
- Build System: idf.py
- Compiler version (run ``xtensa-esp32-elf-gcc --version`` to find it): xtensa-esp32-elf-gcc (crosstool-NG esp-2021r2) 8.4.0
- Operating System: macOS
- (Windows only) environment type:
- Using an IDE?: Yes (vscode with IDF extension, but also using idf.py on cli)
- Power Supply: USB + battery

## Problem Description

My scenario is an ESP32 that is running a ULP program every 10 seconds using `ulp_set_wakeup_period()`. Each time the ULP program runs it increments a run counter and saves the RTC ticks counter value for that run in an array. The run counter and array of ticks values are stored in RTC memory to survive deep sleep.

The main IDF program running on the ESP32 starts the ULP program the first time it boots (it does nothing with the ULP when waking from deep sleep). The ESP32 then enters a loop where it looks for new data coming in from the ULP program. When new data is found the entire contents of the stored ULP data array is printed. After 60 seconds in this loop the main ESP32 CPU enters deep sleep, and stays in deep sleep for 60 seconds using `esp_sleep_enable_timer_wakeup()`.

### Expected Behavior

Expect the ULP to 1) increment runs count and 2) save a ticks value every 10 seconds, regardless of whether the main ESP32 CPU is awake or in deep sleep, and regardless of whether an `ext0` wake source is enabled or not.

### Actual Behavior

When the sleep timer is the only deep sleep wakeup source configured in the program, then the program works correctly. However, if an `ext0` wakeup source is configured in addition to the timer source then I observe that while the ESP32 main CPU is awake everything works correctly, but when in deep sleep the ULP wakeup period is doubled. Instead of seeing a ticks value saved every 10 seconds I see one every 20 seconds.

This doubling of the ULP wakeup period is only seen when the `ext0` wake source is used, and when the main CPU is in deep sleep.

I should note that in the above scenario, the `ext0` wake source does work as expected to wake the ESP32. The only strange behavior seems to be the ULP wake delay being doubled.

I also experimented with different wake delays, and the doubling of the delay is consistent.

### Steps to reproduce

1. Checkout, build and run the sample code as-is, and observe the doubling behavior of the ULP delay.
2. Comment-out the line with the `ext0` wakeup source in the code, rebuild and reflash/run, and observe correct ULP delays.

Note that the example does use the (awesome) [HULP library from boarchuz](https://github.com/boarchuz/HULP) for convenience. The project includes it as a git submodule, so be sure to fetch submodules before building.

And as you can see in the code, I'm using GPIO2 as the `ext0` wake pin.

### Code to reproduce this issue

See the code in this repo :)

## Debug Logs

I'll add logs for both when an `ext0` wake source is enabled and when not enabled. In each case I've included the logs of the first boot and 60s awake period, first 60s deep-sleep period, and the second boot (from deep sleep) for 60s until it goes back into deep-sleep.

This first log is when the `ext0` wake source *is enabled*. As you can see, the ESP32 boots, and then every 10 seconds it detects new data from the ULP and prints all saved data. After 60 seconds it enters deep sleep. Once the ESP32 wakes up and prints out the ULP data you can see that instead of saving 6 values while it was in deep sleep (one every 10s), it only saved 3 (one every 20s). But once the ESP32 is awake again the new data comes in every 10 seconds. While I've stopped the log here, it repeats just like this over and over as the ESP32 enters and exits deep sleep.

```
ets Jul 29 2019 12:21:46

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:5888
load:0x40078000,len:14804
ho 0 tail 12 room 4
load:0x40080400,len:3792
entry 0x40080694
I (29) boot: ESP-IDF v5.0-dev-517-g68cf4ef2be 2nd stage bootloader
I (29) boot: compile time 11:39:43
I (29) boot: chip revision: 3
I (31) boot_comm: chip revision: 3, min. bootloader chip revision: 0
I (37) boot.esp32: SPI Speed      : 40MHz
I (40) boot.esp32: SPI Mode       : DIO
I (44) boot.esp32: SPI Flash Size : 2MB
I (48) boot: Enabling RNG early entropy source...
I (52) boot: Partition Table:
I (55) boot: ## Label            Usage          Type ST Offset   Length
I (61) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (68) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (74) boot:  2 factory          factory app      00 00 00010000 00100000
I (81) boot: End of partition table
I (84) boot_comm: chip revision: 3, min. application chip revision: 0
I (90) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=09094h ( 37012) map
I (111) esp_image: segment 1: paddr=000190bc vaddr=3ffb0000 size=023e0h (  9184) load
I (115) esp_image: segment 2: paddr=0001b4a4 vaddr=40080000 size=04b74h ( 19316) load
I (123) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=172f4h ( 94964) map
I (158) esp_image: segment 4: paddr=0003731c vaddr=40084b74 size=071c8h ( 29128) load
I (170) esp_image: segment 5: paddr=0003e4ec vaddr=400c0000 size=00064h (   100) load
I (170) esp_image: segment 6: paddr=0003e558 vaddr=50000200 size=004c8h (  1224) load
I (181) boot: Loaded app from partition at offset 0x10000
I (181) boot: Disabling RNG early entropy source...
I (195) cpu_start: Pro cpu up.
I (195) cpu_start: Starting app cpu, entry point is 0x400810f4
I (0) cpu_start: App cpu up.
I (206) cpu_start: Pro cpu start user code
I (206) cpu_start: cpu freq: 160000000 Hz
I (206) cpu_start: Application information:
I (208) cpu_start: Project name:     ulp-behavior-test
I (213) cpu_start: App version:      c5c6f3c-dirty
I (217) cpu_start: Compile time:     Dec  9 2021 11:39:40
I (223) cpu_start: ELF file SHA256:  32bddffb18a31396...
I (228) cpu_start: ESP-IDF:          v5.0-dev-517-g68cf4ef2be
I (233) heap_init: Initializing. RAM available for dynamic allocation:
I (239) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (244) heap_init: At 3FFB2CD0 len 0002D330 (180 KiB): DRAM
I (250) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (255) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (261) heap_init: At 4008BD3C len 000142C4 (80 KiB): IRAM
I (267) spi_flash: detected chip: generic
I (270) spi_flash: flash io: dio
W (273) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (286) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (294) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (304) ulp-test: Booting from cold start
I (304) ulp-test: Starting ULP program with 10s delay...
I (314) ulp-test: ULP started
I (314) ulp-test: Have new ULP data! Runs: 1
I (314) ulp-test: Printing 1 ULP datapoints
I (324) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (10334) ulp-test: Have new ULP data! Runs: 2
I (10334) ulp-test: Printing 2 ULP datapoints
I (10334) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (10334) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (20344) ulp-test: Have new ULP data! Runs: 3
I (20344) ulp-test: Printing 3 ULP datapoints
I (20344) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (20344) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (20354) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (30364) ulp-test: Have new ULP data! Runs: 4
I (30364) ulp-test: Printing 4 ULP datapoints
I (30364) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (30364) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (30374) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (30384) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (40394) ulp-test: Have new ULP data! Runs: 5
I (40394) ulp-test: Printing 5 ULP datapoints
I (40394) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (40394) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (40404) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (40414) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (40424) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (50434) ulp-test: Have new ULP data! Runs: 6
I (50434) ulp-test: Printing 6 ULP datapoints
I (50434) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (50434) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (50444) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (50454) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (50464) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (50474) ulp-test: ULP Data Index:   5 || Time:    50347ms -  50.3s || Delta:    10000ms -  10.0s
I (60474) ulp-test: Have new ULP data! Runs: 7
I (60474) ulp-test: Printing 7 ULP datapoints
I (60474) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (60474) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (60484) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (60494) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (60504) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (60514) ulp-test: ULP Data Index:   5 || Time:    50347ms -  50.3s || Delta:    10000ms -  10.0s
I (60514) ulp-test: ULP Data Index:   6 || Time:    60347ms -  60.3s || Delta:    10000ms -  10.0s
I (60524) ulp-test: Entering deep sleep for 60s
ets Jul 29 2019 12:21:46

rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:5888
load:0x40078000,len:14804
ho 0 tail 12 room 4
load:0x40080400,len:3792
entry 0x40080694
I (29) boot: ESP-IDF v5.0-dev-517-g68cf4ef2be 2nd stage bootloader
I (29) boot: compile time 11:39:43
I (29) boot: chip revision: 3
I (31) boot_comm: chip revision: 3, min. bootloader chip revision: 0
I (37) boot.esp32: SPI Speed      : 40MHz
I (41) boot.esp32: SPI Mode       : DIO
I (44) boot.esp32: SPI Flash Size : 2MB
I (48) boot: Enabling RNG early entropy source...
I (52) boot: Partition Table:
I (55) boot: ## Label            Usage          Type ST Offset   Length
I (61) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (68) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (74) boot:  2 factory          factory app      00 00 00010000 00100000
I (81) boot: End of partition table
I (84) boot_comm: chip revision: 3, min. application chip revision: 0
I (90) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=09094h ( 37012) map
I (111) esp_image: segment 1: paddr=000190bc vaddr=3ffb0000 size=023e0h (  9184) load
I (115) esp_image: segment 2: paddr=0001b4a4 vaddr=40080000 size=04b74h ( 19316) load
I (123) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=172f4h ( 94964) map
I (158) esp_image: segment 4: paddr=0003731c vaddr=40084b74 size=071c8h ( 29128) load
I (170) esp_image: segment 5: paddr=0003e4ec vaddr=400c0000 size=00064h (   100) 
I (170) esp_image: segment 6: paddr=0003e558 vaddr=50000200 size=004c8h (  1224) 
I (179) boot: Loaded app from partition at offset 0x10000
I (180) boot: Disabling RNG early entropy source...
I (194) cpu_start: Pro cpu up.
I (194) cpu_start: Starting app cpu, entry point is 0x400810f4
I (0) cpu_start: App cpu up.
I (205) cpu_start: Pro cpu start user code
I (205) cpu_start: cpu freq: 160000000 Hz
I (206) cpu_start: Application information:
I (207) cpu_start: Project name:     ulp-behavior-test
I (212) cpu_start: App version:      c5c6f3c-dirty
I (217) cpu_start: Compile time:     Dec  9 2021 11:39:40
I (222) cpu_start: ELF file SHA256:  32bddffb18a31396...
I (227) cpu_start: ESP-IDF:          v5.0-dev-517-g68cf4ef2be
I (233) heap_init: Initializing. RAM available for dynamic allocation:
I (239) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (244) heap_init: At 3FFB2CD0 len 0002D330 (180 KiB): DRAM
I (249) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (254) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (260) heap_init: At 4008BD3C len 000142C4 (80 KiB): IRAM
I (266) spi_flash: detected chip: generic
I (269) spi_flash: flash io: dio
W (272) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (285) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (293) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (303) ulp-test: Booting due to deep sleep timer wakeup
I (303) ulp-test: Have new ULP data! Runs: 10
I (313) ulp-test: Printing 10 ULP datapoints
I (313) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (323) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (333) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (343) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (343) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (353) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (363) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (373) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (383) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (393) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (10403) ulp-test: Have new ULP data! Runs: 11
I (10403) ulp-test: Printing 11 ULP datapoints
I (10403) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (10403) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (10413) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (10423) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (10433) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (10443) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (10443) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (10453) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (10463) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (10473) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (10483) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (20493) ulp-test: Have new ULP data! Runs: 12
I (20493) ulp-test: Printing 12 ULP datapoints
I (20493) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (20493) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (20503) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (20513) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (20523) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (20533) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (20533) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (20543) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (20553) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (20563) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (20573) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (20583) ulp-test: ULP Data Index:  11 || Time:   140000ms - 140.0s || Delta:     9975ms -  10.0s
I (30593) ulp-test: Have new ULP data! Runs: 13
I (30593) ulp-test: Printing 13 ULP datapoints
I (30593) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (30593) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (30603) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (30613) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (30623) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (30633) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (30633) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (30643) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (30653) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (30663) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (30673) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (30683) ulp-test: ULP Data Index:  11 || Time:   140000ms - 140.0s || Delta:     9975ms -  10.0s
I (30693) ulp-test: ULP Data Index:  12 || Time:   149975ms - 150.0s || Delta:     9975ms -  10.0s
I (40703) ulp-test: Have new ULP data! Runs: 14
I (40703) ulp-test: Printing 14 ULP datapoints
I (40703) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (40703) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (40713) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (40723) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (40733) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (40743) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (40743) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (40753) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (40763) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (40773) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (40783) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (40793) ulp-test: ULP Data Index:  11 || Time:   140000ms - 140.0s || Delta:     9975ms -  10.0s
I (40803) ulp-test: ULP Data Index:  12 || Time:   149975ms - 150.0s || Delta:     9975ms -  10.0s
I (40813) ulp-test: ULP Data Index:  13 || Time:   159951ms - 160.0s || Delta:     9976ms -  10.0s
I (49813) ulp-test: Have new ULP data! Runs: 15
I (49813) ulp-test: Printing 15 ULP datapoints
I (49813) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (49813) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (49823) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (49833) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (49843) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (49853) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (49853) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (49863) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (49873) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (49883) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (49893) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (49903) ulp-test: ULP Data Index:  11 || Time:   140000ms - 140.0s || Delta:     9975ms -  10.0s
I (49913) ulp-test: ULP Data Index:  12 || Time:   149975ms - 150.0s || Delta:     9975ms -  10.0s
I (49923) ulp-test: ULP Data Index:  13 || Time:   159951ms - 160.0s || Delta:     9976ms -  10.0s
I (49923) ulp-test: ULP Data Index:  14 || Time:   169926ms - 169.9s || Delta:     9975ms -  10.0s
I (59933) ulp-test: Have new ULP data! Runs: 16
I (59933) ulp-test: Printing 16 ULP datapoints
I (59933) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (59933) ulp-test: ULP Data Index:   1 || Time:    10321ms -  10.3s || Delta:     9975ms -  10.0s
I (59943) ulp-test: ULP Data Index:   2 || Time:    20296ms -  20.3s || Delta:     9975ms -  10.0s
I (59953) ulp-test: ULP Data Index:   3 || Time:    30272ms -  30.3s || Delta:     9976ms -  10.0s
I (59963) ulp-test: ULP Data Index:   4 || Time:    40247ms -  40.2s || Delta:     9975ms -  10.0s
I (59973) ulp-test: ULP Data Index:   5 || Time:    50222ms -  50.2s || Delta:     9975ms -  10.0s
I (59973) ulp-test: ULP Data Index:   6 || Time:    60198ms -  60.2s || Delta:     9976ms -  10.0s
I (59983) ulp-test: ULP Data Index:   7 || Time:    80148ms -  80.1s || Delta:    19950ms -  19.9s
I (59993) ulp-test: ULP Data Index:   8 || Time:   100099ms - 100.1s || Delta:    19951ms -  20.0s
I (60003) ulp-test: ULP Data Index:   9 || Time:   120049ms - 120.0s || Delta:    19950ms -  19.9s
I (60013) ulp-test: ULP Data Index:  10 || Time:   130025ms - 130.0s || Delta:     9976ms -  10.0s
I (60023) ulp-test: ULP Data Index:  11 || Time:   140000ms - 140.0s || Delta:     9975ms -  10.0s
I (60033) ulp-test: ULP Data Index:  12 || Time:   149975ms - 150.0s || Delta:     9975ms -  10.0s
I (60043) ulp-test: ULP Data Index:  13 || Time:   159951ms - 160.0s || Delta:     9976ms -  10.0s
I (60043) ulp-test: ULP Data Index:  14 || Time:   169926ms - 169.9s || Delta:     9975ms -  10.0s
I (60053) ulp-test: ULP Data Index:  15 || Time:   179901ms - 179.9s || Delta:     9975ms -  10.0s
I (61063) ulp-test: Entering deep sleep for 60s
```

This is the logs when the `ext0` wake source is commented-out in the code. As you can see, the ULP saves a value every 10 seconds regardless of whether the ESP is awake or in deep sleep.

```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:5888
load:0x40078000,len:14804
ho 0 tail 12 room 4
load:0x40080400,len:3792
entry 0x40080694
I (29) boot: ESP-IDF v5.0-dev-517-g68cf4ef2be 2nd stage bootloader
I (29) boot: compile time 11:39:43
I (29) boot: chip revision: 3
I (31) boot_comm: chip revision: 3, min. bootloader chip revision: 0
I (37) boot.esp32: SPI Speed      : 40MHz
I (40) boot.esp32: SPI Mode       : DIO
I (44) boot.esp32: SPI Flash Size : 2MB
I (48) boot: Enabling RNG early entropy source...
I (52) boot: Partition Table:
I (55) boot: ## Label            Usage          Type ST Offset   Length
I (61) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (68) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (74) boot:  2 factory          factory app      00 00 00010000 00100000
I (81) boot: End of partition table
I (84) boot_comm: chip revision: 3, min. application chip revision: 0
I (90) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0901ch ( 36892) map
I (111) esp_image: segment 1: paddr=00019044 vaddr=3ffb0000 size=023e0h (  9184) load
I (115) esp_image: segment 2: paddr=0001b42c vaddr=40080000 size=04bech ( 19436) load
I (123) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=17224h ( 94756) map
I (158) esp_image: segment 4: paddr=0003724c vaddr=40084bec size=07150h ( 29008) load
I (170) esp_image: segment 5: paddr=0003e3a4 vaddr=400c0000 size=00064h (   100) load
I (170) esp_image: segment 6: paddr=0003e410 vaddr=50000200 size=004c8h (  1224) load
I (180) boot: Loaded app from partition at offset 0x10000
I (180) boot: Disabling RNG early entropy source...
I (194) cpu_start: Pro cpu up.
I (195) cpu_start: Starting app cpu, entry point is 0x400810f4
I (0) cpu_start: App cpu up.
I (206) cpu_start: Pro cpu start user code
I (206) cpu_start: cpu freq: 160000000 Hz
I (206) cpu_start: Application information:
I (208) cpu_start: Project name:     ulp-behavior-test
I (213) cpu_start: App version:      c5c6f3c-dirty
I (217) cpu_start: Compile time:     Dec  9 2021 11:39:40
I (222) cpu_start: ELF file SHA256:  598543aecae12f5e...
I (227) cpu_start: ESP-IDF:          v5.0-dev-517-g68cf4ef2be
I (233) heap_init: Initializing. RAM available for dynamic allocation:
I (239) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (244) heap_init: At 3FFB2CD0 len 0002D330 (180 KiB): DRAM
I (250) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (255) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (260) heap_init: At 4008BD3C len 000142C4 (80 KiB): IRAM
I (267) spi_flash: detected chip: generic
I (269) spi_flash: flash io: dio
W (272) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (286) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (294) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (304) ulp-test: Booting from cold start
I (304) ulp-test: Starting ULP program with 10s delay...
I (314) ulp-test: ULP started
I (314) ulp-test: Have new ULP data! Runs: 1
I (314) ulp-test: Printing 1 ULP datapoints
I (324) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (10334) ulp-test: Have new ULP data! Runs: 2
I (10334) ulp-test: Printing 2 ULP datapoints
I (10334) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (10334) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (20344) ulp-test: Have new ULP data! Runs: 3
I (20344) ulp-test: Printing 3 ULP datapoints
I (20344) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (20344) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (20354) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (30364) ulp-test: Have new ULP data! Runs: 4
I (30364) ulp-test: Printing 4 ULP datapoints
I (30364) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (30364) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (30374) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (30384) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (40394) ulp-test: Have new ULP data! Runs: 5
I (40394) ulp-test: Printing 5 ULP datapoints
I (40394) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (40394) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (40404) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (40414) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (40424) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (50434) ulp-test: Have new ULP data! Runs: 6
I (50434) ulp-test: Printing 6 ULP datapoints
I (50434) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (50434) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (50444) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (50454) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (50464) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (50474) ulp-test: ULP Data Index:   5 || Time:    50347ms -  50.3s || Delta:    10000ms -  10.0s
I (60474) ulp-test: Have new ULP data! Runs: 7
I (60474) ulp-test: Printing 7 ULP datapoints
I (60474) ulp-test: ULP Data Index:   0 || Time:      347ms -   0.3s || Delta:      347ms -   0.3s
I (60474) ulp-test: ULP Data Index:   1 || Time:    10347ms -  10.3s || Delta:    10000ms -  10.0s
I (60484) ulp-test: ULP Data Index:   2 || Time:    20347ms -  20.3s || Delta:    10000ms -  10.0s
I (60494) ulp-test: ULP Data Index:   3 || Time:    30347ms -  30.3s || Delta:    10000ms -  10.0s
I (60504) ulp-test: ULP Data Index:   4 || Time:    40347ms -  40.3s || Delta:    10000ms -  10.0s
I (60514) ulp-test: ULP Data Index:   5 || Time:    50347ms -  50.3s || Delta:    10000ms -  10.0s
I (60514) ulp-test: ULP Data Index:   6 || Time:    60347ms -  60.3s || Delta:    10000ms -  10.0s
I (60524) ulp-test: Entering deep sleep for 60s
ets Jul 29 2019 12:21:46

rst:0x5 (DEEPSLEEP_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:5888
load:0x40078000,len:14804
ho 0 tail 12 room 4
load:0x40080400,len:3792
entry 0x40080694
I (29) boot: ESP-IDF v5.0-dev-517-g68cf4ef2be 2nd stage bootloader
I (29) boot: compile time 11:39:43
I (29) boot: chip revision: 3
I (31) boot_comm: chip revision: 3, min. bootloader chip revision: 0
I (37) boot.esp32: SPI Speed      : 40MHz
I (41) boot.esp32: SPI Mode       : DIO
I (44) boot.esp32: SPI Flash Size : 2MB
I (48) boot: Enabling RNG early entropy source...
I (52) boot: Partition Table:
I (55) boot: ## Label            Usage          Type ST Offset   Length
I (61) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (68) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (74) boot:  2 factory          factory app      00 00 00010000 00100000
I (81) boot: End of partition table
I (84) boot_comm: chip revision: 3, min. application chip revision: 0
I (90) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=0901ch ( 36892) map
I (111) esp_image: segment 1: paddr=00019044 vaddr=3ffb0000 size=023e0h (  9184) load
I (115) esp_image: segment 2: paddr=0001b42c vaddr=40080000 size=04bech ( 19436) load
I (123) esp_image: segment 3: paddr=00020020 vaddr=400d0020 size=17224h ( 94756) map
I (158) esp_image: segment 4: paddr=0003724c vaddr=40084bec size=07150h ( 29008) load
I (170) esp_image: segment 5: paddr=0003e3a4 vaddr=400c0000 size=00064h (   100) 
I (170) esp_image: segment 6: paddr=0003e410 vaddr=50000200 size=004c8h (  1224) 
I (179) boot: Loaded app from partition at offset 0x10000
I (179) boot: Disabling RNG early entropy source...
I (194) cpu_start: Pro cpu up.
I (194) cpu_start: Starting app cpu, entry point is 0x400810f4
I (0) cpu_start: App cpu up.
I (205) cpu_start: Pro cpu start user code
I (205) cpu_start: cpu freq: 160000000 Hz
I (205) cpu_start: Application information:
I (207) cpu_start: Project name:     ulp-behavior-test
I (212) cpu_start: App version:      c5c6f3c-dirty
I (217) cpu_start: Compile time:     Dec  9 2021 11:39:40
I (222) cpu_start: ELF file SHA256:  598543aecae12f5e...
I (227) cpu_start: ESP-IDF:          v5.0-dev-517-g68cf4ef2be
I (233) heap_init: Initializing. RAM available for dynamic allocation:
I (239) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (244) heap_init: At 3FFB2CD0 len 0002D330 (180 KiB): DRAM
I (249) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (254) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (260) heap_init: At 4008BD3C len 000142C4 (80 KiB): IRAM
I (266) spi_flash: detected chip: generic
I (269) spi_flash: flash io: dio
W (272) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (285) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (293) gpio: GPIO[2]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (303) ulp-test: Booting due to deep sleep timer wakeup
I (303) ulp-test: Have new ULP data! Runs: 13
I (313) ulp-test: Printing 13 ULP datapoints
I (313) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (323) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (333) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (343) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (343) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (353) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (363) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (373) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (383) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (393) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (403) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (403) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (413) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (10423) ulp-test: Have new ULP data! Runs: 14
I (10423) ulp-test: Printing 14 ULP datapoints
I (10423) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (10423) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (10433) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (10443) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (10453) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (10463) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (10463) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (10473) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (10483) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (10493) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (10503) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (10513) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (10523) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (10533) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (20533) ulp-test: Have new ULP data! Runs: 15
I (20533) ulp-test: Printing 15 ULP datapoints
I (20533) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (20533) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (20543) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (20553) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (20563) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (20573) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (20573) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (20583) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (20593) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (20603) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (20613) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (20623) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (20633) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (20643) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (20643) ulp-test: ULP Data Index:  14 || Time:   140056ms - 140.1s || Delta:     9979ms -  10.0s
I (30653) ulp-test: Have new ULP data! Runs: 16
I (30653) ulp-test: Printing 16 ULP datapoints
I (30653) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (30653) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (30663) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (30673) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (30683) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (30693) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (30693) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (30703) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (30713) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (30723) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (30733) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (30743) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (30753) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (30763) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (30763) ulp-test: ULP Data Index:  14 || Time:   140056ms - 140.1s || Delta:     9979ms -  10.0s
I (30773) ulp-test: ULP Data Index:  15 || Time:   150035ms - 150.0s || Delta:     9979ms -  10.0s
I (40783) ulp-test: Have new ULP data! Runs: 17
I (40783) ulp-test: Printing 17 ULP datapoints
I (40783) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (40783) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (40793) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (40803) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (40813) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (40823) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (40823) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (40833) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (40843) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (40853) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (40863) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (40873) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (40883) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (40893) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (40893) ulp-test: ULP Data Index:  14 || Time:   140056ms - 140.1s || Delta:     9979ms -  10.0s
I (40903) ulp-test: ULP Data Index:  15 || Time:   150035ms - 150.0s || Delta:     9979ms -  10.0s
I (40913) ulp-test: ULP Data Index:  16 || Time:   160014ms - 160.0s || Delta:     9979ms -  10.0s
I (49923) ulp-test: Have new ULP data! Runs: 18
I (49923) ulp-test: Printing 18 ULP datapoints
I (49923) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (49923) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (49933) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (49943) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (49953) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (49963) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (49963) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (49973) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (49983) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (49993) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (50003) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (50013) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (50023) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (50033) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (50033) ulp-test: ULP Data Index:  14 || Time:   140056ms - 140.1s || Delta:     9979ms -  10.0s
I (50043) ulp-test: ULP Data Index:  15 || Time:   150035ms - 150.0s || Delta:     9979ms -  10.0s
I (50053) ulp-test: ULP Data Index:  16 || Time:   160014ms - 160.0s || Delta:     9979ms -  10.0s
I (50063) ulp-test: ULP Data Index:  17 || Time:   169994ms - 170.0s || Delta:     9980ms -  10.0s
I (60073) ulp-test: Have new ULP data! Runs: 19
I (60073) ulp-test: Printing 19 ULP datapoints
I (60073) ulp-test: ULP Data Index:   0 || Time:      346ms -   0.3s || Delta:      346ms -   0.3s
I (60073) ulp-test: ULP Data Index:   1 || Time:    10325ms -  10.3s || Delta:     9979ms -  10.0s
I (60083) ulp-test: ULP Data Index:   2 || Time:    20304ms -  20.3s || Delta:     9979ms -  10.0s
I (60093) ulp-test: ULP Data Index:   3 || Time:    30284ms -  30.3s || Delta:     9980ms -  10.0s
I (60103) ulp-test: ULP Data Index:   4 || Time:    40263ms -  40.3s || Delta:     9979ms -  10.0s
I (60113) ulp-test: ULP Data Index:   5 || Time:    50242ms -  50.2s || Delta:     9979ms -  10.0s
I (60113) ulp-test: ULP Data Index:   6 || Time:    60221ms -  60.2s || Delta:     9979ms -  10.0s
I (60123) ulp-test: ULP Data Index:   7 || Time:    70201ms -  70.2s || Delta:     9980ms -  10.0s
I (60133) ulp-test: ULP Data Index:   8 || Time:    80180ms -  80.2s || Delta:     9979ms -  10.0s
I (60143) ulp-test: ULP Data Index:   9 || Time:    90159ms -  90.2s || Delta:     9979ms -  10.0s
I (60153) ulp-test: ULP Data Index:  10 || Time:   100139ms - 100.1s || Delta:     9980ms -  10.0s
I (60163) ulp-test: ULP Data Index:  11 || Time:   110118ms - 110.1s || Delta:     9979ms -  10.0s
I (60173) ulp-test: ULP Data Index:  12 || Time:   120097ms - 120.1s || Delta:     9979ms -  10.0s
I (60183) ulp-test: ULP Data Index:  13 || Time:   130077ms - 130.1s || Delta:     9980ms -  10.0s
I (60183) ulp-test: ULP Data Index:  14 || Time:   140056ms - 140.1s || Delta:     9979ms -  10.0s
I (60193) ulp-test: ULP Data Index:  15 || Time:   150035ms - 150.0s || Delta:     9979ms -  10.0s
I (60203) ulp-test: ULP Data Index:  16 || Time:   160014ms - 160.0s || Delta:     9979ms -  10.0s
I (60213) ulp-test: ULP Data Index:  17 || Time:   169994ms - 170.0s || Delta:     9980ms -  10.0s
I (60223) ulp-test: ULP Data Index:  18 || Time:   179973ms - 180.0s || Delta:     9979ms -  10.0s
I (61233) ulp-test: Entering deep sleep for 60s
```

## Other items if possible

- The github project has an `sdkconfig.defaults` file - the only other change I made was to disable ANSI color in logging.
- As mentioned above, the ULP code does use the [HULP library](https://github.com/boarchuz/HULP), but only minimally. Based on the behavior I'm seeing I really don't think it is contributing to the strange ULP behavior.


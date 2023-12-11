#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/core.c
  src/version.c
  src/ble/bluetooth.c
  src/ble/eddystone.c
  src/drivers/board.c
  src/drivers/leds/drv_leds.c
  src/drivers/buttons/drv_button.c
  src/drivers/pwr/drv_pwr.c
  src/drivers/sound/drv_sound.c
  src/drivers/adc/drv_adc.c
)

# NORDIC SDK APP END
zephyr_library_include_directories(.)

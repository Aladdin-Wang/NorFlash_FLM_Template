# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_ECLIPSE_THREADX_FILEX 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(CONFIG_HPM_SPI 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(hello_world)

sdk_compile_definitions(-DFX_STANDALONE_ENABLE)

sdk_inc(.)
sdk_inc(${BOARD})
sdk_inc(../..)
sdk_inc(../../norflash)
sdk_inc(../../nandflash)
sdk_app_src(
../../norflash/chry_sflash_norflash.c
../../nandflash/chry_sflash_nandflash.c
../../nandflash/lx_chry_sflash_nandflash.c
../../nandflash/fx_chry_sflash_nandflash.c
../../nandflash/msc_lx_chry_sflash_nandflash.c
../../port/hpm/chry_sflash_port_hpm.c
)

add_subdirectory(../../third_party/levelx/common levelx_common)

sdk_app_src(fx_usb_nandflash_test.c)
generate_ide_projects()

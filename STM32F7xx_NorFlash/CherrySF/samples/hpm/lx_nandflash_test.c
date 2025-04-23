/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "chry_sflash_nandflash.h"
#include "hpm_l1c_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_gpio_drv.h"
#include "lx_api.h"

extern struct chry_sflash_nandflash g_nandflash;
extern LX_NAND_FLASH g_lx_nandflash;
extern UINT lx_nand_flash_open_with_format(void);

struct chry_sflash_host spi_host;

#define TRANSFER_SIZE (2 * 1024U)

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t wbuff[TRANSFER_SIZE];
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t rbuff[TRANSFER_SIZE];

void lx_nandflash_test()
{
    UINT status;
    UINT status2;
    uint64_t elapsed = 0, now;
    double write_speed, read_speed;

    lx_nand_flash_open_with_format();

    for (size_t i = 0; i < 10; i++) {
        now = mchtmr_get_count(HPM_MCHTMR);
        for (size_t i = 0; i < 64; i++) {
            _lx_nand_flash_sector_write(&g_lx_nandflash, i, wbuff);
        }
        elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
        write_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;

        now = mchtmr_get_count(HPM_MCHTMR);
        for (size_t i = 0; i < 64; i++) {
            _lx_nand_flash_sector_read(&g_lx_nandflash, i, rbuff);
        }
        elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
        read_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;
        printf("write_speed:%.2f KB/s, read_speed:%.2f KB/s\n", write_speed, read_speed);

        _lx_nand_flash_sector_release(&g_lx_nandflash, 64);
        for (size_t i = 0; i < 64; i++) {
            _lx_nand_flash_sector_release(&g_lx_nandflash, i);
        }
    }

    for (size_t i = 0; i < TRANSFER_SIZE; i++) {
        if (rbuff[i] != wbuff[i]) {
            printf("write read error\n");
            printf("i = %d, rbuff[i] = %d, wbuff[i] = %d\n", i, rbuff[i], wbuff[i]);
            while (1) {}
        }
    }

    printf("lx_test done\r\n");

    while (1) {}
}

uint8_t area_buffer[64] = { 0 };

int main(void)
{
    uint64_t elapsed = 0, now;
    double write_speed, read_speed;
    int ret;

    board_init();

    memset(rbuff, 0xaa, sizeof(rbuff));
    for (uint32_t i = 0; i < sizeof(wbuff); i++) {
        wbuff[i] = i % 0xFF;
    }

    spi_host.spi_idx = 0;
    spi_host.iomode = CHRY_SFLASH_IOMODE_QUAD;
    chry_sflash_init(&spi_host);

    ret = chry_sflash_nandflash_init(&g_nandflash, &spi_host);
    printf("init ret:%d\n", ret);

    chry_sflash_set_frequency(&spi_host, 80000000);

    for (size_t i = 0; i < 1024; i++) {
        chry_sflash_nandflash_erase(&g_nandflash, i);
    }

    lx_nandflash_test();
#if 0
    for (size_t i = 0; i < 1; i++) {
        ret = chry_sflash_nandflash_erase(&flash, i);
        printf("erase ret:%d\n", ret);

        now = mchtmr_get_count(HPM_MCHTMR);
        for (size_t j = 0; j < 64; j++) {
            ret += chry_sflash_nandflash_write(&flash, i, j, wbuff, TRANSFER_SIZE, NULL, 0);
        }
        elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
        write_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;
        printf("write ret:%d\n", ret);
    }
    now = mchtmr_get_count(HPM_MCHTMR);
    for (size_t i = 0; i < 1; i++) {
        for (size_t j = 0; j < 64; j++) {
            ret += chry_sflash_nandflash_read(&flash, i, j, rbuff, TRANSFER_SIZE, NULL, 0);
        }
    }
    elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
    read_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;
    printf("read ret:%d\n", ret);

    printf("write_speed:%.2f KB/s, read_speed:%.2f KB/s\n", write_speed, read_speed);

    for (size_t i = 0; i < TRANSFER_SIZE; i++) {
        if (rbuff[i] != wbuff[i]) {
            printf("write read error\n");
            printf("i = %d, rbuff[i] = %d, wbuff[i] = %d\n", i, rbuff[i], wbuff[i]);
            while (1) {}
        }
    }
    printf("done\r\n");
#endif
    while (1) {
    }
    return 0;
}

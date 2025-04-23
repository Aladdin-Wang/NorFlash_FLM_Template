/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "chry_sflash_norflash.h"
#include "hpm_l1c_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_gpio_drv.h"

struct chry_sflash_norflash flash;
struct chry_sflash_host spi_host;

#define TRANSFER_SIZE (8 * 1024U)

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t wbuff[TRANSFER_SIZE];
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t rbuff[TRANSFER_SIZE];

int main(void)
{
    uint64_t elapsed = 0, now;
    double write_speed, read_speed;
    int ret;

    board_init();

    memset(rbuff, 0, sizeof(rbuff));
    for (uint32_t i = 0; i < sizeof(wbuff); i++) {
        wbuff[i] = i % 0xFF;
    }

    spi_host.spi_idx = 0;
    spi_host.iomode = CHRY_SFLASH_IOMODE_DUAL;
    chry_sflash_init(&spi_host);

    chry_sflash_norflash_init(&flash, &spi_host);

    chry_sflash_set_frequency(&spi_host, 50000000);

    ret = chry_sflash_norflash_erase(&flash, 0, TRANSFER_SIZE);
    printf("erase ret:%d\n", ret);

    now = mchtmr_get_count(HPM_MCHTMR);
    ret = chry_sflash_norflash_write(&flash, 0, wbuff, TRANSFER_SIZE);
    elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
    write_speed = (double)TRANSFER_SIZE * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;
    printf("write ret:%d\n", ret);

    now = mchtmr_get_count(HPM_MCHTMR);
    ret = chry_sflash_norflash_read(&flash, 0, rbuff, TRANSFER_SIZE);
    elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
    read_speed = (double)TRANSFER_SIZE * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;
    printf("read ret:%d\n", ret);

    printf("write_speed:%.2f KB/s, read_speed:%.2f KB/s\n", write_speed, read_speed);
    for (size_t i = 0; i < TRANSFER_SIZE; i++) {
        if (rbuff[i] != wbuff[i]) {
            printf("write read error\n");
            while(1){}
        }
    }
    printf("done\r\n");
    while (1) {
    }
    return 0;
}

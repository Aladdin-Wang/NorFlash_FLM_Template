#include <stdio.h>
#include "fx_api.h"
#include "lx_api.h"
#include "board.h"
#include "chry_sflash_nandflash.h"
#include "hpm_l1c_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_usb_drv.h"
#include "usbd_core.h"

extern struct chry_sflash_nandflash g_nandflash;
struct chry_sflash_host spi_host;

#define TRANSFER_SIZE (2 * 1024U)

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t wbuff[TRANSFER_SIZE];
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t rbuff[TRANSFER_SIZE];

/* Buffer for FileX FX_MEDIA sector cache. This must be large enough for at least one
   sector, which are typically 512 bytes in size.  */

unsigned char media_memory[4096];

/* Define NAND simulated device driver entry.  */

VOID _fx_nand_flash_simulator_driver(FX_MEDIA *media_ptr);

/* Define FileX global data structures.  */

FX_MEDIA nand_disk;
FX_FILE my_file;
FX_FILE my_file1;

void fs_init(void)
{
    UINT status;

    /* Initialize NAND flash.  */
    lx_nand_flash_initialize();

    /* Initialize FileX.  */
    fx_system_initialize();

    do {
        /* Open the NAND disk.  */
        status = fx_media_open(&nand_disk, "NAND DISK", _fx_nand_flash_simulator_driver, FX_NULL, media_memory, sizeof(media_memory));
        /* Check the media open status.  */
        if (status != FX_SUCCESS) {
            /* Error, break the loop!  */
            printf("fail to open media, format now\r\n");

#if 1
            /* Format the NAND disk - the memory for the NAND flash disk is setup in
            the NAND simulator. Note that for best performance, the format of the
            NAND flash should be less than one full NAND flash block of sectors.  */
            fx_media_format(&nand_disk,
                            _fx_nand_flash_simulator_driver, // Driver entry
                            FX_NULL,                         // Unused
                            media_memory,                    // Media buffer pointer
                            sizeof(media_memory),            // Media buffer size
                            "MY_NAND_DISK",                  // Volume Name
                            2,                               // Number of FATs
                            128,                             // Directory Entries
                            0,                               // Hidden sectors
                            64 * 1024,                       // Total sectors
                            2048,                            // Sector size
                            1,                               // Sectors per cluster
                            1,                               // Heads
                            1);                              // Sectors per track
#endif
        }
    } while (status != FX_SUCCESS);
}

int main(void)
{
    int ret;

    board_init();
    board_init_led_pins();
    board_init_usb_pins();

    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);

    spi_host.spi_idx = 0;
    spi_host.iomode = CHRY_SFLASH_IOMODE_QUAD;
    chry_sflash_init(&spi_host);

    memset(rbuff, 0xaa, sizeof(rbuff));
    for (uint32_t i = 0; i < sizeof(wbuff); i++) {
        wbuff[i] = 0x31;
    }

    ret = chry_sflash_nandflash_init(&g_nandflash, &spi_host);
    printf("init ret:%d\n", ret);

    chry_sflash_set_frequency(&spi_host, 80000000);

    // for (size_t i = 0; i < 1024; i++) {
    //     chry_sflash_nandflash_erase(&g_nandflash, i);
    // }

    fs_init();

    extern void msc_ram_init(uint8_t busid, uint32_t reg_base);
    msc_ram_init(0, CONFIG_HPM_USBD_BASE);
}
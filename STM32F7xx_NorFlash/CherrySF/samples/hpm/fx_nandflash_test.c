/* This is a small demo of the high-performance FileX FAT file system with LevelX
   and the NAND simulated driver.  */

#include <stdio.h>
#include "fx_api.h"
#include "lx_api.h"
#include "board.h"
#include "chry_sflash_nandflash.h"
#include "hpm_l1c_drv.h"
#include "hpm_mchtmr_drv.h"
#include "hpm_gpio_drv.h"

extern struct chry_sflash_nandflash g_nandflash;
struct chry_sflash_host spi_host;

#define TRANSFER_SIZE (8 * 1024U)

ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t wbuff[TRANSFER_SIZE];
ATTR_PLACE_AT_WITH_ALIGNMENT(".ahb_sram", HPM_L1C_CACHELINE_SIZE)
uint8_t rbuff[TRANSFER_SIZE];

/* Buffer for FileX FX_MEDIA sector cache. This must be large enough for at least one
   sector, which are typically 512 bytes in size.  */

unsigned char media_memory[4096];

/* Define NAND simulated device driver entry.  */

VOID _fx_nand_flash_simulator_driver(FX_MEDIA *media_ptr);

/* Define thread prototypes.  */

void thread_0_entry(ULONG thread_input);

/* Define FileX global data structures.  */

FX_MEDIA nand_disk;
FX_FILE my_file;
FX_FILE my_file1;

ULONG thread_0_counter;

int main(void)
{
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

    /* Initialize NAND flash.  */
    lx_nand_flash_initialize();

    /* Initialize FileX.  */
    fx_system_initialize();

    thread_0_entry(0);
}

void thread_0_entry(ULONG thread_input)
{
    UINT status;
    ULONG actual;
    uint64_t elapsed = 0, now;
    double write_speed, read_speed;

    LX_PARAMETER_NOT_USED(thread_input);

    /* Format the NAND disk - the memory for the NAND flash disk is setup in
       the NAND simulator. Note that for best performance, the format of the
       NAND flash should be less than one full NAND flash block of sectors.  */
    fx_media_format(&nand_disk,
                    _fx_nand_flash_simulator_driver, // Driver entry
                    FX_NULL,                         // Unused
                    media_memory,                    // Media buffer pointer
                    sizeof(media_memory),            // Media buffer size
                    "MY_NAND_DISK",                  // Volume Name
                    1,                               // Number of FATs
                    32,                              // Directory Entries
                    0,                               // Hidden sectors
                    64*1024,                         // Total sectors
                    2048,                            // Sector size
                    1,                               // Sectors per cluster
                    1,                               // Heads
                    1);                              // Sectors per track

    /* Loop to repeat the demo over and over!  */
    do {
        /* Open the NAND disk.  */
        status = fx_media_open(&nand_disk, "NAND DISK", _fx_nand_flash_simulator_driver, FX_NULL, media_memory, sizeof(media_memory));

        /* Check the media open status.  */
        if (status != FX_SUCCESS) {
            /* Error, break the loop!  */
            break;
        }

        /* Create a file called TEST.TXT in the root directory.  */
        status = fx_file_create(&nand_disk, "TEST.TXT");

        /* Check the create status.  */
        if (status != FX_SUCCESS) {
            /* Check for an already created status. This is expected on the
               second pass of this loop!  */
            if (status != FX_ALREADY_CREATED) {
                /* Create error, break the loop.  */
                break;
            }
        }

        /* Open the test file.  */
        status = fx_file_open(&nand_disk, &my_file, "TEST.TXT", FX_OPEN_FOR_WRITE);

        /* Check the file open status.  */
        if (status != FX_SUCCESS) {
            /* Error opening file, break the loop.  */
            break;
        }

        /* Seek to the beginning of the test file.  */
        status = fx_file_seek(&my_file, 0);

        /* Check the file seek status.  */
        if (status != FX_SUCCESS) {
            /* Error performing file seek, break the loop.  */
            break;
        }

        now = mchtmr_get_count(HPM_MCHTMR);

        for (size_t i = 0; i < 64; i++)
        {
            /* Write a string to the test file.  */
            status = fx_file_write(&my_file, wbuff, TRANSFER_SIZE);
            elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
        }

        write_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;

        /* Check the file write status.  */
        if (status != FX_SUCCESS) {
            /* Error writing to a file, break the loop.  */
            break;
        }

        /* Seek to the beginning of the test file.  */
        status = fx_file_seek(&my_file, 0);

        /* Check the file seek status.  */
        if (status != FX_SUCCESS) {
            /* Error performing file seek, break the loop.  */
            break;
        }

        now = mchtmr_get_count(HPM_MCHTMR);
        for (size_t i = 0; i < 64; i++)
        {
            /* Read the first 28 bytes of the test file.  */
            status = fx_file_read(&my_file, rbuff, TRANSFER_SIZE, &actual);
            elapsed = (mchtmr_get_count(HPM_MCHTMR) - now);
        }

        read_speed = (double)(TRANSFER_SIZE * 64) * (clock_get_frequency(clock_mchtmr0) / 1000) / elapsed;

        /* Check the file read status.  */
        if ((status != FX_SUCCESS)) {
            /* Error reading file, break the loop.  */
            break;
        }
        printf("write_speed:%.2f KB/s, read_speed:%.2f KB/s\n", write_speed, read_speed);

        /* Close the test file.  */
        status = fx_file_close(&my_file);

        /* Check the file close status.  */
        if (status != FX_SUCCESS) {
            /* Error closing the file, break the loop.  */
            break;
        }

        /* Delete the file.  */
        status = fx_file_delete(&nand_disk, "TEST.TXT");

        /* Check the file delete status.  */
        if (status != FX_SUCCESS) {
            /* Error deleting the file, break the loop.  */
            break;
        }

        /* Close the media.  */
        status = fx_media_close(&nand_disk);

        /* Check the media close status.  */
        if (status != FX_SUCCESS) {
            /* Error closing the media, break the loop.  */
            break;
        }

        /* Increment the thread counter, which represents the number
           of successful passes through this loop.  */
        thread_0_counter++;
    } while (1);

    /* If we get here the FileX test failed!  */
    return;
}

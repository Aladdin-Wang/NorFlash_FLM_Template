/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/
/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** LevelX Component                                                      */
/**                                                                       */
/**   NAND Flash Simulator                                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary files.  */

#include "lx_api.h"
#include "chry_sflash_nandflash.h"

struct chry_sflash_nandflash g_nandflash;

/* Define constants for the NAND flash simulation. */

#define TOTAL_BLOCKS             1024
#define PHYSICAL_PAGES_PER_BLOCK 64       /* Min value of 2                                               */
#define BYTES_PER_PHYSICAL_PAGE  2048     /* 2048 bytes per page                                           */
#define WORDS_PER_PHYSICAL_PAGE  2048 / 4 /* Words per page                                               */
#define SPARE_BYTES_PER_PAGE     64       /* 64 "spare" bytes per page                                    */
                                          /* For 2048 byte block spare area:                              */
#define BAD_BLOCK_POSITION       0        /*      0 is the bad block byte postion                         */
#define EXTRA_BYTE_POSITION      0        /*      0 is the extra bytes starting byte postion              */
#define ECC_BYTE_POSITION        8        /*      8 is the ECC starting byte position                     */
#define SPARE_DATA1_OFFSET       4
#define SPARE_DATA1_LENGTH       4
#define SPARE_DATA2_OFFSET       2
#define SPARE_DATA2_LENGTH       2

/* Definition of the spare area is relative to the block size of the NAND part and perhaps manufactures of the NAND part.
   Here are some common definitions:

    2048 Byte Block

        Bytes               Meaning

        0               Bad block flag
        2-3             USER data 2
        4-7             USER data 1
        8-13            ECC bytes
        14-15           ECC for spare
*/

UCHAR space_area_buffer[SPARE_BYTES_PER_PAGE] = { 0 };

UINT _lx_nand_flash_simulator_initialize(LX_NAND_FLASH *nand_flash);

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_read(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, ULONG *destination, ULONG words);
UINT _lx_nand_flash_simulator_write(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, ULONG *source, ULONG words);
UINT _lx_nand_flash_simulator_block_erase(LX_NAND_FLASH *nand_flash, ULONG block, ULONG erase_count);
UINT _lx_nand_flash_simulator_block_erased_verify(LX_NAND_FLASH *nand_flash, ULONG block);
UINT _lx_nand_flash_simulator_page_erased_verify(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page);
UINT _lx_nand_flash_simulator_block_status_get(LX_NAND_FLASH *nand_flash, ULONG block, UCHAR *bad_block_byte);
UINT _lx_nand_flash_simulator_block_status_set(LX_NAND_FLASH *nand_flash, ULONG block, UCHAR bad_block_byte);
UINT _lx_nand_flash_simulator_extra_bytes_get(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *destination, UINT size);
UINT _lx_nand_flash_simulator_extra_bytes_set(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *source, UINT size);
UINT _lx_nand_flash_simulator_system_error(LX_NAND_FLASH *nand_flash, UINT error_code, ULONG block, ULONG page);

UINT _lx_nand_flash_simulator_pages_read(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages);
UINT _lx_nand_flash_simulator_pages_write(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages);
UINT _lx_nand_flash_simulator_pages_copy(LX_NAND_FLASH *nand_flash, ULONG source_block, ULONG source_page, ULONG destination_block, ULONG destination_page, ULONG pages, UCHAR *data_buffer);
#else
UINT _lx_nand_flash_simulator_read(ULONG block, ULONG page, ULONG *destination, ULONG words);
UINT _lx_nand_flash_simulator_write(ULONG block, ULONG page, ULONG *source, ULONG words);
UINT _lx_nand_flash_simulator_block_erase(ULONG block, ULONG erase_count);
UINT _lx_nand_flash_simulator_block_erased_verify(ULONG block);
UINT _lx_nand_flash_simulator_page_erased_verify(ULONG block, ULONG page);
UINT _lx_nand_flash_simulator_block_status_get(ULONG block, UCHAR *bad_block_byte);
UINT _lx_nand_flash_simulator_block_status_set(ULONG block, UCHAR bad_block_byte);
UINT _lx_nand_flash_simulator_extra_bytes_get(ULONG block, ULONG page, UCHAR *destination, UINT size);
UINT _lx_nand_flash_simulator_extra_bytes_set(ULONG block, ULONG page, UCHAR *source, UINT size);
UINT _lx_nand_flash_simulator_system_error(UINT error_code, ULONG block, ULONG page);

UINT _lx_nand_flash_simulator_pages_read(ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages);
UINT _lx_nand_flash_simulator_pages_write(ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages);
UINT _lx_nand_flash_simulator_pages_copy(ULONG source_block, ULONG source_page, ULONG destination_block, ULONG destination_page, ULONG pages, UCHAR *data_buffer);
#endif

UINT _lx_nand_flash_simulator_initialize(LX_NAND_FLASH *nand_flash)
{
    /* Setup geometry of the NAND flash.  */
    nand_flash->lx_nand_flash_total_blocks = TOTAL_BLOCKS;
    nand_flash->lx_nand_flash_pages_per_block = PHYSICAL_PAGES_PER_BLOCK;
    nand_flash->lx_nand_flash_bytes_per_page = BYTES_PER_PHYSICAL_PAGE;

    /* Setup function pointers for the NAND flash services.  */
    // nand_flash -> lx_nand_flash_driver_read =                   _lx_nand_flash_simulator_read;
    // nand_flash -> lx_nand_flash_driver_write =                  _lx_nand_flash_simulator_write;
    nand_flash->lx_nand_flash_driver_block_erase = _lx_nand_flash_simulator_block_erase;
    nand_flash->lx_nand_flash_driver_block_erased_verify = _lx_nand_flash_simulator_block_erased_verify;
    nand_flash->lx_nand_flash_driver_page_erased_verify = _lx_nand_flash_simulator_page_erased_verify;
    nand_flash->lx_nand_flash_driver_block_status_get = _lx_nand_flash_simulator_block_status_get;
    nand_flash->lx_nand_flash_driver_block_status_set = _lx_nand_flash_simulator_block_status_set;
    // nand_flash -> lx_nand_flash_driver_extra_bytes_get =        _lx_nand_flash_simulator_extra_bytes_get;
    // nand_flash -> lx_nand_flash_driver_extra_bytes_set =        _lx_nand_flash_simulator_extra_bytes_set;
    nand_flash->lx_nand_flash_driver_system_error = _lx_nand_flash_simulator_system_error;

    nand_flash->lx_nand_flash_driver_pages_read = _lx_nand_flash_simulator_pages_read;
    nand_flash->lx_nand_flash_driver_pages_write = _lx_nand_flash_simulator_pages_write;
    nand_flash->lx_nand_flash_driver_pages_copy = _lx_nand_flash_simulator_pages_copy;

    nand_flash->lx_nand_flash_spare_data1_offset = SPARE_DATA1_OFFSET;
    nand_flash->lx_nand_flash_spare_data1_length = SPARE_DATA1_LENGTH;

    nand_flash->lx_nand_flash_spare_data2_offset = SPARE_DATA2_OFFSET;
    nand_flash->lx_nand_flash_spare_data2_length = SPARE_DATA2_LENGTH;

    nand_flash->lx_nand_flash_spare_total_length = SPARE_BYTES_PER_PAGE;

    /* Return success.  */
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_pages_read(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages)
#else
UINT _lx_nand_flash_simulator_pages_read(ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages)
#endif
{
    UINT i;

    for (i = 0; i < pages; i++) {
        int ret = chry_sflash_nandflash_read(&g_nandflash, block, page + i, (uint8_t *)(main_buffer + i * BYTES_PER_PHYSICAL_PAGE), BYTES_PER_PHYSICAL_PAGE, spare_buffer + i * SPARE_BYTES_PER_PAGE, SPARE_BYTES_PER_PAGE);
        if (ret < 0) {
            return (LX_ERROR);
        }
    }
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_pages_write(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages)
#else
UINT _lx_nand_flash_simulator_pages_write(ULONG block, ULONG page, UCHAR *main_buffer, UCHAR *spare_buffer, ULONG pages)
#endif
{
    UINT i;

    for (i = 0; i < pages; i++) {
        int ret = chry_sflash_nandflash_write(&g_nandflash, block, page, (uint8_t *)(main_buffer + i * BYTES_PER_PHYSICAL_PAGE), BYTES_PER_PHYSICAL_PAGE, spare_buffer + i * SPARE_BYTES_PER_PAGE, SPARE_BYTES_PER_PAGE);
        if (ret < 0) {
            return (LX_INVALID_WRITE);
        }
    }
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_pages_copy(LX_NAND_FLASH *nand_flash, ULONG source_block, ULONG source_page, ULONG destination_block, ULONG destination_page, ULONG pages, UCHAR *data_buffer)
#else
UINT _lx_nand_flash_simulator_pages_copy(ULONG source_block, ULONG source_page, ULONG destination_block, ULONG destination_page, ULONG pages, UCHAR *data_buffer)
#endif
{
    UINT i;
    UINT status = LX_SUCCESS;

    for (i = 0; i < pages; i++) {
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
        status = _lx_nand_flash_simulator_pages_read(nand_flash, source_block, source_page + i, data_buffer, data_buffer + BYTES_PER_PHYSICAL_PAGE, 1);
#else
        status = _lx_nand_flash_simulator_pages_read(source_block, source_page + i, data_buffer, data_buffer + BYTES_PER_PHYSICAL_PAGE, 1);
#endif
        if (status != LX_SUCCESS && status != LX_NAND_ERROR_CORRECTED) {
            break;
        }
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
        status = _lx_nand_flash_simulator_pages_write(nand_flash, destination_block, destination_page + i, data_buffer, data_buffer + BYTES_PER_PHYSICAL_PAGE, 1);
#else
        status = _lx_nand_flash_simulator_pages_write(destination_block, destination_page + i, data_buffer, data_buffer + BYTES_PER_PHYSICAL_PAGE, 1);
#endif
        if (status != LX_SUCCESS) {
            break;
        }
    }
    return (status);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_block_erase(LX_NAND_FLASH *nand_flash, ULONG block, ULONG erase_count)
#else
UINT _lx_nand_flash_simulator_block_erase(ULONG block, ULONG erase_count)
#endif
{
    LX_PARAMETER_NOT_USED(erase_count);
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif

    chry_sflash_nandflash_erase(&g_nandflash, block);
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_block_erased_verify(LX_NAND_FLASH *nand_flash, ULONG block)
#else
UINT _lx_nand_flash_simulator_block_erased_verify(ULONG block)
#endif
{
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif
    /* Return success.  */
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_page_erased_verify(LX_NAND_FLASH *nand_flash, ULONG block, ULONG page)
#else
UINT _lx_nand_flash_simulator_page_erased_verify(ULONG block, ULONG page)
#endif
{
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif

    /* Return success.  */
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_block_status_get(LX_NAND_FLASH *nand_flash, ULONG block, UCHAR *bad_block_byte)
#else
UINT _lx_nand_flash_simulator_block_status_get(ULONG block, UCHAR *bad_block_byte)
#endif
{
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif

    /* Pickup the bad block byte and return it.  */
    chry_sflash_nandflash_read(&g_nandflash, block, 0, NULL, 0, space_area_buffer, SPARE_BYTES_PER_PAGE);
    *bad_block_byte = (UCHAR)space_area_buffer[BAD_BLOCK_POSITION];
    /* Return success.  */
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_block_status_set(LX_NAND_FLASH *nand_flash, ULONG block, UCHAR bad_block_byte)
#else
UINT _lx_nand_flash_simulator_block_status_set(ULONG block, UCHAR bad_block_byte)
#endif
{
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif

    /* Set the bad block byte.  */
    chry_sflash_nandflash_read(&g_nandflash, block, 0, NULL, 0, space_area_buffer, SPARE_BYTES_PER_PAGE);
    space_area_buffer[BAD_BLOCK_POSITION] = bad_block_byte;
    chry_sflash_nandflash_write(&g_nandflash, block, 0, NULL, 0, space_area_buffer, SPARE_BYTES_PER_PAGE);
    /* Return success.  */
    return (LX_SUCCESS);
}

#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
UINT _lx_nand_flash_simulator_system_error(LX_NAND_FLASH *nand_flash, UINT error_code, ULONG block, ULONG page)
#else
UINT _lx_nand_flash_simulator_system_error(UINT error_code, ULONG block, ULONG page)
#endif
{
    LX_PARAMETER_NOT_USED(error_code);
    LX_PARAMETER_NOT_USED(block);
    LX_PARAMETER_NOT_USED(page);
#ifdef LX_NAND_ENABLE_CONTROL_BLOCK_FOR_DRIVER_INTERFACE
    LX_PARAMETER_NOT_USED(nand_flash);
#endif

    /* Custom processing goes here...  all errors except for LX_NAND_ERROR_CORRECTED are fatal.  */
    return (LX_ERROR);
}

/* Create a NAND flash control block.  */

LX_NAND_FLASH       g_lx_nandflash;

/* Memory buffer size should be at least 8 * total block count + 2 * page size,
   that is 8 * 1024 + 2 * (2048+64) = 8224 bytes */
ULONG               lx_memory_buffer[(8 * 1024 + 2 * (2048 + 64)) / sizeof(ULONG)];

UINT lx_nand_flash_open_with_format(void)
{
    UCHAR status, status2;

    do {
        /* Open the NAND flash simulation.  */
        status = _lx_nand_flash_open(&g_lx_nandflash, "sim nand flash", _lx_nand_flash_simulator_initialize, lx_memory_buffer, sizeof(lx_memory_buffer));
        if (status != LX_SUCCESS) {
            printf("Failed to open NAND flash simulator, status: %d, reformatting now\r\n", status);
            status2 = _lx_nand_flash_format(&g_lx_nandflash, "sim nand flash", _lx_nand_flash_simulator_initialize, lx_memory_buffer, sizeof(lx_memory_buffer));
            if (status2 != LX_SUCCESS) {
                printf("Failed to format NAND flash simulator, status: %d\r\n", status2);
                /* Return an I/O error to FileX.  */
                return LX_ERROR;
            }
        }
    } while (status != LX_SUCCESS);

    return LX_SUCCESS;
}
/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions adapted for New Device Flash
 * @version  V1.0.0
 * @date     10. January 2018
 ******************************************************************************/
/*
 * Copyright (c) 2010-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "FlashOS.H"        // FlashOS Structures
#include "sys.h"
#include "qspi.h"
#include "chry_sflash_norflash.h"

#define PAGE_SIZE            4096
/* 
   Mandatory Flash Programming Functions (Called by FlashOS):
                int Init        (unsigned long adr,   // Initialize Flash
                                 unsigned long clk,
                                 unsigned long fnc);
                int UnInit      (unsigned long fnc);  // De-initialize Flash
                int EraseSector (unsigned long adr);  // Erase Sector Function
                int ProgramPage (unsigned long adr,   // Program Page Function
                                 unsigned long sz,
                                 unsigned char *buf);

   Optional  Flash Programming Functions (Called by FlashOS):
                int BlankCheck  (unsigned long adr,   // Blank Check
                                 unsigned long sz,
                                 unsigned char pat);
                int EraseChip   (void);               // Erase complete Device
      unsigned long Verify      (unsigned long adr,   // Verify Function
                                 unsigned long sz,
                                 unsigned char *buf);

       - BlanckCheck  is necessary if Flash space is not mapped into CPU memory space
       - Verify       is necessary if Flash space is not mapped into CPU memory space
       - if EraseChip is not provided than EraseSector for all sectors is called
*/


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */
uint8_t aux_buf[PAGE_SIZE] ;
uint32_t base_adr;
static struct chry_sflash_norflash   flash;
static struct chry_sflash_host spi_host;
int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
	base_adr = adr;	
	if(fnc == 1){
		memset(&flash,0,sizeof(flash));
		memset(&spi_host,0,sizeof(spi_host));		
		spi_host.spi_idx = 0;
		spi_host.iomode = CHRY_SFLASH_IOMODE_QUAD;
		QSPI_Init();	
		chry_sflash_init(&spi_host);
		chry_sflash_norflash_init(&flash, &spi_host);	
	}
  return (0);                                  // Finished without Errors
}


/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {


  /* Add your Code */
  return (0);                                  // Finished without Errors
}


/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseChip (void) {

  /* Add your Code */
  return (0);                                  // Finished without Errors
}


/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */

int EraseSector (unsigned long adr) {

  /* Add your Code */
	chry_sflash_norflash_erase(&flash,adr - base_adr, PAGE_SIZE);
  return (0);                                  // Finished without Errors
}
/*
 *  Blank Check Checks if Memory is Blank
 *    Parameter:      adr:  Block Start Address
 *                    sz:   Block Size (in bytes)
 *                    pat:  Block Pattern
 *    Return Value:   0 - OK,  1 - Failed
 */

int BlankCheck (unsigned long adr, unsigned long sz, unsigned char pat) {

	return (1);                                        /* Always Force Erase */
}

/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {
   chry_sflash_norflash_write(&flash, adr - base_adr,buf , sz);
  return (0);                                  // Finished without Errors
}
/*  
 *  Verify Flash Contents
 *    Parameter:      adr:  Start Address
 *                    sz:   Size (in bytes)
 *                    buf:  Data
 *    Return Value:   (adr+sz) - OK, Failed Address
 */

/*
   Verify function is obsolete because all other function leave 
    the SPIFI in memory mode so a memory compare could be used.
 */
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf) {
    uint32_t i;

    uint32_t offset = adr - base_adr;

    while (sz) {
        uint32_t chunk = sz > PAGE_SIZE ? PAGE_SIZE : sz;

        chry_sflash_norflash_read(&flash, offset,aux_buf, chunk);  

        for (i = 0; i < chunk; i++) {
            if (aux_buf[i] != buf[i]) {
                return adr + i;  
            }
        }

        offset += chunk;
        buf += chunk;
        adr += chunk;
        sz -= chunk;
    }

    return (adr);  
}

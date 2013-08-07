#include <windows.h>
#include <ceddk.h>


//static DWORD FLASH_SIZE    = (16777216 * 2);   // Paired 16MB parts.
static DWORD FLASH_SIZE    = (33554432*2);     // for Pared 32MB P30.
static DWORD ERASE_BLOCKS  = 256;              // Total number of blocks (hiding parameter blocks).
static DWORD L3_BLOCK_SIZE = 65536;            // 64KB per block on L3 (blocks 0 - 3).



// START: these routines will be replaced by the FMD when it's done.
// TODO - clean these routines up.
DWORD GetFlashID(PBYTE lpFlashBaseAddress)
{
    DWORD nDeviceID = 0;
    volatile DWORD *pFlash = (volatile DWORD *)(lpFlashBaseAddress);

    // Get the flash part ID.
    //
    *pFlash   = 0x00900090;
    nDeviceID = (*(pFlash + 1) & 0xFFFF);

    // Put the flash part in read array mode.
    //
    *pFlash   = 0x00FF00FF;


    //EdbgOutputDebugString("INFO: DetectFlashDevice: flash type is undetermined.\r\n");
    return(nDeviceID);
}


static BOOL FlashErase(PBYTE lpFlashBaseAddress, DWORD FlashLength)
{
    DWORD i,j;
    DWORD num_blocks;
    DWORD num_blocks_to_erase;
    DWORD num_l3_blocks_to_erase = 0;
    DWORD num_l3_blocks_erased;
    volatile DWORD *pFlash;
    DWORD status;
    DWORD BLOCK_SIZE = (FLASH_SIZE / ERASE_BLOCKS);
   

    // Determine the number of blocks to erase.
    num_blocks = (FlashLength / (BLOCK_SIZE));
    
    if (FlashLength % (BLOCK_SIZE))
    {
        num_blocks++;
    }

    if (num_blocks < 1)
    {
        num_blocks = 1;
        //EdbgOutputDebugString("WARNING: FlashErase: calculation error.  Erase blocks = %d\n\r", num_blocks);
        //PrintString("Block calculation error.\r\n");
    }

    else if (num_blocks > ERASE_BLOCKS)
    {
        num_blocks = ERASE_BLOCKS;
        //EdbgOutputDebugString("WARNING: FlashErase: calculation error.  Erase blocks = %d\n\r", num_blocks);
        //PrintString("Block calculation error.\r\n");
    }

    

    pFlash = (volatile DWORD *)(lpFlashBaseAddress);

   
    // For K3/K18 FLASH, unlock individual blocks one at a time
    num_blocks_to_erase = num_blocks;

    // For L3, need to set for use within the FOR loop
    num_l3_blocks_erased = num_l3_blocks_to_erase;

    for (j = 0; j < num_blocks_to_erase; j++)
    {
        *pFlash = 0x00600060;
        *pFlash = 0x00d000d0;

        while ((*pFlash & 0x00800080) != 0x00800080) ;

        i = *pFlash;
        if ((i & 0x00000008) == 0x00000008)
            //EdbgOutputDebugString("ERROR: FlashErase: voltage range error ... lower flash.\r\n");
        if ((i & 0x00080000) == 0x00080000)
            //EdbgOutputDebugString("ERROR: FlashErase: voltage range error ... upper flash.\r\n");

        if ((i & 0x00000030) == 0x00000030)
            //EdbgOutputDebugString("ERROR: FlashErase: command sequence error ... lower flash.\r\n");
        if ((i & 0x00300000) == 0x00300000)
            //EdbgOutputDebugString("ERROR: FlashErase: command sequence error ... upper flash.\r\n");

        if ((i & 0x00000020) == 0x00000020)
            //EdbgOutputDebugString("ERROR: FlashErase: clear lock bits error ... lower flash.\r\n");
        if ((i & 0x00200000) == 0x00200000)
            //EdbgOutputDebugString("ERROR: FlashErase: clear lock bits error ... upper flash.\r\n");

        if (i != 0x00800080)
        {
            //EdbgOutputDebugString("ERROR: FlashErase: status register returned 0x%X\r\n", i);
            //EdbgOutputDebugString("ERROR: FlashErase: unrecoverable failure encountered while erasing flash.  System halted!\r\n");
            return(FALSE);
        }
     
            pFlash += (BLOCK_SIZE / 4);
    }
    pFlash = (volatile DWORD *)(lpFlashBaseAddress);

    // Clear the status register
    *pFlash = 0x00500050;

 

    // For L3, need to set for use within the FOR loop
    num_l3_blocks_erased = num_l3_blocks_to_erase;

    // Erase each block.
    for (j = 0; j < num_blocks; j++)
    {
        // Issue erase and confirm command
        *pFlash = 0x00200020;
        *pFlash = 0x00d000d0;
        while ((*pFlash & 0x00800080) != 0x00800080);

        // Check if the flash block erased successfully.
        // Was either block locked?
        status = *pFlash;
        if ((status & 0x00200000) || (status & 0x00000020))
        {
            if (status & 0x00200000)
            {
                //EdbgOutputDebugString("ERROR: FlashErase: block erase failure.  Lock bit upper flash set!\r\n");
            }

            if (status & 0x00000020)
            {
                //EdbgOutputDebugString("ERROR: FlashErase: block erase failure.  Lock bit lower flash set!\r\n");
            }

            //EdbgOutputDebugString("ERROR: FlashErase: unrecoverable failure encountered while erasing flash.\r\n");
            return(FALSE);
        }
        //EdbgOutputDebugString(".");
        //PrintString(".");

      
            pFlash += (BLOCK_SIZE / 4);
    }

    //EdbgOutputDebugString("\r\n");
    //PrintString("\r\n");

    pFlash = (volatile DWORD *)(lpFlashBaseAddress);

    // Put the flash back into read mode
        *pFlash = 0x00FF00FF;

    // Flash erase verification.
    for (i = 0; i < FlashLength / 4; i++)
    {
        if (*pFlash++ != 0xFFFFFFFF)
        {
            //EdbgOutputDebugString("ERROR: FlashErase: erase verification failure at address 0x%X Data 0x%X.\r\n", pFlash-1, *(pFlash-1));
            //EdbgOutputDebugString("ERROR: FlashErase: unrecoverable failure encountered while erasing flash.\r\n");
            return(FALSE);
        }
    }


    return(TRUE);
}

// TODO - block-aligned address.
BOOL FlashWrite(PBYTE lpFlashBaseAddress, UINT8 *pDataBuffer, DWORD DataLength)
{
    DWORD i,j,b;
    DWORD sizeFlashCache; // size in bytes of the flash cache
    DWORD chunksPerBlock;
    DWORD count;
    DWORD val = 0;
    DWORD dwLength = DataLength;

    volatile DWORD *pFlash         = (volatile DWORD *) lpFlashBaseAddress;
    volatile DWORD *pBlockAddress  = (volatile DWORD *) lpFlashBaseAddress;
    volatile DWORD *pDeviceAddress = (volatile DWORD *) lpFlashBaseAddress;
    volatile DWORD *pFlashCache    = (DWORD *) pDataBuffer;

    volatile DWORD *pdwFlashStart;
    volatile DWORD *pdwRamStart;

    
    	
     if (!FlashErase(lpFlashBaseAddress, DataLength))
     { 
        return(FALSE);
     }
      
    b = 0;

  
        sizeFlashCache = 128;  // 128 bytes total, 64 bytes per device
        chunksPerBlock = 2048; // 256K / 128bytes = 2048

    //EdbgOutputDebugString("INFO: FlashWrite: writing to flash...\r\n");
    //PrintString("\r\nFlash writing...\r\n");
    while (DataLength > 0)
    {
        // Calculate the number of DWORDS to program in each iteration based on the size in bytes
        // of the flash cache.
        if (DataLength > sizeFlashCache)
            count = sizeFlashCache / 4;
        else
            count = DataLength / 4;

        // Issue Write to Buffer Command
        *pBlockAddress = 0x00E800E8;

        // Check that the write buffer is available.
        // Read the Extended Status Register and
        // wait for the assertion of XSR[7] before continuing.
        while ( (*pBlockAddress & 0x00800080) != 0x00800080)
        {
            *pBlockAddress = 0x00E800E8;
        }

        // Configure the size of the buffer that we will write.
        // Write the word count (device expects a 0 based number)
        *pBlockAddress = ((count - 1) << 16 | (count - 1));

        pdwFlashStart = pDeviceAddress;
        pdwRamStart   = pFlashCache;

        // Write up to "count" DWORDS into the Flash memory staging area
        for (i = 0; i < count; i++)
        {
            *pDeviceAddress++ = *pFlashCache++;
        }

        // increment the number of 64 or 128 byte segments written.
        b++;

        // Now program the buffer into Flash
        *pBlockAddress = 0x00D000D0;

        i = 0;
        while ((i & 0x00800080) != 0x00800080)
        {
            i = *pBlockAddress;
        }

        DataLength -= count << 2;

        // Read back the segment just written
        *pFlash = 0x00FF00FF;
        for (i = 0; i < count; i++)
        {
          

            if (*pdwFlashStart++ != *pdwRamStart++)
            {
                //EdbgOutputDebugString("ERROR: FlashWrite: flash programming failure at address 0x%x (expected 0x%x actual 0x%x).\r\n", pdwFlashStart-1, *(pdwRamStart-1), *(pdwFlashStart-1));
                //PrintString("Programming failure!!\r\n");
                return(FALSE);
            }
        }

        if ((b % chunksPerBlock) == 0)
        {
            //EdbgOutputDebugString(".");
            //PrintString(".");
        }
    }
    //PrintString("\r\n");

    // Verify the data written to flash...
    //
    pFlash      = (volatile DWORD *) lpFlashBaseAddress;
    pFlashCache = (DWORD *) pDataBuffer;

    // Put Flash into read mode.
    *pFlash = 0x00FF00FF;

    //EdbgOutputDebugString("INFO: FlashWrite: verifying the data written to flash...\r\n");
    //PrintString("Flash data verify...");
 
    for ( j = 0; j < dwLength/4; j++ )
    {
        if (*pFlash++ != *pFlashCache++)
        {
            //EdbgOutputDebugString( "ERROR: FlashWrite: verification failure at address 0x%x (expected 0x%x actual 0x%x))\r\n", pFlash-1, *(pFlashCache-1), *(pFlash-1) );
            //PrintString("Flash data verification failure!!");
            return(FALSE);
        }
    }

    //EdbgOutputDebugString("INFO: FlashWrite: flash programmed successfully!\r\n");
    //PrintString("\r\nFlash programmed successfully!\r\n");

    return(TRUE);
}
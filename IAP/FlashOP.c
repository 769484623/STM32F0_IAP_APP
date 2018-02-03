#include "FlashOP.h"
#include "stm32f0xx.h"
#include "stm32f0xx_flash.h"

// 64 pages - 3 IAP pages
#define ALLFlashPage 61
void APPFlashErase(uint16_t Sector)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
	while(FLASH_ErasePage(APPStartFlashAddr + Sector * 1024) != FLASH_COMPLETE);
	FLASH_Lock();
}
void FlashWrite(uint32_t StartAddr,uint8_t* Buffer,uint16_t Length)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
	uint16_t TempBuffer = 0;
	for(uint16_t i = 0; i < Length; i += 2)
	{
		TempBuffer = Buffer[i] | 0xFF00;
		if( i + 1 < Length)
		{
			TempBuffer &= ((Buffer[i + 1] << 8) | 0x00FF);
		}
		FLASH_ProgramHalfWord(StartAddr + i,TempBuffer);
	}
	FLASH_Lock();
}

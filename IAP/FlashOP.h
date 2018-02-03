#ifndef _FlashOP
#define _FlashOP
#include "stdint.h"

#define APPStartFlashAddr 0x08000C00
void FlashWrite(uint32_t StartAddr,uint8_t* Buffer,uint16_t Length);
void APPFlashErase(uint16_t Sector);

#endif

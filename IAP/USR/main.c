#include "main.h"
#include "BSP.h"
#include "FlashOP.h"

typedef void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;

int main(void)
{
  if (IAPDownloadCheck())
  {
		IAP_Init();
    while (DownloadFirmware() == -1)
      ;
  }
  else
  {
    if (((*(__IO uint32_t *)APPStartFlashAddr) & 0x2FFE0000) == 0x20000000)
    {
			/* DeInit GPIO */
			GPIO_DeInit(GPIOB);
      /* Jump to user application */
      JumpAddress = *(__IO uint32_t *)(APPStartFlashAddr + 4);
      Jump_To_Application = (pFunction)JumpAddress;

      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t *)APPStartFlashAddr);

      /* Jump to application */
      Jump_To_Application();
    }
  }
  while (1)
  {
  };
}

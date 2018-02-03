#include "stm32f0xx.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_usart.h"
#include "stm32f0xx_rcc.h"
#include "stm32f0xx_crc.h"
#include "FlashOP.h"

volatile uint32_t Systimer = 0;
void sTick() //start Sys tick 20ms
{
    if (SysTick_Config(SystemCoreClock / 1000)) //10,100ms;50,20ms;1000,1ms;100000,10us;1000000,1us;
    {
        while (1)
            ;
    }
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}
void SysTick_Handler(void)
{
    Systimer++;
}
void SystemClockConfig()
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}
void GPIOConfig()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
}
void USARTConfig()
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}
void IAP_Init(void)
{
    SystemClockConfig();
    GPIOConfig();
    USARTConfig();
    sTick();
}
uint8_t IAPDownloadCheck()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0)
    {
        return 1;
    }
    return 0;
}
uint8_t CRC_CalcBlockCRC8Bit(uint8_t data[], uint32_t BufferLength)
{
    uint8_t i;
    uint8_t crc = 0; // Initial value
    while (BufferLength--)
    {
        crc ^= *data++; // crc ^= *data; data++;
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}
#define Timeout 5
uint16_t USART_RecvByte(USART_TypeDef *USARTx)
{
    uint32_t LocalTime = 0;
    uint8_t Factor = 0;
    if (Systimer + Timeout < Systimer) //Overflow
    {
        Factor = Timeout;
    }
    LocalTime = Systimer;
    for (; (LocalTime + Timeout + Factor > Systimer + Factor) && USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET;)
        ;
    return (LocalTime + Timeout + Factor <= Systimer + Factor) ? 0x8000 : ((uint16_t)(USARTx->RDR & (uint16_t)0x01FF));
}
void USART_SendByte(USART_TypeDef *USARTx, uint8_t Data)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
        ;
    USART_SendData(USARTx, Data);
}

#define USART_ACK 0x3C
#define USART_PACKET_TOO_LONG 0x5F
#define USART_CRC_ERROR 0x0F
#define DownloadBufferLength 1024
int8_t DownloadFirmware(void)
{
    volatile uint32_t DownloadLength = 0;
    volatile uint16_t ControlVal = 0, BufferLength = 0,CRCVal = 0;
    uint8_t DownloadBuffer[DownloadBufferLength] = {0};
    uint8_t CRC_Calc = 0;
    do
    {
        ControlVal = USART_RecvByte(USART1);
        if (ControlVal & 0x8000)
        {
            return -1;
        }
        else if (ControlVal == 0x00)//download
        {
            uint16_t BufferLength_Slices = 0;
            BufferLength = 0;
            for (uint8_t i = 0; i < 2; i++)
            {
                BufferLength_Slices = USART_RecvByte(USART1);
                if (BufferLength_Slices & 0x8000)
                {
                    return -1;
                }
                BufferLength |= (BufferLength_Slices << (8 * i));
            }
            CRCVal = USART_RecvByte(USART1);
            if (CRCVal & 0x8000)
            {
                return -1;
            }
            if (BufferLength > DownloadBufferLength)
            {
                USART_SendByte(USART1, USART_PACKET_TOO_LONG);
                continue;
            }
            else
            {
                USART_SendByte(USART1, USART_ACK);
                for (uint16_t i = 0; i < BufferLength; i++)
                {
                    uint16_t RecvByte = USART_RecvByte(USART1);
                    if (RecvByte & 0x8000)
                    {
                        return -1;
                    }
                    DownloadBuffer[i] = RecvByte;
                }
                CRC_Calc = CRC_CalcBlockCRC8Bit(DownloadBuffer, BufferLength);
                if (CRCVal != CRC_Calc)
                {
                    USART_SendByte(USART1, USART_CRC_ERROR);
                }
                else
                {
                    if (DownloadLength % 1024 == 0)
                    {
                        uint16_t Sector = (DownloadLength + BufferLength) / 1024;
                        APPFlashErase(Sector);
                    }
                    FlashWrite(APPStartFlashAddr + DownloadLength, DownloadBuffer, BufferLength);
                    USART_SendByte(USART1, USART_ACK);
                    DownloadLength += BufferLength;
                }
            }
        }
    } while (ControlVal != 0xFF);
    return 0;
}

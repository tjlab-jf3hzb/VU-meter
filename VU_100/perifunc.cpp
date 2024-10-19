/*--------------------------------------------------------------

"perifunc.cpp"

Oct. 15, 2024
T.Uebo,  Tj Lab

----------------------------------------------------------------*/
#include "Arduino.h"
#include "perifunc.h"
#include <ch32v20x.h>
#include <SPI.h>

/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::Interrupt_setup(void)
{
   NVIC_InitTypeDef NVIC_InitStructure={0};

   NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn ;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}



/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::TIM2_setup( uint16_t tp, uint16_t ps)
//       tp: Period,   ps: prescaler 
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseInitStructure.TIM_Period = tp-1;
  TIM_TimeBaseInitStructure.TIM_Prescaler = ps-1;
  TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  TIM_ARRPreloadConfig(TIM2, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
}



/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::DMA_setup(
  DMA_Channel_TypeDef *DMA_CHx, uint32_t ppadr, uint32_t memadr, uint16_t bufsize
  )
  {
	DMA_InitTypeDef DMA_InitStructure = {0};

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA_CHx);

	DMA_InitStructure.DMA_PeripheralBaseAddr = ppadr;
	DMA_InitStructure.DMA_MemoryBaseAddr = memadr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = bufsize;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA_CHx, &DMA_InitStructure);
}


/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::SPI_setup(void)
{
  SPI.begin();
  SPI_HandleTypeDef *h_spi=SPI.getHandle();
  SPI.setClockDivider(SPI_CLOCK_DIV4); //36MHz
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI_I2S_DMACmd( h_spi->Instance, SPI_I2S_DMAReq_Tx, ENABLE);

  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_RST, HIGH);
}



/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::SPI_TX_dma16(uint16_t *buff, uint32_t len)
{
  SPI_HandleTypeDef *h_spi=SPI.getHandle(); // "h_spi->Instance" = "SPI1"
  h_spi->Instance->CTLR1 |= SPI_CTLR1_DFF; //16bit DATA
  while(SPI_I2S_GetFlagStatus(h_spi->Instance, SPI_I2S_FLAG_BSY )==SET){}
  digitalWrite(TFT_CS, LOW);

	while( len > 0xffff ) {
		Peripheral_function::DMA_setup( DMA1_Channel3, (u32)&h_spi->Instance->DATAR, (uint32_t *)buff, 0xffff );
		DMA_Cmd(DMA1_Channel3, ENABLE);
		len -= 0xffff;
		while(!DMA_GetFlagStatus(DMA1_FLAG_TC3)) {};
	}

	if( len ) {
		Peripheral_function::DMA_setup( DMA1_Channel3, (u32)&h_spi->Instance->DATAR, (uint32_t *)buff, (uint16_t)len );
		DMA_Cmd(DMA1_Channel3, ENABLE);
		while(!DMA_GetFlagStatus(DMA1_FLAG_TC3)) {};
	}

  while(SPI_I2S_GetFlagStatus(h_spi->Instance, SPI_I2S_FLAG_BSY )==SET){}
  digitalWrite(TFT_CS, HIGH);
  h_spi->Instance->CTLR1 &= ~SPI_CTLR1_DFF; //8bit DATA
}



/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
void Peripheral_function::ADC_setup(void)
{
  ADC_InitTypeDef ADC_InitStructure;

  ADC_Cmd(ADC1, DISABLE); 
  ADC_DeInit(ADC1);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_ADCCLKConfig(RCC_PCLK2_Div4);

  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_InitStructure.ADC_OutputBuffer =ADC_OutputBuffer_Enable;
  ADC_InitStructure.ADC_Pga = ADC_Pga_1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC1->STATR = 0;
  ADC_Cmd(ADC1,ENABLE); 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_7Cycles5 );
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
  uint16_t dat=ADC1->RDATAR;
}
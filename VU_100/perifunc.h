#ifndef _PERI_FUNC_
#define _PERI_FUNC_

#include "prm.h"

class Peripheral_function {
public:
  void Interrupt_setup(void);
  void TIM2_setup(uint16_t tp, uint16_t ps);
  void DMA_setup(DMA_Channel_TypeDef *DMA_CHx, uint32_t ppadr, uint32_t memadr, uint16_t bufsize);
  void SPI_setup(void);
  void SPI_TX_dma16(uint16_t *buff, uint32_t len);
  void ADC_setup(void);

//protected:
//private:

};

#endif // _PERI_FUNC_
/*------------------------------------------------------------------------------

VU meter using CH32V203K8T6 & 128x160LCD(ST7735S)

Oct. 15, 2024
T.Uebo,  Tj Lab


Board maneger: CH32 MCU EVT Boards
Board select: CH32V203G8 EVT
Optimize: fastest -O3

Ver. 1.00  Oct. 15, 2024



TFT SPI
  RST:  PA4  10
  SCK:  PA5  11
  MISO: PA6  12
  MOSI: PA7  13
  CS:   PB0  14
  DC:   PB1  15

Audio input
  PA1  7

------------------------------------------------------------------------------*/
#include <SPI.h>
#include <ch32v20x.h>
#include "ST7735.h"
#include "prm.h"
#include "perifunc.h"
#include "math_func.h"
#include "VUmet_image150.h"

math_function M;
Peripheral_function PF;
ST7735 tft;

uint16_t Fbuf[W_fb*H_fb];
uint8_t Abuf[W_fb*(H_fb+2)];
int Ndiv;

#define Ringbuf_size 2048//
int16_t Rbuf[Ringbuf_size];
int pt_rb=0;

// 2nd order system parameter
float z=0.0f;
float zi0=0, zi1=0, zo0=0, zo1=0;
float fo_by_fs = fN;
float Q = 1.0f/(2.0f*DmpR);
float wn0 = 2.0f * pi_const * fo_by_fs;
float alpha = M._sin(wn0)/(2.0f*Q);
float b0 =  (1.0f - M._cos(wn0))/(2.0f*(1.0f+alpha));
float b1 =  (1.0f - M._cos(wn0))/(1.0f+alpha);
float b2 =  (1.0f - M._cos(wn0))/(2.0f*(1.0f+alpha));
float a1 =  -2.0f * M._cos(wn0)/(1.0f+alpha);
float a2 =   (1.0f - alpha)/(1.0f+alpha);
//

volatile uint16_t LED_Status = 0;
/*------------------------------------------------------------------------
-------------------------------------------------------------------------*/
void setup() {
  Ndiv=0;
  int H_size=0;
  while(H_size<H_LCD)
  {
    Ndiv++;
    H_size+=H_fb;
  }
  PF.Interrupt_setup();
  PF.TIM2_setup(50, 144);  // 144/144MHz * 50 = 1/20kHz
  PF.SPI_setup();
  PF.ADC_setup();

  pinMode(LED, OUTPUT);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(Rotation);
}


/*------------------------------------------------------------------------
-------------------------------------------------------------------------*/
void loop() {
  LED_Status = !LED_Status ;
  digitalWrite(LED, LED_Status);

  int32_t va=0;
  for(int i=0; i<Ringbuf_size; i++) va+=(int32_t)Rbuf[i];
  int32_t vavg=va/Ringbuf_size;

  float po=0;
  for(int i=0; i<Ringbuf_size; i++){
    float tmp=(float)(Rbuf[i]-vavg);
    po+=tmp*tmp;
  }
  po/=( (float)Ringbuf_size);

  float vrms = (17.78f/4096.0f) * M._sqrt(po); //vrms=1.0 at  0.7746/2[Vrms] (=0.5477[Vamp])

  // 2nd orfer system for needle motion --------------
  z = -a1*zo0 -a2*zo1 + vrms*b0 + zi0*b1 + zi1*b2;
  zi1=zi0; zi0=vrms;
  zo1=zo0; zo0=z;

  // Display process --------------------------------------------------------
  for(int i=0; i<Ndiv; i++)
  {
    int16_t yoff=H_fb*i;

    // Clear back ground
    for(int i=0; i<W_fb*H_fb; i++) Fbuf[i]=met_image[0];

    // Meter display
    disp_meter(7, 25, yoff);

    //Transfer data to LCD
    tft.setAddrWindow(0, yoff, W_fb, H_fb);
    uint32_t ylen = H_LCD - yoff;
    if(ylen>=H_fb) ylen=H_fb;
    PF.SPI_TX_dma16(Fbuf, W_fb*ylen);
  }
  //------------------------------------------------------------------------
}





/*------------------------------------------------------------------------
  Functions
-------------------------------------------------------------------------*/
void disp_meter(int16_t xmt, int16_t ymt, int16_t yoff)
{
  drawBitmap(xmt, ymt-yoff, (uint16_t *)met_image, met_Width, met_Height);
  needle(z, 2, 60, 107, ymt, yoff, cl_ND);
}


void drawBitmap(int xp, int yp, uint16_t *image, uint16_t im_Width, uint16_t im_Height){
  if( yp<H_fb && (yp+im_Height)>0 )
  {

    int ys=0;
    int ye=(int)im_Height;
    if(yp<0) ys=-yp;

    for(int y=ys; y<ye; y++){
      for(int x=0; x<(int)im_Width; x++){
        int pt_buf = x+xp+(y+yp)*W_fb;
        if(pt_buf>=0 && pt_buf<(W_fb*H_fb) )
        {
          int pt_img = x+y*im_Width;
          Fbuf[pt_buf]=image[pt_img];
        }
      }
    }

  }
}



#define Llim -0.43f   // Limit(L) (rad)
#define Hlim +0.43f   // Limit(H) (rad)
#define Ref_angl (+0.15f-0.005f)  // Ref_angl(rad) at meter input value of 1.0
void needle(float input, int16_t wd, int16_t r, int16_t len, int16_t pos, int16_t y_off, uint32_t cl){
  // angle: -0.43rad to +0.43rad
    float f, xo, yo, xf, yf, COS_, SIN_;
    int16_t xp,yp;

    float angle = Llim + input *(Ref_angl - Llim);
    if(angle<Llim) angle=Llim;
    if(angle>Hlim) angle=Hlim;

    int16_t nd_length=len;
    int16_t nd_bottom=r;
    int16_t nd_width=wd;
    int16_t rot_center= pos + 180;
    
    for(int i=0; i<W_fb*(H_fb+2); i++) Abuf[i]=0;

    COS_=M._cos(angle + pi_const);
    SIN_=M._sin(angle + pi_const);   
    float x_tmp;
    int k,j;
    for(j=0; j<nd_width; j++){
        for(k=0; k<=nd_length; k++){    
            xo= COS_*((float)j-0.5f*(float)nd_width )  -SIN_*(float)(k + nd_bottom);
            yo= COS_*(float)(k + nd_bottom) + SIN_*((float)j-0.5f*(float)nd_width );

            xf=0.5f*(float)W_LCD + xo;
            yf=yo + rot_center - (float)y_off;
            dot(xf,yf);         
        }
    }

    float ND_R= (float)( (cl>>16) & 0xff ); 
    float ND_G= (float)( (cl>> 8) & 0xff ); 
    float ND_B= (float)(  cl & 0xff ); 

// Alpha blend
    float c, a; 
    for(xp=0; xp<W_fb; xp++){
        for(yp=0; yp<H_fb; yp++){
            if(Abuf[xp+(yp+1)*W_fb]!=0){
                a=(float)Abuf[xp+(yp+1)*W_fb];
                uint16_t dd=Fbuf[xp+yp*W_fb];
                uint16_t vR=(dd>>8) &0x00F8;
                uint16_t vG=(dd>>3) &0x00FC;
                uint16_t vB=(dd<<3) &0x00F8;
                c=(float)vR;
                vR=(uint16_t)( c + (ND_R - c)*(a/255.0f) );                
                c=(float)vG;
                vG=(uint16_t)( c + (ND_G - c)*(a/255.0) );                
                c=(float)vB;
                vB=(uint16_t)( c + (ND_B - c)*(a/255.0) );
                Fbuf[xp+yp*W_fb]= (vR<<8)&0xF800 | (vG<<3)&0x07E0 | (vB>>3)&0x001F;
            }  
        }
    } 
}


void dot(float x, float y)
{
  int xd, yd, xu, yu;
  float Rxu, Rxd, Ryu, Ryd;
  int dat;
  int pt;
 
  if(y>=-1 && y<(H_fb+1) )
  {  
    y+=1.0f;

    xd = (int)x;
    yd = (int)y;
  
    if ( xd>=0 && xd<(W_fb-1) && yd<=H_fb ) {
      xu = xd + 1;
      yu = yd + 1;
  
      Rxd = ( (float)xu - x );
      Rxu = ( x - (float)xd );
      Ryd = ( (float)yu - y );
      Ryu = ( y - (float)yd );

      pt=xd+yd*W_fb;
      dat = (int)Abuf[pt] + (int)(Rxd * Ryd * 256.0);
      if (dat > 0xFF) dat = 0xFF;
      Abuf[pt] = (uint8_t)dat;

      pt=xu+yd*W_fb;
      dat = (int)Abuf[pt] + (int)(Rxu * Ryd * 256.0);
      if (dat > 0xFF) dat = 0xFF;
      Abuf[pt] = (uint8_t)dat;
  
      pt=xd+yu*W_fb;
      dat = (int)Abuf[pt] + (int)(Rxd * Ryu * 256.0);
      if (dat > 0xFF) dat = 0xFF;
      Abuf[pt] = (uint8_t)dat;
  
      pt=xu+yu*W_fb;
      dat = (int)Abuf[pt] + (int)(Rxu * Ryu * 256.0);
      if (dat > 0xFF) dat = 0xFF;
      Abuf[pt] = (uint8_t)dat;
    }
  }
}


/*------------------------------------------------------------------------
  Interrupt handler
-------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

void TIM2_IRQHandler(void)   __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void)
{
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    Rbuf[pt_rb]=ADC1->RDATAR;
    pt_rb++;
    pt_rb&=(Ringbuf_size-1);
}


#ifdef __cplusplus
}
#endif


// End ----------
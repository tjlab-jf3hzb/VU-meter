#ifndef _PRM_
#define _PRM_

//Needle motion parameters
#define Tau 250e-3
#define DmpR 0.7f
//Needle color
#define cl_ND 0xFF2000U


#define Vin PA1
#define TFT_CS  PB0
#define TFT_DC  PB1
#define TFT_RST PA4
#define LED PB5

#define W_LCD 160
#define H_LCD 128
#define Rotation 1
#define W_fb  W_LCD
#define H_fb  26

#define fs 31.5f //Hz
#define fN ( (3.0f/Tau)/(2.0f*pi_const)/fs )

#endif
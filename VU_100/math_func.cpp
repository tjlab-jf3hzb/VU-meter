/*--------------------------------------------------------------

"math_func.cpp"

Oct. 15, 2024
T.Uebo,  Tj Lab

----------------------------------------------------------------*/

#include "Arduino.h"
#include "math_func.h"



float trigo_func(float deg)
{
  float y;
  while(deg<0.0f) deg+=360.0f;
  while(deg>=360.0f) deg-=360.0f;

  int16_t degI=(int16_t)deg;
  float degF=deg-(float)degI;
  int indx_s, indx_e;
  float ys, ye;

  if(degI>=0 && degI<90){
    indx_s=90-degI;
    indx_e=90-(degI+1);
    ys= pgm_read_float(&sin_tbl[indx_s]);
    ye= pgm_read_float(&sin_tbl[indx_e]);
    y=(ye-ys)*degF + ys;
  }
  else if(degI>=90 && degI<180){
    indx_s=degI-90;
    indx_e=(degI+1)-90;
    ys= pgm_read_float(&sin_tbl[indx_s]);
    ye= pgm_read_float(&sin_tbl[indx_e]);
    y=(-ye+ys)*degF - ys;
  }
  else if(degI>=180 && degI<270){
    indx_s=270-degI;
    indx_e=270-(degI+1);
    ys= pgm_read_float(&sin_tbl[indx_s]);
    ye= pgm_read_float(&sin_tbl[indx_e]);
    y=(-ye+ys)*degF - ys;
  }
  else if(degI>=270){
    indx_s=degI-270;
    indx_e=(degI+1)-270;
    ys= pgm_read_float(&sin_tbl[indx_s]);
    ye= pgm_read_float(&sin_tbl[indx_e]);
    y=(ye-ys)*degF + ys;
  }
  return(y);
}



float log2(float x){
  float y, ys, ye;
  if(x>0.0f){
    int k=0;
    while(x<1.0f){ x*=2.0f; k--;}
    while(x>=2.0f){ x*=0.5f; k++;}
    x=100.0f*(x-1.0f);
    int xI=(int)x;
    float xf=x-(float)xI;
    ys= pgm_read_float(&log2_tbl[xI]);
    ye= pgm_read_float(&log2_tbl[xI+1]);
    y=(float)k+( (ye-ys)*xf + ys);
    return(y);
  }
  else
  {
    return(NAN);
  }
}


float pow2(float x){
  float y, ys, ye;

  int k=(int)x;
  x=x-(float)k;
  if(x<0){x+=1.0f, k--;}

  x=100.0f*x;
  int xI=(int)x;
  float xf=x-(float)xI;

  ys= pgm_read_float(&pow2_tbl[xI]);
  ye= pgm_read_float(&pow2_tbl[xI+1]);
  y=(ye-ys)*xf + ys;

  if(k<0){
    for(int i=k; i<0; i++) y*=0.5f;
  } else if(k>0){
    for(int i=k; i>0; i--) y*=2.0f;
  }
  return(y);
}


/*-----------------------------------------------------------------------------------
-------------------------------------------------------------------------------------*/
float math_function::_cos(float x)
{
  float deg=180.0f*x*inv_pi;
  return( trigo_func(deg) );
}

//
float math_function::_sin(float x)
{
  float deg=180.0f*x*inv_pi - 90.0f;
  return( trigo_func(deg) );
}

//
float math_function::_log10(float x)
{
  return( inv_log2_10 * log2(x) );
}

//
float math_function::_log(float x)
{
  return( inv_log2_e * log2(x) );
}

//
float math_function::_pow(float a, float b)
{ //a^b
  float y=pow2(b*log2(a));
  return(y);
} 



float math_function::_sqrt(float x)
{
  float y;
  y=1;
  for (int i=0; i<100; i++) y=(x/y+y)*0.5f;
  return(y);
}



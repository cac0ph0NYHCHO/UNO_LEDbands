#include "WS_Dout.h"

bool Failure_Flag = 0;
/*************************************************************  Dout I/O  *************************************************************/
bool Dout_Open(uint8_t CHx)
{
  if(!Set_EXIO(CHx, true)){
    printf("Failed to Open CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_Closs(uint8_t CHx)
{
  if(!Set_EXIO(CHx, false)){
    printf("Failed to LOW CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_CHx_Toggle(uint8_t CHx)
{
   if(!Set_Toggle(CHx)){
    printf("Failed to Toggle CH%d!!!\r\n", CHx);
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}
bool Dout_CHx(uint8_t CHx, bool State)
{
  bool result = 0;
  if(State)
    result = Dout_Open(CHx);
  else
    result = Dout_Closs(CHx);
  if(!result)
    Failure_Flag = 1;
  return result;
}
bool Dout_CHxs_PinState(uint8_t PinState)
{
  if(!Set_EXIOS(PinState)){
    printf("Failed to set the dout status!!!\r\n");
    Failure_Flag = 1;
    return 0;
  }
  return 1;
}

void Dout_Init(void)
{
  TCA9554PWR_Init(0x00, 0xFF);
}

/********************************************************  Data Analysis  ********************************************************/
bool Dout_Flag[8] = {1, 1, 1, 1, 1, 1, 1, 1};       // Dout current status flag

void Dout_Immediate(uint8_t CHx, bool State, uint8_t Mode_Flag)
{
  if(!CHx || CHx > 8){
    printf("Dout_Immediate(function): Incoming parameter error!!!!\r\n");
    Failure_Flag = 1;
  }
  else{
    uint8_t ret = 0;
    ret = Dout_CHx(CHx,State);                                               
    if(ret){
      Dout_Flag[CHx-1] = State;
      if(Dout_Flag[0])
        printf("|***  Dout CH%d High  ***|\r\n",CHx);
      else
        printf("|***  Dout CH%d Low ***|\r\n",CHx);
    }
  }
}

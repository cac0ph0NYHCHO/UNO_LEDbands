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

/********************************************************  Data Analysis  ********************************************************/
void Dout_Analysis(uint8_t *buf,uint8_t Mode_Flag)
{
  uint8_t ret = 0;
  if(Mode_Flag == Bluetooth_Mode_Trigger)
    printf("Bluetooth Data :\r\n");
  else if(Mode_Flag == WIFI_Mode_Trigger)
    printf("ETH Data :\r\n");
  else if(Mode_Flag == RS485_Mode_Trigger)
    printf("RS485 Data :\r\n");  
  switch(buf[0])
  {
    case CH1: 
      ret = Dout_CHx_Toggle(GPIO_PIN_CH1);
      if(ret){
        Dout_Flag[0] =! Dout_Flag[0];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[0])
          printf("|***  Dout CH1 High  ***|\r\n");
        else
          printf("|***  Dout CH1 Low ***|\r\n");
      }
      break;
    case CH2: 
      ret = Dout_CHx_Toggle(GPIO_PIN_CH2);
      if(ret){
        Dout_Flag[1] =! Dout_Flag[1];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[1])
          printf("|***  Dout CH2 High  ***|\r\n");
        else
          printf("|***  Dout CH2 Low ***|\r\n");
      }
      break;
    case CH3:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH3);
      if(ret){
        Dout_Flag[2] =! Dout_Flag[2];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[2])
          printf("|***  Dout CH3 High  ***|\r\n");
        else
          printf("|***  Dout CH3 Low ***|\r\n");
      }
      break;
    case CH4:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH4);
      if(ret){
        Dout_Flag[3] =! Dout_Flag[3];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[3])
          printf("|***  Dout CH4 High  ***|\r\n");
        else
          printf("|***  Dout CH4 Low ***|\r\n");
      }
      break;
    case CH5:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH5);
      if(ret){  
        Dout_Flag[4] =! Dout_Flag[4];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[4])
          printf("|***  Dout CH5 High  ***|\r\n");
        else
          printf("|***  Dout CH5 Low ***|\r\n");
      }
      break;
    case CH6:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH6);
      if(ret){
        Dout_Flag[5] =! Dout_Flag[5];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[5])
          printf("|***  Dout CH6 High  ***|\r\n");
        else
          printf("|***  Dout CH6 Low ***|\r\n");
      }
      break;
    case CH7:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH7);
      if(ret){
        Dout_Flag[6] =! Dout_Flag[6];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[6])
          printf("|***  Dout CH7 High  ***|\r\n");
        else
          printf("|***  Dout CH7 Low ***|\r\n");
      }
      break;
    case CH8:
      ret = Dout_CHx_Toggle(GPIO_PIN_CH8);
      if(ret){
        Dout_Flag[7] =! Dout_Flag[7];
        Buzzer_Open_Time(200, 0);
        if(Dout_Flag[7])
          printf("|***  Dout CH8 High  ***|\r\n");
        else
          printf("|***  Dout CH8 Low ***|\r\n");
      }
      break;
    case ALL_ON:
      ret = Dout_CHxs_PinState(0xFF);
      if(ret){
        memset(Dout_Flag,1, sizeof(Dout_Flag));
        printf("|***  Dout ALL High  ***|\r\n");
        Buzzer_Open_Time(500, 0);
      }
      break;
    case ALL_OFF:
      ret = Dout_CHxs_PinState(0x00);
      if(ret){
        memset(Dout_Flag,0, sizeof(Dout_Flag));
        printf("|***  Dout ALL Low ***|\r\n");
        Buzzer_Open_Time(500, 150); 
      }
      break;
    default:
      printf("Note : Non-instruction data was received !\r\n");
  }
}

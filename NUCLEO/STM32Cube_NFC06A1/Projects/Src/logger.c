/*********************************************************************************
* File Name :	logger.c
* Author:      ST-Micro & Paul Fritzen
* Description: Serial output log implementation file
*		          This driver provides a printf-like way to output log messages
*         	      via the UART interface as well as enables the reception of of
*		          messages via a UART Interrupt. It makes use of the uart driver.
**********************************************************************************
* Attention!
*
* Copyright (c) 2020 STMicroelectronics.
* All rights reserved.
*
* This software component is licensed by ST under Ultimate Liberty license
* SLA0094, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0094
*
********************************************************************************/

/* ------------------------- Includes ------------------------- */
#include "logger.h"
#include "st_errno.h"
#include "demo.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>




/* ------------------------- DEFINES ------------------------- */
#if (USE_LOGGER == LOGGER_ON)

#define MAX_HEX_STR         4
#define MAX_HEX_STR_LENGTH  128
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;
#endif /* #if USE_LOGGER == LOGGER_ON */


#if (USE_LOGGER == LOGGER_OFF && !defined(HAL_UART_MODULE_ENABLED))
  #define UART_HandleTypeDef void
#endif

#define USART_TIMEOUT          2000		/* Maximum Timeout values for flags waiting loops. These timeouts are not based
   	   	   	   	   	   	   	   	   	   	   on accurate values, they just guarantee that the application will not remain
   	   	   	   	   	   	   	   	   	   	   stuck if the UART communication is corrupted.
   	   	   	   	   	   	   	   	   	   	   You may modify these timeout values depending on CPU frequency and application
      	   	   	   	   	   	   	   	   	   conditions (interrupts routines ...). */





/* ------------------------- Private Variables ------------------------- */
uint8_t g_Rx_Data[ MAX_RX_SIZE ];	// placeholder Manufacturing Info Array, Total item count must be a multiple of 4
uint8_t g_bMsgReceived = 0;			// boolean flag used to signal when a message has been transmitted to the unit
UART_HandleTypeDef *pLogUsart;      /*!< pointer to the logger Handler */





/* ------------------------- Private Function Prototypes ------------------------- */
uint8_t logUsartTx(uint8_t *data, uint16_t dataLen);	// initializes the UART handle and UART IP





/* ---------------------------------  Functions  --------------------------------- */
/****************************************************************************
* Function Name    : logUsartInit
* Date             : unknown
* Author           : ST-Micro
* Description      : This function initializes the UART handle and UART IP.
*
* Input Parameters : husart, handle to USART HW
*
* Return           : none
*
*****************************************************************************/

// BEGIN logUsartInit
void logUsartInit(UART_HandleTypeDef *husart)
{
  uint8_t rxInitiator   = 0;	// index used to traverse Rx buffer and set all data to 0

  husart->Instance = USART2;
  husart->Init.BaudRate = 19200;
  husart->Init.WordLength = UART_WORDLENGTH_8B;
  husart->Init.StopBits = UART_STOPBITS_1;
  husart->Init.Parity = UART_PARITY_NONE;
  husart->Init.Mode = UART_MODE_TX_RX;
  husart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  husart->Init.OverSampling = UART_OVERSAMPLING_16;
  husart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  husart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(husart);
  
  pLogUsart = husart;

  // FOR each cell of the Rx Buffer set it to zero
  for (rxInitiator = 0; rxInitiator < sizeof(g_Rx_Data); rxInitiator++)
  {
	// set each cell to 0
  	g_Rx_Data[rxInitiator] = 0;
  }
  // END FOR
}
// END logUsartInit





/*****************************************************************************
* Function Name    : logUsartTx
* Date             : unknown
* Author           : ST-Micro
* Description      : This function Transmit data via USART
*
* Inputs		   : data, data to be transmitted
* @param[in]	   : dataLen, length of data to be transmitted
*
* Return           : HAL status or ERR_INVALID_HANDLE in case the UART HW
* 						is not initialized yet
*
*****************************************************************************/

// BEGIN logUsartInit
uint8_t logUsartTx(uint8_t *data, uint16_t dataLen)
{
  if(pLogUsart == 0)
    return ERR_INVALID_HANDLE;

#if (USE_LOGGER == LOGGER_ON)
{
    return HAL_UART_Transmit(pLogUsart, data, dataLen, USART_TIMEOUT);
}
#else
{
    return 0;
}
#endif /* #if USE_LOGGER == LOGGER_ON */

}
// END logUsartTx





/****************************************************************************
* Function Name    : logUsart
* Date             : unknown
* Author           : ST-Micro
* Description      : This function is used to write a formated string via
* 						the UART interface.
*
* Input Parameters : format, data to be transmitted
* Return		   : cnt, Number of data sent
*
*****************************************************************************/

// BEGIN logUsart
int logUsart(const char* format, ...)
{

// IF LOGGER is ON
#if (USE_LOGGER == LOGGER_ON)
{
    #define LOG_BUFFER_SIZE 256
    char buf[LOG_BUFFER_SIZE];
    va_list argptr;
    va_start(argptr, format);
    int cnt = vsnprintf(buf, LOG_BUFFER_SIZE, format, argptr);
    va_end(argptr);  
      
    /* */
    logUsartTx((uint8_t*)buf, strlen(buf));
    return cnt;
}

// ELSE LOGGER is OFF
#else
{
    return 0;
}

// END IF USE_LOGGER is ON
#endif

}
// END logUsart





/****************************************************************************
* Function Name    : hex2Str
* Date             : unknown
* Author           : ST-Micro
* Description      : This function converts hex data into a formated string
*
* Input Parameters : data, pointer to buffer to be dumped.
* 					 dataLen, buffer length
*
* Return		   : hexStr,pointer to converted data as hex formated string
*
*****************************************************************************/

// BEGIN hex2Str()
char* hex2Str(unsigned char * data, size_t dataLen)
{

// IF LOGGER is ON
#if (USE_LOGGER == LOGGER_ON)
{
	unsigned char * pin = data;
	const char * hex = "0123456789ABCDEF";
	char * pout = hexStr[hexStrIdx];
	uint8_t i = 0;
	uint8_t idx = hexStrIdx;

	if( dataLen > (MAX_HEX_STR_LENGTH/2) )
	{
	  dataLen = (MAX_HEX_STR_LENGTH/2);
	}

	if(dataLen == 0)
	{
	  pout[0] = 0;
	}
	else
	{
	  for(; i < dataLen - 1; ++i)
	  {
		  *pout++ = hex[(*pin>>4)&0xF];
		  *pout++ = hex[(*pin++)&0xF];
	  }
	  *pout++ = hex[(*pin>>4)&0xF];
	  *pout++ = hex[(*pin)&0xF];
	  *pout = 0;
	}

	hexStrIdx++;
	hexStrIdx %= MAX_HEX_STR;

	return hexStr[idx];
}

// ELSE
#else
{
	return NULL;
}

//END IF USE_LOGGER is ON
#endif

}
// END hex2Str()





/****************************************************************************
* Function Name    : init_UART_RX
* Date             : unknown
* Author           : ST-Micro & Paul Fritzen
* Description      : Initializes UART Reception via Hardware Interrupts
*
* Input Parameters : none
*
* Return		   : none
*
*****************************************************************************/

// BEGIN init_UART_RX
void init_UART_RX()
{
	// enable UART RX Interrupt
	HAL_UART_Receive_IT(pLogUsart, g_Rx_Data, MAX_RX_SIZE);
}
// END init_UART_RX





/****************************************************************************
* Function Name    : HAL_UART_RxCpltCallback
* Date             : unknown
* Author           : ST-Micro & Paul Fritzen
* Description      : This function is an Acknowledgment for UART Rx Completion
*
* Input Parameters : huart, UART handle.
*
* Return		   : none
*
*****************************************************************************/

// BEGIN HAL_UART_RxCpltCallback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *husart)
{
	// set msg received flag to true
	g_bMsgReceived = 1;

	// call function to re-enable UART Rx Interrupts
	HAL_UART_Receive_IT(pLogUsart, g_Rx_Data, MAX_RX_SIZE);
}
// END HAL_UART_RxCpltCallback





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

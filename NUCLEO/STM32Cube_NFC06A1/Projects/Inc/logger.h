/********************************************************************************
* File Name :	logger.h
* Author:      ST-Micro & Paul Fritzen
* Description: Serial output log declaration file
*		          This driver provides a printf-like way to output log messages
*         	      via the UART interface as well as enables the reception of of
*		          messages via a UART Interrupt. It makes use of the uart driver.
*
********************************************************************************
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
*******************************************************************************/





/* ------------------------- DEFINES ------------------------- */
#ifndef LOGGER_H	/* Define to prevent recursive inclusion */
#define LOGGER_H

#ifdef __cplusplus				// IF we are using C++
extern "C" {					// DEFINE C++ Stuff
#endif							// END IF

#if (USE_LOGGER == LOGGER_OFF && !defined(HAL_UART_MODULE_ENABLED))		// IF Logger is off and UART has not been enabled
  #define UART_HandleTypeDef void										// THEN define UART Handle Type Definition
#endif																	// END IF

// Assign as an argument per build configuration
// #define DEBUG_OUTPUT 0  // Define 1 to activate Debug communications output via UART, 0 to de-activate

#define LOGGER_ON    1  /*!< Allows activating logger    */
#define LOGGER_OFF   0  /*!< Allows deactivating logger  */
#define MAX_RX_SIZE  1  // zzqq temp placeholder for size of Rx Data

// Print statement active only when debug is enabled
#if DEBUG_OUTPUT
#define DEBUG_LOG(...) platformLog(__VA_ARGS__)
#else
#define DEBUG_LOG(...) do {} while (0)
#endif


/* ------------------------- Includes ------------------------- */
#include "platform.h"





/* ------------------------- Exported Variables ------------------------- */
extern uint8_t g_Rx_Data[MAX_RX_SIZE];		// placeholder Manufacturing Info Array, Total item count must be a multiple of 4
extern uint8_t g_bMsgReceived;				// boolean flag used to signal when a message has been transmitted to the unit





/* ------------------------- Exported Function Prototypes ------------------------- */
/****************************************************************************
* Function Name    : logUsartInit
* Date             : unknown
* Author           : ST-Micro
* Description      : This function initializes the UART handle and UART IP.
*
* Input Parameters : husart, handle to USART HW
*
*****************************************************************************/
extern void logUsartInit(UART_HandleTypeDef *husart);




/****************************************************************************
* Function Name    : logUsart
* Date             : unknown
* Author           : ST-Micro
* Description      : This function is used to write a formated string via
* 						the UART interface.
*
* Input Parameters : format, data to be transmitted
*
* Return		   : cnt, Number of data sent
*
*****************************************************************************/
extern int logUsart(const char* format, ...);





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
extern char* hex2Str(unsigned char * data, size_t dataLen);





/****************************************************************************
* Function Name    : init_UART_RX
* Date             : unknown
* Author           : ST-Micro
* Description      : Initializes UART Reception via Hardware Interrupts
*
* Input Parameters : none
*
* Return		   : none
*
*****************************************************************************/
extern void init_UART_RX(void);





/****************************************************************************
* Function Name    : HAL_UART_RxCpltCallback
* Date             : unknown
* Author           : ST-Micro
* Description      : This function is an Acknowledgment for UART Rx Completion
*
* Input Parameters : huart, UART handle.
*
* Return		   : none
*
*****************************************************************************/
extern void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);





#ifdef __cplusplus				// IF we are using C++
}								// DEFINE C++ Stuff
#endif							// END IF

#endif 							// END IF LOGGER_H





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

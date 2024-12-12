/**********************************************************************************************************************
* File Name      	 : demo.h
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
*   Modified by: Paul Fritzen & Ed Maskelony
* Description        : Header file for ST Demo Applications, re-coded to run tagFinder by Paul Fritzen
*
***********************************************************************************************************************

***********************************************************************************************************************
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
**********************************************************************************************************************/

/* ------------------------- DEFINES ------------------------- */
#ifndef DEMO_H					// IF DEMO_H is not defined
#define DEMO_H					// Define DEMO_H to prevent recursive inclusion

#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN           4     /*!< NFCV Block len                      */

#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */
#define DEMO_NFCV_WRITE_TAG           true //false /*!< NFCV demonstrate Write Single Block */

#define BLOCK_SIZE (4)              // canned CC byte count
#define PWD_SIZE (8)                // canned PWD byte count
#define CC_LENGTH (1)               // length of blocks reserved for CC File
#define CC_FILE_START (0)           // decimal address into EEPROM to see CC file located.
#define INFO_START_BLOCK (37)       // Address 144, decimal address into EEPROM to store Manufacturing Information
#define STAMP_BLOCK (55)            // Address 220, decimal address into EEPROM to store Devirginized Marker
#define RECIPE_START_BLOCK (56)     // Address 232, decimal address into EEPROM to store Recipe
#define TEST_FLAG_BLOCK (60)        // Address 240, decimal address into EEPROM to read/write Test Mode Flag
#define TEST_REPLY_BLOCK (60)       // Address 240, decimal address into EEPROM to write Test Pass Response
#define MSG_SIZE (100)              // size of NDEF Message
#define RX_DATA_SIZE (9)            // zzqq temp placeholder for size of Manufacturing Information
#define MAX_FAILS (5)               // maximum number of allowed failures
#define WRITE_FAIL (1)              // error code declaring excessive write failure
#define WRITE_PASS (0)              // code declaring write success

#define FACTORY_STAMP {'@','I','C','M'} // Marker/keyword that indicates whether the unit was already programmed

#ifdef __cplusplus				// IF we are using C++
extern "C" {					// DEFINE C++ Stuff
#endif							// END IF

/* ------------------------- Includes ------------------------- */
#include "platform.h"
#include "st_errno.h"

/* ------------------------- Exported Variables ------------------------- */
extern uint8_t g_DiscovState;	// state of NFC Discovery Machine

/* ------------------------- Exported Function Prototypes ------------------------- */
bool init_TagFinder( void );		// Prototype for function that initialized tagFinder
extern uint8_t tagFinder( void );	// Prototype for function that handles all NFC Tag finding & writing

#ifdef __cplusplus				// IF we are using C++
}								// DEFINE C++ Stuff
#endif							// END IF

#endif 							// END IF DEMO_H





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

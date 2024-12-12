/***********************************************************************************************************************
* Program Name       : UTF NFC Reader/Writer Firmware
* Creation Date      : 02/10/2023
* Version            : v1.20
* Product            : UTF NFC Reader/Writer Add On Board
* Code Name          : Attila
* Software Engineer  : Paul E. Fritzen & ST-Micro
* Hardware           : ST-Micro Nucleo-L053R8 Development Board w/ X-Nucleo-NFC06A1 Add On Board
*
* Description        : This Program controls Nucleo-L053R8 Development Board w/ X-Nucleo-NFC06A1 Add On Board.
*                       The program allows information to be sent from a device (such as a UTF) to the Nucleo Board in
*                       order to operate the NFC reader/writer board. The information can include commands to make the
*                       NFC writer configure the test mode of a UUT, write the factory configuration data to the UUT.
*                       or return the UUT's NFC Chip to it's default state.
*
* Thought for the Day: "You are our Moonlight
*                       Our Precious Moonlight
*                       You make us happy
*                       When skies are gray
*                       You'll always know dear
*                       How much we love you
*                       How could they take our
*                       Luna away?
*
*                       Rest In Peace Luna, the sweetest Kitty that ever lived. Life will never be the same without you.
*                       Our hearts may be broken and will never fully heal, but we will hold you in them forever still,
*                       waiting for the day we can see you again."
*
***********************************************************************************************************************/

/******************************************************************************
* File Name  :  main.c
* Author     :	ST-Micro & Paul Fritzen
* Description:  Main program body
*******************************************************************************
* Attention!
*
* Copyright (c) 2017 STMicroelectronics.
* All rights reserved.
*
* This software component is licensed by ST under Ultimate Liberty license
* SLA0094, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0094
*
******************************************************************************/

/* ------------------------- Includes ------------------------- */
#include "main.h"
#include "demo.h"
#include "platform.h"
#include "logger.h"
#include "st_errno.h"
#include "rfal_rf.h"
#include "rfal_analogConfig.h"





/* ------------------------- Private Variables ------------------------- */
// ADC_HandleTypeDef hadc;			// zzqq re-pin, used for ADC
uint8_t globalCommProtectCnt = 0;   /*!< Global Protection counter     */
UART_HandleTypeDef hlogger;         /*!< Handler to the UART HW logger */
uint8_t hariKari = 0;				// suicide switch in case of problem






/* ------------------------- Private Function Prototypes ------------------------- */
void SystemClock_Config(void);		// prototype to init system clock
//static void MX_ADC_Init(void); 	// zzqq re-pinning, prototype to init ADC




/* ---------------------------------  Functions  --------------------------------- */

/***********************************************************************************************************************
* Function Name      : main
* Creation Date      : 08/10/2022
* Software Engineer  : Paul E. Fritzen
* Description        : Main Function. Initializes all peripherals and calls tagFinder to operate NFC Reader/Writer
*
***********************************************************************************************************************/

// BEGIN main
int main(void)
{
    // Call Function to Initialize Hardware Abstraction Layer (HAL)
	HAL_Init();	/* STM32L0xx HAL library initialization:
       	   	   	   - Configure the Flash pre-fetch, Flash pre-read and Buffer caches
       	   	   	   - Systick timer is configured by default as source of time base, but user
             	 	 can eventually implement his proper time base source (a general purpose
             	 	 timer for example or other time source), keeping in mind that Time base
             	 	 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
             	 	 handled in milliseconds basis.
       	   	   	   - Low Level Initialization */

	// Call Function to Configure System Clock
	SystemClock_Config();		/* Configure the System clock to have a frequency of 80 MHz */

	// Call Function to initialize ADC
//  MX_ADC_Init(); // zzqq re-pin, Delete this if not using ADC for testing Super Cap

	// Call Function to initialize LEDs on NFC Board
	NFC06A1_LED_Init();

	// Call Function to initialize Blue Button
//	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);

// IF using I2C to communicate with the NFC Board
#if RFAL_USE_I2C

	// THEN call the function to initialize I2C
	BSP_I2C1_Init();

// ELSE
#else

	// Call Function to initialize SPI
	BSP_SPI1_Init();

//END IF RFAL_USE_I2C
#endif

	// Call Function to initialize log module
	logUsartInit(&hlogger);
  
    // Call Function to initialize UART RX
    init_UART_RX();

   // display boot up msg in debug mode
    DEBUG_LOG("NFC Reader/Writer for ICM using Nucleo-L053R8 & X-NUCLEO-NFC06A1\r\n");

  /* IF rfal initalization failed */
  if( (!(init_TagFinder())) )
  {
    DEBUG_LOG("Initialization failed..\r\n");

    // WHILE Forever
    while(1)
    {
      // Toggle all LEDs every 100 ticks
      NFC06A1_LED_Toggle( TX_LED );
      NFC06A1_LED_Toggle( TA_LED );
      NFC06A1_LED_Toggle( TB_LED );
      NFC06A1_LED_Toggle( TF_LED );
      NFC06A1_LED_Toggle( TV_LED );
      NFC06A1_LED_Toggle( AP2P_LED );
      platformDelay(100);
    }
    // END WHILE
  }

  // ELSE rfal initialization did not fail
  else
  {
    DEBUG_LOG("Initialization succeeded..\r\n");

	// Toggle each LED 6x
	for (int i = 0; i < 6; i++)
	{
	  NFC06A1_LED_Toggle( TX_LED );
	  NFC06A1_LED_Toggle( TA_LED );
	  NFC06A1_LED_Toggle( TB_LED );
	  NFC06A1_LED_Toggle( TF_LED );
	  NFC06A1_LED_Toggle( TV_LED );
	  NFC06A1_LED_Toggle( AP2P_LED );
	  platformDelay(200);
	}
	// END FOR

	// Turn all LEDs Off
	NFC06A1_LED_OFF( TA_LED );
	NFC06A1_LED_OFF( TB_LED );
	NFC06A1_LED_OFF( TF_LED );
	NFC06A1_LED_OFF( TV_LED );
	NFC06A1_LED_OFF( AP2P_LED );
	NFC06A1_LED_OFF( TX_LED );
  }

  /* WHILE Forever */
  while (1)
  {
	/* Run Tag Finder Application */
	hariKari = tagFinder();

	// IF we have a critical error
	if (hariKari != 0)
	{
        DEBUG_LOG("Critical tagFinder() error!!\r\n");
	}
  }
  //END WHILE
}
// END main





/***********************************************************************************************************************
* Function Name      : SystemClock_Config
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : System Clock Configuration
*         				The system Clock is configured as follow :
*            				System Clock source            = MSI
*            				SYSCLK(Hz)                     = 48000000
*            				HCLK(Hz)                       = 48000000
*            				AHB Prescaler                  = 1
*           				APB1 Prescaler                 = 1
*				            APB2 Prescaler                 = 1
*				            Flash Latency(WS)              = 2
*
***********************************************************************************************************************/

// BEGIN SystemClock_Config
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
  
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /* Enable Power Clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
}
// END SystemClock_Config





/***********************************************************************************************************************
* Function Name      : MX_ADC_Init
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : Initializes ADC
*
***********************************************************************************************************************/
/*
// zzqq BEGIN Re-pin
static void MX_ADC_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)

  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  // Configure for the selected ADC regular channel to be converted.

  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

} // zzqq END Re-Pin
*/





/***********************************************************************************************************************
* Function Name      : _Error_Handler
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : Blank Function for user to create own error handling
*
***********************************************************************************************************************/
// BEGIN _Error_Handler
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}
// END _Error_Handler




// IF Using Full Assert
#ifdef USE_FULL_ASSERT

/***********************************************************************************************************************
* Function Name      : assert_failed
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : Reports the name of the source file and the source line number
*         				where the assert_param error has occurred.
*
* Parameters:
* 	file: pointer to the source file name
*   line: assert_param error line source number
*
* Return:
* 	None
***********************************************************************************************************************/

// BEGIN assert_failed
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
// END assert_failed

#endif
// END IF Using Full Assert





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*********************************************************************************
* File Name :  demo_polling.c
* Author:      ST-Micro
*   Modified by: Paul Fritzen & Ed Maskelony
* Description: Looks for NFC-V tags and performs operations to test and then
* 				perform factory initialization of said tag. This is achieved by
* 				polling for NFC Tags and displaying what type of tag is discovered
* 				via an LED. If an NFC-V Tag is detected and a command has  been
* 				sent via UART Interrupt then said command will be executed.
* 				Commands include writing an initiate test flag into the target
* 				tags memory, checking for a reply flag in target tags memory, and
* 				running the full factory initialization using additional data sent
* 				along with that command and hard coded, non-changing data as well.
*
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
*********************************************************************************/





/* ------------------------- Includes ------------------------- */
#include "demo.h"
#include "utils.h"
#include "rfal_nfc.h"
#include "rfal_st25xv.h"
#include "logger.h"
#include "icm_models.h"

#if (defined(ST25R3916) || defined(ST25R95)) && RFAL_FEATURE_LISTEN_MODE
#include "demo_ce.h"
#endif





/* ------------------------- Private Variables ------------------------- */
uint8_t g_DiscovState = DEMO_ST_NOTINIT;	// state of NFC Discovery Machine
static rfalNfcDiscoverParam discParam;		// discovery parameter structure


#if RFAL_FEATURE_NFC_DEP
 P2P communication data
static uint8_t ndefLLCPSYMM[] = {0x00, 0x00};
static uint8_t ndefInit[] = {0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E, 0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65, 0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02};
static uint8_t ndefUriSTcom[] = {0x13, 0x20, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0x23, 0xc1, 0x01,
                                  0x00, 0x00, 0x00, 0x1c, 0x55, 0x00, 0x68, 0x74, 0x74, 0x70, 0x3a,
                                  0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x73, 0x74, 0x2e, 0x63, 0x6f, 0x6d,
                                  0x2f, 0x73, 0x74, 0x32, 0x35, 0x2D, 0x64, 0x65, 0x6D, 0x6F };
#endif /* RFAL_FEATURE_NFC_DEP */

#if (defined(ST25R3916) || defined(ST25R95)) && RFAL_FEATURE_LISTEN_MODE
/* NFC-A CE config */
/* 4-byte UIDs with first byte 0x08 would need random number for the subsequent 3 bytes.
 * 4-byte UIDs with first byte 0x*F are Fixed number, not unique, use for this demo
 * 7-byte UIDs need a manufacturer ID and need to assure uniqueness of the rest.*/
static uint8_t ceNFCA_NFCID[]     = {0x5F, 'S', 'T', 'M'};    /* =_STM, 5F 53 54 4D NFCID1 / UID (4 bytes) */
static uint8_t ceNFCA_SENS_RES[]  = {0x02, 0x00};             /* SENS_RES / ATQA for 4-byte UID            */
static uint8_t ceNFCA_SEL_RES     = 0x20;                     /* SEL_RES / SAK                             */
 #endif /* RFAL_FEATURE_LISTEN_MODE */
#if defined(ST25R3916) && RFAL_FEATURE_LISTEN_MODE
/* NFC-F CE config */
static uint8_t ceNFCF_nfcid2[]     = {0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static uint8_t ceNFCF_SC[]         = {0x12, 0xFC};
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                       /* SENSF_RES                                */
                                      0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             /* NFCID2                                   */
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                      0x00, 0x00 };                                               /* RD                                       */
#endif /* RFAL_FEATURE_LISTEN_MODE */

// Default Password
static uint8_t payLoad_DEF_PWD[PWD_SIZE] =
    {
        0x00, //
        0x00, //
        0x00, //
        0x00, //
        0x00, //
        0x00, //
        0x00, //
        0x00  //
};

// RF Config Password
static uint8_t payLoad_RF_CONFIG_PWD[PWD_SIZE] =
    {
        'p',
        'w',
        'd',
        '1',
        '2',
        '3',
        '4',
        '5'};

// Area 1 Config Password
static uint8_t payLoad_RF_AREA_1_PWD[PWD_SIZE] =
    {
        'p',
        'w',
        'd',
        '1',
        '2',
        '3',
        '4',
        '5'};

// Stores the data read back from the NFC chip so that it can be
// compared to the program sent over UART.
uint8_t written[PROGRAM_LEN];

/* ------------------------- Private Function Prototypes ------------------------- */
uint8_t tagFinder( void );
static uint8_t deInitializer( rfalNfcvListenDevice *nfcvDev );
static uint8_t writeConfiguration( rfalNfcvListenDevice *nfcvDev);
static uint8_t factoryInitializer( rfalNfcvListenDevice *nfcvDev);
static uint8_t processCommand( rfalNfcvListenDevice *nfcvDev );
static uint8_t initializeTest( rfalNfcvListenDevice * nfcvDev );
static uint8_t checkReply( rfalNfcvListenDevice * nfcvDev );
/*static void demoP2P( rfalNfcDevice *nfcDev );
static void demoAPDU( void );
static void demoNfcv( rfalNfcvListenDevice *nfcvDev );
static void demoNfcf( rfalNfcfListenDevice *nfcfDev );
static void demoCE( rfalNfcDevice *nfcDev );*/
static void demoNotif( rfalNfcState st );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );





/* ---------------------------------  Functions  --------------------------------- */

/***********************************************************************************************************************
* Function Name      : Demo Notification
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : This function receives the event notifications from RFAL
*
***********************************************************************************************************************/

// BEGIN demoNotif()
static void demoNotif( rfalNfcState st )
{
    uint8_t       devCnt;
    rfalNfcDevice *dev;


    if( st == RFAL_NFC_STATE_WAKEUP_MODE )
    {
        platformLog("Wake Up mode started \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_TECHDETECT )
    {
        platformLog("Wake Up mode terminated. Polling for devices \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_SELECT )
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound( &dev, &devCnt );
        rfalNfcSelect( 0 );

        platformLog("Multiple Tags detected: %d \r\n", devCnt);
    }
}
// END demoNotif()




/***********************************************************************************************************************
* Function Name      : init_TagFinder
* Creation Date      : Unknown
* Software Engineer  : ST-Micro
* Description        : This function Initializes the required layers for the demo
*
* return: TRUE if initialization success or FALSE if initialization failure
*
***********************************************************************************************************************/

// BEGIN init_TagFinder()
bool init_TagFinder( void )
{
    ReturnCode err;

    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        discParam.compMode      = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit      = 1U;
        discParam.nfcfBR        = RFAL_BR_212;
        discParam.ap2pBR        = RFAL_BR_424;
        discParam.maxBR         = RFAL_BR_KEEP;
/*	This parameter is unused for NFC-V tags
        ST_MEMCPY( &discParam.nfcid3, NFCID3, sizeof(NFCID3) );
        ST_MEMCPY( &discParam.GB, GB, sizeof(GB) );
        discParam.GBLen         = sizeof(GB);
*/
        discParam.notifyCb             = demoNotif;
        discParam.wakeupEnabled        = false;
        discParam.wakeupConfigDefault  = true;
        discParam.totalDuration        = 1000U;
        discParam.techs2Find           = 0;
#if RFAL_FEATURE_NFCA
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_A;
#endif
#if RFAL_FEATURE_NFCB
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_B;
#endif
#if RFAL_FEATURE_NFCF
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_F;
#endif
#if RFAL_FEATURE_NFCV
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_V;
#endif
#if RFAL_FEATURE_ST25TB
        discParam.techs2Find          |= RFAL_NFC_POLL_TECH_ST25TB;
#endif
#if DEMO_CARD_EMULATION_ONLY
        discParam.totalDuration        = 60 * 1000U; /* 60 seconds */
        discParam.techs2Find           = 0;
#endif

#if defined(ST25R3911) || defined(ST25R3916)  /* AP2P */
#if RFAL_FEATURE_NFC_DEP
        discParam.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#if RFAL_FEATURE_LISTEN_MODE
        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_FEATURE_LISTEN_MODE */
#endif /* RFAL_FEATURE_NFC_DEP */
#endif /* ST25R3911 || ST25R3916 */

#if RFAL_FEATURE_LISTEN_MODE
#if defined(ST25R3916) || defined(ST25R95)    /* CE */
        /* Set configuration for NFC-A CE */
        ST_MEMCPY( discParam.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN );     /* Set SENS_RES / ATQA */
        ST_MEMCPY( discParam.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_LM_NFCID_LEN_04 );           /* Set NFCID / UID */
        discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;                                  /* Set NFCID length to 7 bytes */
        discParam.lmConfigPA.SEL_RES  = ceNFCA_SEL_RES;                                        /* Set SEL_RES / SAK */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* ST25R3916 || ST25R95*/
#if defined(ST25R3916)
        /* Set configuration for NFC-F CE */
        ST_MEMCPY( discParam.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN );                 /* Set System Code */
        ST_MEMCPY( &ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN], ceNFCF_nfcid2, RFAL_NFCID2_LEN );     /* Load NFCID2 on SENSF_RES */
        ST_MEMCPY( discParam.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN );  /* Set SENSF_RES / Poll Response */

        discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* ST25R3916 */
#endif /* RFAL_FEATURE_LISTEN_MODE */

        /* Check for valid configuration by calling Discover once */
        err = rfalNfcDiscover( &discParam );
        rfalNfcDeactivate( false );

        if( err != ERR_NONE )
        {
            return false;
        }

        g_DiscovState = DEMO_ST_START_DISCOVERY;
        return true;
    }
    return false;
}
//END init_TagFinder()




/***********************************************************************************************************************
* Function Name      : tagFinder
* Creation Date      : 8/11/2022
* Software Engineer  : ST-Micro & Paul Fritzen
* Description        : This function executes the Tag Finder state machine. It must be called periodically to detect
* 						any NFC Tags inside the RF Field & differentiate between them. If an NFC-V Tag is detected and
* 						a command requesting writing has been successfully received via UART then the processCommand()
* 						function will be called to enable RF Writing to the target device.
*
*  Input Parameters  : nfcvDev, a pointer to an NFC Device Structure
*  Return            : error, signifies an error occurred if not 0 or no error
*  					    occurred if 0.
*
***********************************************************************************************************************/

//BEGIN tagFinder()
uint8_t tagFinder( void )
{
    //constants

    //variables
    static rfalNfcDevice *nfcDevice;	// NFC device detected within the Add On Boards RF field
    static uint8_t       writeArmed;	// boolean variable used to gate write actions in conjunction w/ a push button
    uint8_t              error = 0;     // signifies if an RF write error occurred or not. 0 is no error, assume success

/************************************************ BEGIN FUNCTION ************************************************/

    // IF the flag signifying a message has been received is set to true
    if (g_bMsgReceived == 1)
    {
    	// reset flag to false
    	g_bMsgReceived = 0;

        // restart discovery state loop
    	g_DiscovState = DEMO_ST_START_DISCOVERY;

        // Write Rx Data to Console
        DEBUG_LOG("Data: %s\r\n", hex2Str(g_Rx_Data, sizeof(g_Rx_Data)));

       	// Set Write Tag Flag to true
       	writeArmed = 1;
    }
    // END IF

    /* Run RFAL worker periodically */
    rfalNfcWorker();

    //BEGIN SWITCH g_DiscovState
    switch( g_DiscovState )
    {
        /*******************************************************************************/

        // CASE in Start of Discovery
        case DEMO_ST_START_DISCOVERY:

        	// Turn off all LEDs
			platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
			platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
			platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
			platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
			platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
			platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);

			// Call function to deactivate NFC
			rfalNfcDeactivate( false );

			// call function to start NFC Discovery
			rfalNfcDiscover( &discParam );

            // change state to discovery mode
			g_DiscovState = DEMO_ST_DISCOVERY;

            // BREAK
            break;

        /*******************************************************************************/
        // CASE in Discovery
        case DEMO_ST_DISCOVERY:

			// IF an NFC Tag is in the RF Field
			if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
			{
				// call function to get active device and assign to device structure
				rfalNfcGetActiveDevice( &nfcDevice );

					// BEGIN SWITCH nfc type
					switch( nfcDevice->type )
					{
						/*******************************************************************************/

						// CASE NFC-V
						case RFAL_NFC_LISTEN_TYPE_NFCV: ;
#if DEBUG_OUTPUT
								// assign device Unique ID
								uint8_t devUID[RFAL_NFCV_UID_LEN]; //8U
								static uint8_t prevUID[RFAL_NFCV_UID_LEN]; //8U

								// Copy the UID into local variable
								ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );

								// is the detected tag different from last poll?
								if (0 != memcmp(devUID, prevUID, nfcDevice->nfcidLen)){
								    // save detected UID
								    ST_MEMCPY( prevUID, nfcDevice->nfcid, nfcDevice->nfcidLen );

	                                // Reverse the UID for display purposes
	                                REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );
	                                // Write UID to Console
	                                platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));

								}
#endif
								// Light the LED for NFC-V
								platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);

								// IF Write Flag is true
								if (writeArmed == 1)
								{
									// disarm the Write Flag
									writeArmed = 0;

									// call factory initializer to write to part
									error = processCommand( &nfcDevice->dev.nfcv );
								}
								// END IF

						break;
						// END CASE NFC-V

						/*******************************************************************************/

						// CASE NFC-A
						case RFAL_NFC_LISTEN_TYPE_NFCA:

							// Light the LED for NFC-A
							platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);

						break;
						// END CASE NFC-A

						/*******************************************************************************/

						// CASE NFC-B or CASE NFC-ST25TB
						case RFAL_NFC_LISTEN_TYPE_NFCB:
						case RFAL_NFC_LISTEN_TYPE_ST25TB:

							// Light the LED for NFC-B
							platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);

						break;
						// END CASE NFC-B or CASE NFC-ST25TB

						/*******************************************************************************/

						// CASE NFC-F
						case RFAL_NFC_LISTEN_TYPE_NFCF:

							// Light the LED for NFC-F
							platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);

						break;
						// END CASE NFC-F

						/*******************************************************************************/

						// CASE NFC-AP2P Listen or CASE NFC-AP2P Poll
						case RFAL_NFC_LISTEN_TYPE_AP2P:
						case RFAL_NFC_POLL_TYPE_AP2P:

							// Light the LED for NFC-AP2P
							platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);

						break;
						// END CASE NFC-AP2P Listen or Poll

						/*******************************************************************************/

						// CASE NFC-A Poll or CASE NFC-F Poll
						case RFAL_NFC_POLL_TYPE_NFCA:
						case RFAL_NFC_POLL_TYPE_NFCF:

							// Light LED for Type A or Type F
							platformLedOn( ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PORT : PLATFORM_LED_F_PORT),
										   ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PIN  : PLATFORM_LED_F_PIN)  );
						break;
						// END CASE NFC-A Poll or CASE NFC-F Poll

						/*******************************************************************************/

						// DEFAULT
						default:

						break;
						// END DEFAULT
			}
			//END SWITCH nfc type

			// Call function to deactivate NFC
			rfalNfcDeactivate( false );

			/* If Not In a Card Emulation Mode delay 500ms. In card emulation mode some polling devices (phones) rely on tags to be re-discoverable */
			#if !defined(DEMO_NO_DELAY_IN_DEMOCYCLE)

				// BEGIN SWITCH nfc device type
				switch( nfcDevice->type )
				{
					// CASE Type A or Type F
					case RFAL_NFC_POLL_TYPE_NFCA:
					case RFAL_NFC_POLL_TYPE_NFCF:

						// BREAK
						break;

					// DEFAULT
					default:
						/* Delay before re-starting polling loop to not flood the UART log with re-discovered tags */
						platformDelay(500);
				}
				// END SWITCH nfc device type

			#endif /* DEMO_NO_DELAY_IN_DEMOCYCLE */

            // set state to Start Discovery
			g_DiscovState = DEMO_ST_START_DISCOVERY;
        }
        //END IF an NFC Tag is in the RF Field

        // BREAK
        break;

		/*******************************************************************************/

		// CASE Not Initialized or Default
		case DEMO_ST_NOTINIT:
		default:

			// BREAK
			break;
	}
	//END SWITCH state

	// Return error
	return error;
}
// END tagFinder Function





/*********************************************************************************
* Function Name    : processCommand
* Date             : 02/10/2023
* Author           : Paul E. Fritzen
* Description      : This function processes and executes commands sent into a
* 					  global buffer in order to operate the NFC Writer.
*
*  Input Parameters: nfcvDev, a pointer to an NFC Device Structure
*  Return          : error, signifies an error occurred if not 0 or no error
*  					  occurred if 0
*********************************************************************************/

// BEGIN processCommand()
static uint8_t processCommand( rfalNfcvListenDevice *nfcvDev )
{


    // variables
	uint8_t error       = 0;	// container for error codes, 0 for no error



    /**************************************** BEGIN FUNCTION ****************************************/

    //BEGIN SWITCH utf-command
    switch (command)
    {
    	/*******************************************************************************/
        case NONE:
            // Write acknowledgment to Console that we are doing nothing
            DEBUG_LOG("No Command Detected!!! ");

        break;
        // END CASE None

        /*******************************************************************************/
        case QUERY_CONFIG:
            // Reply over serial with the model number that is currently installed
            platformLog(ICM_MODEL_STR "\n");

        break;
        // END CASE QUERY_CONFIG

        /*******************************************************************************/
        // CASE Program
        case PROGRAM: ;
        	uint8_t checksum = 0;

            for (int i = 0; i < PROGRAM_LEN; i++)
            {
                checksum += program[i];
            }

            if (checksum != 0)
            {
                platformLog("CHECKSUM_ERR\n");
            }
            else
            {
                error = writeConfiguration(nfcvDev);

                // Delay to allow UTF to Catch Up
                platformDelay(1000);

                if (error == 0)
                {
                    // No errors, transmit Pass status to UTF
                    platformLog("PASS\n");
                }

                else
                {
                    // Error detected, transmit Fail status to UTF
                    platformLog("FAIL\n");
                }
            }
            break;
        // END CASE Initialize Test

        /*******************************************************************************/
        // DEFAULT CASE
        default:
            platformLog("FAIL, invalid serial command\n");
        break;
        // END DEFAULT CASE
    }
    //END SWITCH utf-Command

    // clear command code and Rx buffer
    command = NONE;
    ST_MEMSET(g_Rx_Data, 0, MAX_RX_SIZE);

    // Return any error codes
    return error;
}
// END processCommand()








/****************************************************************************
* Function Name    : initializeTest
* Date             : 02/10/2023
* Author           : Paul E. Fritzen
* Description      : Checks for Virgin State at Block 55 and re-virginizes
* 						NFC Chip of not virgin. Writes factory initialization
* 						flag into Block 60.
*
*  Input Parameters: nfcvDev, a pointer to an NFC Device Structure
*  Return          : error, signifies an error occurred if not 0 or no error
*  					  occurred if 0
*****************************************************************************/

// BEGIN initializeTest function
static uint8_t initializeTest( rfalNfcvListenDevice * nfcvDev )
{
    //constants

    //variables
    ReturnCode error;                  // return code for errors. IF 0 then there are no errors
    uint8_t    blockNum;               // keeps track of the block number to be written to during initialization
    uint8_t    *uid;                   // Unique Identifier of an NFC tag found within the antenna's field
    uint8_t    reqFlag;                // Request Flag, used to tell Middleware Functions we are working with an NFC-V Tag
    uint8_t    blockLength;            // length of a block of data. For the st25dv04k this will always be 4 bytes.
    uint8_t    byteCounter     = 0;    // counter used to track number of bytes loaded into write/read array
    uint8_t    matchCounter    = 0;    // counter used to track number of matching bytes read in de-virginized marker
    uint8_t    failureCounter  = 0;    // counter used to track number of times write/read has failed
    uint8_t    transmitSuccess = 0;    // sentinel boolean used to gate guard against transmission failure
    uint16_t   rcvLen;				   // placeholder variable for receiving Acknowledgment from lower level functions

    uint8_t    rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];		/* Flags + Block Data + CRC */

    // placeholder Test Flag File
    static uint8_t testFlag[BLOCK_SIZE] =
    {
        'T', 'E', 'S', 'T'
    };

    // De-virginized Marker
    static uint8_t factoryStamp[BLOCK_SIZE] = FACTORY_STAMP;

/**************************************** BEGIN FUNCTION ****************************************/

    // set first block number to read from Block 55 to look for the DV Mark
    blockNum = STAMP_BLOCK;

    // set UID
    uid = nfcvDev->InvRes.UID;

    // set flag showing NFC Standard in use is NFC-V/ISO 15693
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;

    // set block Length
    blockLength = BLOCK_SIZE;

    /********** BEGIN Check If Initialized **********/

    // DO
    do
    {
        // read Test Flag Block
        error = rfalNfcvPollerReadSingleBlock(reqFlag, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);

        // log output
        DEBUG_LOG(" Read Marker: %s %s\r\n", (error != ERR_NONE) ? "FAIL": "OK Data:", (error != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        //END IF
    }
    // WHILE transmit success sentinel != true AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // FOR each value in the De-Virg Marker
    for (byteCounter = 0; byteCounter < sizeof(factoryStamp); byteCounter++)
    {
        // IF the read value does not match the expected value
        if (factoryStamp[byteCounter] != rxBuf[(byteCounter + 1)])
        {
            // THEN we are not factory initialized, do not run de-initializer and end loop
            break;
        }
        // ELSE we have a match
        else
        {
        	// increment match counter
            matchCounter++;

        	// IF all values match
            if (matchCounter == sizeof(factoryStamp))
        	{
                // THEN chip is previously initialized, call de-init function to erase it
                deInitializer(nfcvDev);
            }
        }
    }
    // END FOR

    /********** END Check If Initialized **********/






    /********** BEGIN Test Initialization **********/

    // set first block number to write to Block 60 for the Test Flag
    blockNum = TEST_FLAG_BLOCK;

    // DO write the test flag into the NFC Chip
    do
    {
        // write Test Flag
        error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, blockNum, testFlag, blockLength);

        // log output
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( testFlag, BLOCK_SIZE) );


        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    /********** END Test Initialization **********/

    // otherwise all is well, return Write Pass
    return WRITE_PASS;
}
// END initializeTest function




/****************************************************************************
* Function Name    : checkReply
* Date             : 08/11/2022
* Author           : Paul E. Fritzen
* Description      : Checks modified factory initialization flag at Block zzqq
*
*  Input Parameters: nfcvDev, a pointer to an NFC Device Structure
*  Return          : error, signifies an error occurred if not 0 or no error
*  					  occurred if 0
*
*****************************************************************************/

// BEGIN checkReply function
static uint8_t checkReply( rfalNfcvListenDevice * nfcvDev )
{
    //constants

    //variables
    ReturnCode error;                   // return code for errors. IF 0 then there are no errors
    uint8_t    blockNum;                // keeps track of the block number to be written to during initialization
    uint8_t    *uid;                    // Unique Identifier of an NFC tag found within the antenna's field
    uint8_t    reqFlag;                 // Request Flag, used to tell Middleware Functions we are working with an NFC-V Tag
    uint16_t   rcvLen;					// placeholder variable for receiving Acknowledgment from lower level functions
    uint8_t    byteCounter     = 0;     // counter used to track number of bytes loaded into write/read array
    uint8_t    failureCounter  = 0;     // counter used to track number of times write/read has failed
    uint8_t    transmitSuccess = 0;		// sentinel boolean used to gate guard against transmission failure

    // placeholder Test Flag File
    static uint8_t testReply[BLOCK_SIZE] =
    {
            'P'
          , 'A'
          , 'S'
          , 'S'
    };

    uint8_t    rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];		/* Flags + Block Data + CRC */

/**************************************** BEGIN FUNCTION ****************************************/

    // set first block number to write to Block 60 for the Test Flag
    blockNum = TEST_REPLY_BLOCK;

    // set UID
    uid = nfcvDev->InvRes.UID;

    // set flag showing NFC Standard in use is NFC-V/ISO 15693
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;

    // DO
    do
    {
        // read Test Flag Block
        error = rfalNfcvPollerReadSingleBlock(reqFlag, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);

        DEBUG_LOG(" Read Block: %s %s\r\n", (error != ERR_NONE) ? "FAIL": "OK Data:", (error != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        //END IF
    }
    // WHILE transmit success sentinel != true AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // FOR each value in the Test Flag Block
    for (byteCounter = 0; byteCounter < BLOCK_SIZE; byteCounter++)
    {
        // IF the read value does not match the expected value
        if ( testReply[byteCounter] != rxBuf[(byteCounter + 1)] )
        {
            // THEN return Write Fail
            return WRITE_FAIL;
        }
        // END IF
    }
    // END FOR

    // Otherwise all is well, Return Write Pass
    return WRITE_PASS;
}
// END checkReply function


// Write Configuration
// Write the configuration file received over UART to the NFC.
// Read it back and store it in 
static uint8_t writeConfiguration( rfalNfcvListenDevice *nfcvDev )
{
    const uint8_t RF_PWD_0 = 0x00;
    const uint8_t RF_PWD_1 = 0x01;
    const uint8_t RFA1SS   = 0x04;
    const uint8_t WRT_LOCK = 0x05;

    ReturnCode  error;
    uint8_t     nextBlock;
    uint8_t     *uid;
    uint8_t     reqFlag;
    uint8_t     blockLength;
    uint8_t     blockToWrite[BLOCK_SIZE];
    uint8_t     blockRead[BLOCK_SIZE];
    uint8_t     byteCounter = 0;
    uint8_t     failureCounter = 0;
    uint8_t     multiBlockIndex = 0;
    uint8_t     singleBlockIndex = 0;
    uint16_t    rcvLen;

    nextBlock = RECIPE_START_BLOCK + 1;
    uid = nfcvDev->InvRes.UID;
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;
    blockLength = BLOCK_SIZE;
    nextBlock = RECIPE_START_BLOCK + 1;

    for (multiBlockIndex = 0; multiBlockIndex < (sizeof(program)); multiBlockIndex++)
    {
        blockToWrite[singleBlockIndex] = program[multiBlockIndex];
        byteCounter++;

        if (byteCounter == blockLength)
        {
            do
            {
                error = rfalST25xVPollerPresentPassword(reqFlag, uid, RF_PWD_1, payLoad_RF_AREA_1_PWD, sizeof(payLoad_RF_AREA_1_PWD));
                DEBUG_LOG("Present Password: %s\r\n", (error != ERR_NONE) ? "FAIL" : "OK");

                if (error == 0)
                {
                    error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, blockToWrite, blockLength);
                    DEBUG_LOG("Write Block %d: %s\r\n", nextBlock, (error != ERR_NONE) ? "FAIL" : "OK");
                }

                if (error == 0)
                {
                    failureCounter = 0;
                }
                else if (failureCounter <= MAX_FAILS)
                {
                    failureCounter++;
                    platformDelay(1000);
                }
                else
                {
                    return WRITE_FAIL;
                }
            }
            while (error != 0 && failureCounter <= MAX_FAILS);

            byteCounter = 0;
            failureCounter = 0;
            nextBlock++;
        }

        singleBlockIndex++;
        if (singleBlockIndex == blockLength)
        {
            singleBlockIndex = 0;
        }
    }

    nextBlock = RECIPE_START_BLOCK + 1;
    for (multiBlockIndex = 0; multiBlockIndex < (sizeof(program)); multiBlockIndex += blockLength)
    {
        error = rfalNfcvPollerReadSingleBlock(reqFlag, uid, nextBlock, blockRead, sizeof(blockRead), &rcvLen);
        if (error != 0 || rcvLen != blockLength || memcmp(&program[multiBlockIndex], blockRead, blockLength) != 0)
        {
            DEBUG_LOG("Verification Failed Block %d\r\n", nextBlock);
            return WRITE_FAIL;
        }
        DEBUG_LOG("Verification Success Block %d\r\n", nextBlock);
        nextBlock++;
    }

    return WRITE_PASS;
}


/****************************************************************************
* Function Name    : Factory Initializer
* Date             : 02/10/2023
* Author           : Paul E. Fritzen
* Description      : Reads & Writes factory initialization with verification
* 					  output to console. Writes CC File, NDEF, Factory
* 					  Configuration, De-Virginized Marker and configures
* 					  Read/Write Protection, sets and assigns Password for
* 					  User Memory Area 1, as well as sets configuration
* 					  password.
*
*  Input Parameters: nfcvDev, a pointer to an NFC Device Structure
*  Return          : error, signifies an error occurred if not 0 or no error
*  					  occurred if 0
*
*****************************************************************************/

// BEGIN factoryInitializer function
static uint8_t factoryInitializer( rfalNfcvListenDevice *nfcvDev )
{
    //constants
    const uint8_t RF_PWD_0 = 0x00;      	// password number designation of RF Configuration Password
    const uint8_t RF_PWD_1 = 0x01;      	// password number designation of RF Area 1 Password
    const uint8_t RFA1SS   = 0x04;        	// address register of RF Area 1 Password
    const uint8_t WRT_LOCK = 0x05;      	// bit weight to enable password protected Writes for a user memory area

    //variables
    ReturnCode  error;                       // return code for errors. IF 0 then there are no errors
    uint8_t     nextBlock;                   // keeps track of the block number to be written to during initialization
    uint8_t     *uid;                        // Unique Identifier of an NFC tag found within the antenna's field
    uint8_t     reqFlag;                     // Request Flag, used to tell Middleware Functions we are working with an NFC-V Tag
    uint8_t     blockLength;                 // length of a block of data. For the st25dv04k this will always be 4 bytes.
    uint8_t     blockToWrite[BLOCK_SIZE];    // array used to write/read one block
    uint8_t     byteCounter      = 0;        // counter used to track number of bytes loaded into write/read array
    uint8_t     failureCounter   = 0;        // counter used to track number of times write/read has failed
    uint8_t     transmitSuccess  = 0;        // sentinel boolean used to gate guard against transmission failure
    uint8_t     multiBlockIndex  = 0;        // used to keep track of index of entire full size byte array to be written in a FOR LOOP
    uint8_t     singleBlockIndex = 0;        // used to keep track of index of 4 bytes of the entire byte array to be written in a FOR LOOP

    // Capability Container (CC) File
    static uint8_t payLoad_GoodCC[BLOCK_SIZE] =
    {
		0xE1,   // Magic Number, 0xE1 means we are using a 4 byte CC File instead of an 8 Byte CC File (0xE2)
		0x40,   // Version & Access, 0x40 gives full access
		0x40,   // size of T5T_AREA in octbytes (8 * this value is the # of bytes of the Type 5 Tag Area. 0x40 is the full chip)
		0x00	// Additional features, none enabled
    };

    // placeholder NDEF message. Total item count must be a multiple of 4
    static uint8_t payLoad_NdefMsg[] =
    {
        0x03,	// Tag Field, 0x03 represents an NDEF Message
		0x38,		// Length Field, represents length of the Value Field (rest of the data besides TLV Terminator) in bytes

        /* Begin Record 1 */

		0x91,	// Record 1 Header Byte (0b1001_0001):
				//	MessageBegin=1, MessageEnd=0
				//	ChunkFlag=0
				//	ShortRecord=1 (payload length field is only 1 byte, not 4)
				//	ID_Length=0 (header does not have ID Length field)
				//	TypeNameFormat=0o1 (NFC Forum Well Known Type)
		0x01,	// Record 1 Type Length
		0x0C,   // Record 1 Payload Length
		'U',	// Record 1 Type 'U' (Type U is a URI Type)

		// Begin Record 1 Payload
		0x04,   // Byte that represents "https://" in URI record as per NFC Forum T5T Specification
		'i',
		'c',
		'm',
		'o',
		'm',
		'n',
		'i',
		'.',
		'c',
		'o',
		'm',
		// End Record 1 Payload

		/* End Record 1 */



		/* Begin Record 2 */

		0x5C,   // Record 2 Header Byte (0b0101_1100):
				//	MessageBegin=0, MessageEnd=1
				//	ChunkFlag=0
				//	ShortRecord=1 (payload length field is only 1 byte, not 4)
				//	ID_Length=1 (header contains ID Length field)
				//	TypeNameFormat=0o4 (NFC Forum External Type)
		0x0F,   // Record 2 Type Length
		0x15,	// Record 2 Payload Length
		0x00,	// Record 2 ID Length

		// Begin Record 2 Type
		'a',
		'n',
		'd',
		'r',
		'o',
		'i',
		'd',
		'.',
		'c',
		'o',
		'm',
		':',
		'p',
		'k',
		'g',
		// End Record 2 Type

		// Begin Record 2 Payload
		'c',
		'o',
		'm',
		'.',
		'i',
		'c',
		'm',
		'c',
		'o',
		'n',
		't',
		'r',
		'o',
		'l',
		's',
		'.',
		'n',
		'f',
		'c',
		'.',
		'u',

		// End Record 2 Payload

		/* End Record 2 */



		/* Begin TLV Terminator */

		0xFE,	// TLV Terminator, this represents the last byte of the last TLV and must be used if the entire T5T_Area is not being used
		0xFF,	// Filler used to write into unused register because writing must be done in 4 byte quantities.

		/* End TLV Terminator */
        };




    // De-virginized Marker
    static uint8_t payLoad_Factory_Stamp[BLOCK_SIZE] = FACTORY_STAMP;

/**************************************** BEGIN FUNCTION ****************************************/

    // set first block number to write to Block 0 for the CC File
    nextBlock = CC_FILE_START;

    // set UID
    uid = nfcvDev->InvRes.UID;

    // set flag showing NFC Standard in use is NFC-V/ISO 15693
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;

    // set block Length
    blockLength = BLOCK_SIZE;

    /********** BEGIN Write CC File **********/

    // DO
    do
    {
        // write good CC File
        error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, payLoad_GoodCC, blockLength);

        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_GoodCC, BLOCK_SIZE) );

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;

            // Transmit Success
            DEBUG_LOG("CC File Write Success\r\n");

        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        //END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    /********** END Write CC File **********/





    /********** BEGIN Write NDEF **********/

    // increment nextBlock by length reserved for CC File to start of NDEF address block (Block , Address )
    nextBlock = nextBlock + CC_LENGTH;

    // FOR each item in the array
    for (multiBlockIndex = 0; multiBlockIndex < (sizeof(payLoad_NdefMsg)); multiBlockIndex++)
    {
        // assign current item to tx array
        blockToWrite[singleBlockIndex] = payLoad_NdefMsg[multiBlockIndex];

        // Increment byte counter
        byteCounter++;

        // IF byte counter == 4 THEN
        if (byteCounter == blockLength)
        {
            // DO
            do
            {
                // call single block write function sending tx array as parameter
                error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, blockToWrite, blockLength);

                // write results to console
                DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(blockToWrite, blockLength));

                // IF there was no error writing
                if (error == 0)
                {
                    // set Transmit Success Sentinel to TRUE
                    transmitSuccess = 1;

                    // reset failure counter to 0
                    failureCounter = 0;

                    // Transmit Success
                    DEBUG_LOG("NDEF Write Success\r\n");
                }

                // ELSE IF there was an error reading and there have not been 10 consecutive errors
                else if (failureCounter <= MAX_FAILS)
                {
                    // increment failure counter
                    failureCounter++;

                    // Delay for 1 second in case of noise
                    platformDelay(1000);
                }

                // ELSE there have been too many failures
                else
                {
                    // return Write Failure, abort rest of function early
                    return WRITE_FAIL;
                }
                // END IF

            }
            // WHILE transmit success sentinel == false AND Failure Counter < 10
            while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

            // reset byte counter, failure counter, & Transmit Success Sentinel
            byteCounter = 0;
            failureCounter = 0;
            transmitSuccess = 0;

            // increment nextBlock by 1 to write next block
            nextBlock++;
        }
        // END IF

        // increment singleBlockIndex
        singleBlockIndex++;

        // IF we have loaded 4 blocks
        if (singleBlockIndex == blockLength)
        {
            // reset singleBlockIndex to 0
            singleBlockIndex = 0;
        }
        // END IF
    }
    // END FOR

    // reset multiBlockIndex and singleBlockIndex to zero
    multiBlockIndex = 0;
    singleBlockIndex = 0;

    /********** END Write NDEF **********/





    /********** BEGIN Write Product ID **********/

    // set nextBlock to start of Recipe address block (Block 56, Address 224)
    nextBlock = RECIPE_START_BLOCK;

    // DO
     do
     {
        // write Product ID
        error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, payLoad_ID_INFO, sizeof(payLoad_ID_INFO));

        // log output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_ID_INFO, BLOCK_SIZE) );

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;

            // Transmit Success
            DEBUG_LOG("Dogtag Write Success\r\n");
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    /********** END Write Product ID **********/





    /********** BEGIN Write Factory Configuration **********/

    // increment nextBlock by one block
    nextBlock++;

    // FOR each byte in the Factory Configuration array
    for (multiBlockIndex = 0; multiBlockIndex < (sizeof(payLoad_FactoryConfig)); multiBlockIndex++)
    {
        // assign current item to tx array
        blockToWrite[singleBlockIndex] = payLoad_FactoryConfig[multiBlockIndex];

        // Increment byte counter
        byteCounter++;

        // IF byte counter == block length (4) THEN
        if (byteCounter == blockLength)
        {
            // DO
            do
            {
                // call single block write function sending tx array as parameter
                error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, blockToWrite, blockLength);

                // write results to console
                DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(blockToWrite, blockLength));

                // IF there was no error writing
                if (error == 0)
                {
                        // set Transmit Success Sentinel to TRUE
                        transmitSuccess = 1;
                }

                // ELSE IF there was an error reading and there have not been 10 consecutive errors
                else if (failureCounter < MAX_FAILS)
                {
                    // increment failure counter
                    failureCounter++;

                    // Delay to allow UTF to Catch Up
                    platformDelay(1000);
                }

                // ELSE there have been too many failures
                else
                {
                    // return Write Failure, abort rest of function early
                    return WRITE_FAIL;
                }
                //END IF

            }
            // WHILE transmit success sentinel == false AND Failure Counter < 10
            while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

            // reset byte counter
            byteCounter = 0;

            // increment nextBlock by 1 to write next block
            nextBlock++;
        }
        // END IF

        // increment singleBlockIndex
        singleBlockIndex++;

        // IF we have loaded 4 blocks
        if (singleBlockIndex == blockLength)
        {
            //reset singleBlockIndex to 0
            singleBlockIndex = 0;
        }
        // END IF
    }
    // END FOR

    // reset multiBlockIndex and singleBlockIndex to zero
    multiBlockIndex = 0;
    singleBlockIndex = 0;
    /********** END Write Factory Configuration **********/





    /********** BEGIN Read/Write Permission Configuration **********/

    // DO present RF Configuration password
     do
     {
        // present RF Configuration password to open RF Security Session
        error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_0, payLoad_DEF_PWD, sizeof(payLoad_DEF_PWD) );

        // write results to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_DEF_PWD, PWD_SIZE) );

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay to allow UTF to Catch Up
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // DO change write protections for area 1
     do
     {

   	    // change write protection for area 1 so Write is not allowed without presenting a valid password for Area 1
   	    error = rfalST25xVPollerWriteConfiguration( reqFlag, uid, RFA1SS, WRT_LOCK );

        // write results to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", WRT_LOCK );

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // DO
     do
     {

        // present RF Configuration password to open RF Security Session
        error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_1, payLoad_DEF_PWD, sizeof(payLoad_DEF_PWD) );

        // write output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_DEF_PWD, PWD_SIZE) );

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

	    // Delay to allow UTF to Catch Up
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // DO change password for Memory Area 1
    do
    {
    	// change password for Memory Area 1
    	error = rfalST25xVPollerWritePassword(reqFlag, uid, RF_PWD_1, payLoad_RF_AREA_1_PWD, sizeof(payLoad_RF_AREA_1_PWD));

    	// log output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(payLoad_RF_AREA_1_PWD, PWD_SIZE));

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

	        // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // DO present RF Configuration password
    do
    {
    	// present RF Configuration password to open RF Security Session
    	error = rfalST25xVPollerPresentPassword(reqFlag, uid, RF_PWD_0, payLoad_DEF_PWD, sizeof(payLoad_DEF_PWD));

        // log output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(payLoad_DEF_PWD, PWD_SIZE));

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

	        // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    // DO change password for RF Configuration
    do
    {
    	// change password for RF Configuration
    	error = rfalST25xVPollerWritePassword( reqFlag, uid, RF_PWD_0, payLoad_RF_CONFIG_PWD, sizeof(payLoad_RF_CONFIG_PWD) );

        // log output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(payLoad_RF_CONFIG_PWD, PWD_SIZE));

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;
        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

	        // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    /********** END Read/Write Permission Configuration **********/





    /********** BEGIN Write De-virginized Marker **********/

    // set nextBlock to start of Recipe address block (Block 56, Address 224)
    nextBlock = STAMP_BLOCK;

    // DO present RF Configuration password
     do
     {
    	// present RF Configuration password to open RF Security Session
    	error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_1, payLoad_RF_AREA_1_PWD, sizeof(payLoad_RF_AREA_1_PWD));

        // log output to console
        DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(payLoad_RF_AREA_1_PWD, sizeof(payLoad_RF_AREA_1_PWD)));

    	// IF there was no error that arose when presenting the password
    	if (error == 0)
    	{
			// write De-virginized Marker
			error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, nextBlock, payLoad_Factory_Stamp, sizeof(payLoad_ID_INFO));

			// log output to console
            DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str(payLoad_Factory_Stamp, BLOCK_SIZE));

    	}
    	// END IF

        // IF there was no error reading
        if (error == 0)
        {
            // set Transmit Success Sentinel to TRUE
            transmitSuccess = 1;

            // reset failure counter to 0
            failureCounter = 0;

            // Transmit Success
            DEBUG_LOG("DV Mark Write Success\r\n");

        }

        // ELSE IF there was an error reading and there have not been 10 consecutive fails
        else if (failureCounter < MAX_FAILS)
        {
            // increment failure counter
            failureCounter++;

            // Delay for 1 second in case of noise
            platformDelay(1000);
        }

        // ELSE there are too many failures
        else
        {
            // return Write Fail, abort rest of Function
            return WRITE_FAIL;
        }
        // END IF

    }
    // WHILE transmit success sentinel == false AND Failure Counter < 10
    while ((transmitSuccess != 1) && failureCounter <= MAX_FAILS);

    /********** END Write De-virginized Marker **********/

    // All writes success so Return Write Pass
    return WRITE_PASS;
}
// END factoryInitializer function





/****************************************************************************
* Function Name    : Factory De-Initializer
* Date             : 02/06/2023
* Author           : Paul E. Fritzen
* Description      : Resets factory initialization with verification output
*                     to console. De-initializes CC File, NDEF, Factory
*                     configuration, and removes configures Read/Write
*                     Protection, as well resets and de-assigns Passwords
*                     for User Memory Area 1, as well as resetting the
*                     configuration password.
*
*  Input Parameters: nfcvDev, a pointer to an NFC Device Structure
*  Return          : error, signifies an error occurred if not 0 or no error
*                     occurred if 0
*
*****************************************************************************/

// BEGIN deInitializer function
static uint8_t deInitializer( rfalNfcvListenDevice *nfcvDev )
{
    //constants
    const uint8_t RF_PWD_0   = 0x00;    // password number designation of RF Configuration Password
    const uint8_t RF_PWD_1   = 0x01;    // password number designation of RF Area 1 Password
    const uint8_t RFA1SS     = 0x04;    // address register of RF Area 1 Password
    const uint8_t WRT_UNLOCK = 0x00;    // bit weight to enable password protected Writes for a user memory area
    const uint8_t MEM_FTPRNT = 60;      // size of memory area in blocks that may have been pissed in

    //variables
    ReturnCode  error;                  // return code for errors. IF 0 then there are no errors
    uint8_t     *uid;                   // Unique Identifier of an NFC tag found within the antenna's field
    uint8_t     reqFlag;                // Request Flag, used to tell Middleware Functions we are working with an NFC-V Tag
    uint8_t     blockLength;            // length of a block of data. For the st25dv04k this will always be 4 bytes.
    uint8_t     failureCounter    = 0;  // counter used to track number of times write/read has failed
    uint8_t     transmitSuccess   = 0;  // sentinel boolean used to gate guard against transmission failure
    uint8_t     septicBlock = 0;        // used to keep track of index of entire full size byte array to be written in a FOR LOOP

    // Block of 4 zeros
    static uint8_t payLoad_Eraser[BLOCK_SIZE] =
    {
        0x00,
        0x00,
        0x00,
        0x00
    };

    // Default Password
    static uint8_t payLoad_DEF_PWD[PWD_SIZE] =
    {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    // RF Config Password
    static uint8_t payLoad_RF_CONFIG_PWD[PWD_SIZE] =
    {
        'p',
        'w',
        'd',
        '1',
        '2',
        '3',
        '4',
        '5'
    };

/**************************************** BEGIN FUNCTION ****************************************/

    // set UID
    uid = nfcvDev->InvRes.UID;

    // set flag showing NFC Standard in use is NFC-V/ISO 15693
    reqFlag = RFAL_NFCV_REQ_FLAG_DEFAULT;

    // set block Length
    blockLength = BLOCK_SIZE;

    /********** BEGIN Read/Write Permission De-Configuration **********/

    // present RF Configuration password to open RF Security Session
    error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_0, payLoad_RF_CONFIG_PWD, sizeof(payLoad_RF_CONFIG_PWD) );

    // write results to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_DEF_PWD, PWD_SIZE) );

    // IF an error was detected
    if (error != 0)
    {
        // return the WRITE FAIL error code, abort early
        return WRITE_FAIL;
    }
    // END IF

    // change write protection for area 1 back to factory default
    error = rfalST25xVPollerWriteConfiguration( reqFlag, uid, RFA1SS, WRT_UNLOCK );

    // write results to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", WRT_UNLOCK );

    // IF an error was detected
    if (error != 0)
    {
        // return the WRITE FAIL error code, abort early
        return WRITE_FAIL;
    }
    // END IF

    // present RF Configuration password to open RF Security Session
    error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_1, payLoad_RF_CONFIG_PWD, sizeof(payLoad_RF_CONFIG_PWD) );

    // write output to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_RF_CONFIG_PWD, PWD_SIZE) );

    // IF an error was detected
    if (error != 0)
    {
        // return the WRITE FAIL error code, abort early
        return WRITE_FAIL;
    }
    // END IF

    // change password for Memory Area 1 back to default
    error = rfalST25xVPollerWritePassword( reqFlag, uid, RF_PWD_1, payLoad_DEF_PWD, sizeof(payLoad_DEF_PWD) );

    // log output to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_DEF_PWD, PWD_SIZE) );

    // IF an error was detected
    if (error != 0)
    {
        // return the WRITE FAIL error code, abort early
        return WRITE_FAIL;
    }

    // present RF Configuration password to open RF Security Session
    error = rfalST25xVPollerPresentPassword( reqFlag, uid, RF_PWD_0, payLoad_RF_CONFIG_PWD, sizeof(payLoad_RF_CONFIG_PWD) );

    // log output to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_RF_CONFIG_PWD, PWD_SIZE) );

    // IF an error was detected, abort early
    if (error != 0)
    {
        return WRITE_FAIL;
    }

    // change password for RF Configuration
    error = rfalST25xVPollerWritePassword( reqFlag, uid, RF_PWD_0, payLoad_DEF_PWD, sizeof(payLoad_DEF_PWD) );

    // log output to console
    DEBUG_LOG(" Write Block: %s Data: %s\r\n", (error != ERR_NONE) ? "FAIL": "OK", hex2Str( payLoad_DEF_PWD, PWD_SIZE) );

    // IF an error was detected
    if (error != 0)
    {
        // return the WRITE FAIL error code, abort early
        return WRITE_FAIL;
    }
    // END IF

    /********** END Read/Write Permission De-Configuration **********/





    /********** BEGIN Eraser **********/
    // FOR Each Block in the NDEF and Recipe Areas
    for (septicBlock = 0; septicBlock <  MEM_FTPRNT; septicBlock++)
    {
        // DO overwrite every single block with all 0's
        do
        {
            // write good CC File
            error = rfalNfcvPollerWriteSingleBlock(reqFlag, uid, septicBlock, payLoad_Eraser, blockLength);
            // IF there was no error reading
            if (error == 0)
            {
                // set Transmit Success Sentinel to TRUE
            	transmitSuccess = 1;
                // reset failure counter to 0
            	failureCounter = 0;
            }
            // ELSE IF there was an error reading and there have not been 10 consecutive fails
            else if (failureCounter < MAX_FAILS)
            {
                // increment failure counter
                failureCounter++;
            }
            // ELSE there are too many failures
            else
            {
                // return Write Fail, abort rest of Function
                return WRITE_FAIL;
            }
        }
        // WHILE transmit success sentinel == false AND Failure Counter < 10
        while ((transmitSuccess != 1) && failureCounter < MAX_FAILS);
    }
    /********** END Eraser **********/

    // All writes success so Return Write Pass
    return WRITE_PASS;
}
// END deInitializer function

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
 * icm_models.h
 *
 *  Created on: Mar 13, 2023
 *      Author: Ed Maskelony
 *
 * Contains the definitions of product IDs and factory-default config.
 *
 * Configurations are selected with conditional compile logic to reduce ROM size for the Nucleo board.
 * To select a product, define <model number>=1 in the symbols section of the build configuration.
 */

#ifndef ICM_MODELS_H_
#define ICM_MODELS_H_

/* Note: Enclose each case with its own #if and #endif, do NOT use #elif.
 *       This ensures we will not accidentally compile an invalid version
 *       with multiple configs defined.
 */

#if ICM325A
#define ICM_MODEL_STR "ICM325A"
 // Product ID info for Universal Head Pressure Controller
    static uint8_t payLoad_ID_INFO[] =
    {
        0x02,      // data format number
        0x55,0xAA, // Product ID, 2 bytes
        0x0A       // Device Model #
    };

    // default factory configuration for Universal Head Pressure Controller
    // total byte count must be a multiple of 4.
    static uint8_t payLoad_FactoryConfig[] =
    {
        0x88, 0xC9, 0x00, 0x00,
        0x64, 0x32, 0x19, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
#endif  // ICM325A

#if ICM_UFPT_2
#define ICM_MODEL_STR "ICM_UFPT_2"
    // Product ID info for 2-wire universal timer model
    static uint8_t payLoad_ID_INFO[] =
    {
        0x02,      // data format number
        0x55,0xAA, // Product ID, 2 bytes
        0x02       // Device Model #
    };

    // default factory configuration for 2-wire universal timer model.
    // total byte count must be a multiple of 4.
    static uint8_t payLoad_FactoryConfig[] =
    {
        0xA5, 0x01, 0x00, 0x02,
        0x58, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
#endif  // ICM_UFPT_2

#if ICM_UFPT_5
#define ICM_MODEL_STR "ICM_UFPT_5"
    // Product ID info for 5-wire universal timer model
    static uint8_t payLoad_ID_INFO[] =
    {
        0x02,      // data format number
        0x55,0xAA, // Product ID, 2 bytes
        0x05       // Device Model #
    };

    // default factory configuration for for 5-wire universal timer model
    // total byte count must be a multiple of 4.
    static uint8_t payLoad_FactoryConfig[] =
    {
        0xA5, 0x01, 0x00, 0x02,
        0x58, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
#endif  // ICM_UFPT_5

// If more than one was defined above, the preprocessor will error out when redefining ICM_MODEL_STR.
// If no model was set, force compilation to fail here.
#ifndef ICM_MODEL_STR
#error "Must define exactly one model number in STM32CubeIDE build configuration"
#endif

// Size of factory default config payload, in bytes
#define DEFAULT_CONFIG_SIZE (sizeof(payLoad_FactoryConfig))

#endif /* ICM_MODELS_H_ */

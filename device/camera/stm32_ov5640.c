// SPDX-License-Identifier: MIT
// Copyright (c) 2025 EmbedDIP

/**
 * @file stm32_ov5640.c
 * @brief Consolidated OV5640 camera driver implementation for STM32
 *
 */

#include <embedDIP_configs.h>

#ifdef DEVICE_OV5640

    #include "board/stm32f7/configs.h"
    #include "device/camera/camera.h"
    #include "device/serial/serial.h"

    #include <stdarg.h>
    #include <stdint.h>
    #include <stdio.h>

    #include "stm32_ov5640.h"
    #include "stm32f7xx_hal.h"

    /* ============================================================================
     * I2C COMMUNICATION LAYER (Internal - Static Functions)
     * ============================================================================ */

    /* Private defines */
    #define I2C1_TIMING ((uint32_t)0x40912732) /**< I2C timing for 100kHz at 216MHz */

/* Private variables */
static I2C_HandleTypeDef hI2cCamera = {0};

/* Private function prototypes */
static void I2C_MspInit(void);
static void I2C_Error(uint8_t Addr);

/**
 * @brief  Initialize camera I2C peripheral
 * @note   Configures I2C1: PB8 (SCL), PB9 (SDA) at 100kHz
 */
static void CAMERA_IO_Init(void)
{
    if (HAL_I2C_GetState(&hI2cCamera) == HAL_I2C_STATE_RESET) {
        /* Configure I2C peripheral */
        hI2cCamera.Instance = I2C1;
        hI2cCamera.Init.Timing = I2C1_TIMING;
        hI2cCamera.Init.OwnAddress1 = 0;
        hI2cCamera.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        hI2cCamera.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        hI2cCamera.Init.OwnAddress2 = 0;
        hI2cCamera.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        hI2cCamera.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

        /* Initialize I2C MSP and peripheral */
        I2C_MspInit();
        HAL_I2C_Init(&hI2cCamera);
    }
}

/**
 * @brief  Write single byte to camera register (16-bit register address)
 * @param  DeviceAddr: I2C device address
 * @param  Reg: 16-bit register address
 * @param  Value: 8-bit value to write
 */
static void CAMERA_IO_Write(uint16_t DeviceAddr, uint16_t Reg, uint8_t Value)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(&hI2cCamera, DeviceAddr, Reg, I2C_MEMADD_SIZE_16BIT, &Value, 1, 100);

    if (status != HAL_OK) {
        I2C_Error(DeviceAddr);
    }
}

/**
 * @brief  Read single byte from camera register (16-bit register address)
 * @param  DeviceAddr: I2C device address
 * @param  Reg: 16-bit register address
 * @retval 8-bit value read from register
 */
static uint8_t CAMERA_IO_Read(uint16_t DeviceAddr, uint16_t Reg)
{
    uint8_t value = 0;
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(&hI2cCamera, DeviceAddr, Reg, I2C_MEMADD_SIZE_16BIT, &value, 1, 100);

    if (status != HAL_OK) {
        I2C_Error(DeviceAddr);
    }

    return value;
}

/**
 * @brief  Camera delay wrapper
 * @param  Delay: Delay in milliseconds
 */
static void CAMERA_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

/**
 * @brief  Initialize I2C MSP (GPIO and clocks)
 * @note   Configures PB8 (I2C1_SCL) and PB9 (I2C1_SDA) for STM32F746G-Discovery
 */
static void I2C_MspInit(void)
{
    GPIO_InitTypeDef gpio_init;

    /* Enable GPIO clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure I2C1 SCL (PB8) */
    gpio_init.Pin = GPIO_PIN_8;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    /* Configure I2C1 SDA (PB9) */
    gpio_init.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    /* Enable I2C clock */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* Force and release I2C peripheral reset */
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();

    /* Enable I2C interrupts */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

/**
 * @brief  Handle I2C communication error
 * @param  Addr: I2C device address that caused error
 * @note   Currently performs I2C deinitialization and reinitialization
 */
static void I2C_Error(uint8_t Addr)
{
    (void)Addr; /* Unused parameter */

    /* De-initialize and re-initialize I2C to recover from error */
    HAL_I2C_DeInit(&hI2cCamera);
    HAL_I2C_Init(&hI2cCamera);
}

/* ============================================================================
 * OV5640 REGISTER CONFIGURATIONS AND DRIVER
 * ============================================================================ */

/**
 * @brief Main initialization sequence for OV5640
 * @note Configures basic camera registers for operation
 */
const uint16_t OV5640_Init[][2] = {
    {0x3103, 0x11}, {0x3008, 0x82}, {0x3103, 0x03}, {0x3017, 0xFF}, {0x3018, 0xf3}, {0x3034, 0x18},
    {0x3008, 0x02}, {0x3035, 0x41}, {0x3036, 0x60}, {0x3037, 0x13}, {0x3108, 0x01}, {0x3630, 0x36},
    {0x3631, 0x0e}, {0x3632, 0xe2}, {0x3633, 0x12}, {0x3621, 0xe0}, {0x3704, 0xa0}, {0x3703, 0x5a},
    {0x3715, 0x78}, {0x3717, 0x01}, {0x370b, 0x60}, {0x3705, 0x1a}, {0x3905, 0x02}, {0x3906, 0x10},
    {0x3901, 0x0a}, {0x3731, 0x12}, {0x3600, 0x08}, {0x3601, 0x33}, {0x302d, 0x60}, {0x3620, 0x52},
    {0x371b, 0x20}, {0x471c, 0x50}, {0x3a13, 0x43}, {0x3a18, 0x00}, {0x3a19, 0xf8}, {0x3635, 0x13},
    {0x3636, 0x03}, {0x3634, 0x40}, {0x3622, 0x01}, {0x3c01, 0x34}, {0x3c04, 0x28}, {0x3c05, 0x98},
    {0x3c06, 0x00}, {0x3c07, 0x00}, {0x3c08, 0x01}, {0x3c09, 0x2c}, {0x3c0a, 0x9c}, {0x3c0b, 0x40},
    {0x3820, 0x00}, {0x3821, 0x06}, {0x3814, 0x31}, {0x3815, 0x31}, {0x3800, 0x00}, {0x3801, 0x00},
    {0x3802, 0x00}, {0x3803, 0x04}, {0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9b},
    {0x3808, 0x03}, {0x3809, 0x20}, {0x380a, 0x02}, {0x380b, 0x58}, {0x380c, 0x07}, {0x380d, 0x68},
    {0x380e, 0x03}, {0x380f, 0xd8}, {0x3810, 0x00}, {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x06},
    {0x3618, 0x00}, {0x3612, 0x29}, {0x3708, 0x64}, {0x3709, 0x52}, {0x370c, 0x03}, {0x3a02, 0x03},
    {0x3a03, 0xd8}, {0x3a08, 0x01}, {0x3a09, 0x27}, {0x3a0a, 0x00}, {0x3a0b, 0xf6}, {0x3a0e, 0x03},
    {0x3a0d, 0x04}, {0x3a14, 0x03}, {0x3a15, 0xd8}, {0x4001, 0x02}, {0x4004, 0x02}, {0x3000, 0x00},
    {0x3002, 0x1c}, {0x3004, 0xff}, {0x3006, 0xc3}, {0x300e, 0x58}, {0x302e, 0x00}, {0x4740, 0x20},
    {0x4300, 0x30}, {0x501f, 0x00}, {0x4713, 0x03}, {0x4407, 0x04}, {0x440e, 0x00}, {0x460b, 0x35},
    {0x460c, 0x23}, {0x4837, 0x22}, {0x3824, 0x02}, {0x5000, 0xa7}, {0x5001, 0xa3}, {0x5180, 0xff},
    {0x5181, 0xf2}, {0x5182, 0x00}, {0x5183, 0x14}, {0x5184, 0x25}, {0x5185, 0x24}, {0x5186, 0x09},
    {0x5187, 0x09}, {0x5188, 0x09}, {0x5189, 0x75}, {0x518a, 0x54}, {0x518b, 0xe0}, {0x518c, 0xb2},
    {0x518d, 0x42}, {0x518e, 0x3d}, {0x518f, 0x56}, {0x5190, 0x46}, {0x5191, 0xf8}, {0x5192, 0x04},
    {0x5193, 0x70}, {0x5194, 0xf0}, {0x5195, 0xf0}, {0x5196, 0x03}, {0x5197, 0x01}, {0x5198, 0x04},
    {0x5199, 0x12}, {0x519a, 0x04}, {0x519b, 0x00}, {0x519c, 0x06}, {0x519d, 0x82}, {0x519e, 0x38},
    {0x5381, 0x1e}, {0x5382, 0x5b}, {0x5383, 0x08}, {0x5384, 0x0a}, {0x5385, 0x7e}, {0x5386, 0x88},
    {0x5387, 0x7c}, {0x5388, 0x6c}, {0x5389, 0x10}, {0x538a, 0x01}, {0x538b, 0x98}, {0x5300, 0x08},
    {0x5301, 0x30}, {0x5302, 0x10}, {0x5303, 0x00}, {0x5304, 0x08}, {0x5305, 0x30}, {0x5306, 0x08},
    {0x5307, 0x16}, {0x5309, 0x08}, {0x530a, 0x30}, {0x530b, 0x04}, {0x530c, 0x06}, {0x5480, 0x01},
    {0x5481, 0x08}, {0x5482, 0x14}, {0x5483, 0x28}, {0x5484, 0x51}, {0x5485, 0x65}, {0x5486, 0x71},
    {0x5487, 0x7d}, {0x5488, 0x87}, {0x5489, 0x91}, {0x548a, 0x9a}, {0x548b, 0xaa}, {0x548c, 0xb8},
    {0x548d, 0xcd}, {0x548e, 0xdd}, {0x548f, 0xea}, {0x5490, 0x1d}, {0x5580, 0x02}, {0x5583, 0x40},
    {0x5584, 0x10}, {0x5589, 0x10}, {0x558a, 0x00}, {0x558b, 0xf8}, {0x5800, 0x23}, {0x5801, 0x14},
    {0x5802, 0x0f}, {0x5803, 0x0f}, {0x5804, 0x12}, {0x5805, 0x26}, {0x5806, 0x0c}, {0x5807, 0x08},
    {0x5808, 0x05}, {0x5809, 0x05}, {0x580a, 0x08}, {0x580b, 0x0d}, {0x580c, 0x08}, {0x580d, 0x03},
    {0x580e, 0x00}, {0x580f, 0x00}, {0x5810, 0x03}, {0x5811, 0x09}, {0x5812, 0x07}, {0x5813, 0x03},
    {0x5814, 0x00}, {0x5815, 0x01}, {0x5816, 0x03}, {0x5817, 0x08}, {0x5818, 0x0d}, {0x5819, 0x08},
    {0x581a, 0x05}, {0x581b, 0x06}, {0x581c, 0x08}, {0x581d, 0x0e}, {0x581e, 0x29}, {0x581f, 0x17},
    {0x5820, 0x11}, {0x5821, 0x11}, {0x5822, 0x15}, {0x5823, 0x28}, {0x5824, 0x46}, {0x5825, 0x26},
    {0x5826, 0x08}, {0x5827, 0x26}, {0x5828, 0x64}, {0x5829, 0x26}, {0x582a, 0x24}, {0x582b, 0x22},
    {0x582c, 0x24}, {0x582d, 0x24}, {0x582e, 0x06}, {0x582f, 0x22}, {0x5830, 0x40}, {0x5831, 0x42},
    {0x5832, 0x24}, {0x5833, 0x26}, {0x5834, 0x24}, {0x5835, 0x22}, {0x5836, 0x22}, {0x5837, 0x26},
    {0x5838, 0x44}, {0x5839, 0x24}, {0x583a, 0x26}, {0x583b, 0x28}, {0x583c, 0x42}, {0x583d, 0xce},
    {0x5025, 0x00}, {0x3a0f, 0x30}, {0x3a10, 0x28}, {0x3a1b, 0x30}, {0x3a1e, 0x26}, {0x3a11, 0x60},
    {0x3a1f, 0x14}, {0x3008, 0x02},
};

/**
 * @brief Resolution configuration for 640x480 (VGA)
 */
const uint16_t OV5640_VGA[][2] = {
    {0x3808, 0x02},
    {0x3809, 0x80},
    {0x380a, 0x01},
    {0x380b, 0xE0},
    {0x4300, 0x6F},
    {0x4740, 0x22},
    {0x501F, 0x01},
};

/**
 * @brief Resolution configuration for 480x272
 */
const uint16_t OV5640_480x272[][2] = {
    {0x3808, 0x01},
    {0x3809, 0xE0},
    {0x380a, 0x01},
    {0x380b, 0x10},
    {0x4300, 0x6F},
    {0x4740, 0x22},
    {0x501f, 0x01},
};

/**
 * @brief Resolution configuration for 320x240 (QVGA)
 */
const uint16_t OV5640_QVGA[][2] = {
    {0x3808, 0x01},
    {0x3809, 0x40},
    {0x380a, 0x00},
    {0x380b, 0xF0},
    {0x4300, 0x6F},
    {0x4740, 0x22},
    {0x501f, 0x01},
};

/**
 * @brief Resolution configuration for 160x120 (QQVGA)
 */
const uint16_t OV5640_QQVGA[][2] = {
    {0x3808, 0x00},
    {0x3809, 0xA0},
    {0x380a, 0x00},
    {0x380b, 0x78},
    {0x4300, 0x6F},
    {0x4740, 0x22},
    {0x501f, 0x01},
};

/**
 * @brief Camera driver structure (legacy interface)
 */
CAMERA_DrvTypeDef ov5640_drv = {
    ov5640_Init,
    ov5640_ReadID,
    NULL, /* Config function removed */
};

/**
 * @brief Initialize the OV5640 camera
 * @param DeviceAddr: I2C device address (typically 0x78)
 * @param resolution: Resolution setting (CAMERA_R160x120, CAMERA_R320x240, etc.)
 */
void ov5640_Init(uint16_t DeviceAddr, uint32_t resolution)
{
    uint32_t index;

    /* Write main initialization sequence */
    for (index = 0; index < (sizeof(OV5640_Init) / 4); index++) {
        CAMERA_IO_Write(DeviceAddr, OV5640_Init[index][0], OV5640_Init[index][1]);
    }

    /* Configure resolution-specific settings */
    switch (resolution) {
    case CAMERA_R160x120:
        for (index = 0; index < (sizeof(OV5640_QQVGA) / 4); index++) {
            CAMERA_IO_Write(DeviceAddr, OV5640_QQVGA[index][0], OV5640_QQVGA[index][1]);
        }
        break;

    case CAMERA_R320x240:
        for (index = 0; index < (sizeof(OV5640_QVGA) / 4); index++) {
            CAMERA_IO_Write(DeviceAddr, OV5640_QVGA[index][0], OV5640_QVGA[index][1]);
        }
        break;

    case CAMERA_R480x272:
        for (index = 0; index < (sizeof(OV5640_480x272) / 4); index++) {
            CAMERA_IO_Write(DeviceAddr, OV5640_480x272[index][0], OV5640_480x272[index][1]);
        }
        break;

    case CAMERA_R640x480:
        for (index = 0; index < (sizeof(OV5640_VGA) / 4); index++) {
            CAMERA_IO_Write(DeviceAddr, OV5640_VGA[index][0], OV5640_VGA[index][1]);
        }
        break;

    default:
        /* Use default resolution from init sequence */
        break;
    }
}

/**
 * @brief Read OV5640 chip ID
 * @param DeviceAddr: I2C device address
 * @retval 16-bit chip ID (should be 0x5640 for OV5640)
 */
uint16_t ov5640_ReadID(uint16_t DeviceAddr)
{
    uint16_t chip_id;

    /* Initialize I2C interface */
    CAMERA_IO_Init();

    /* Soft reset camera */
    CAMERA_IO_Write(DeviceAddr, 0x3008, 0x80);
    CAMERA_Delay(500);

    /* Read chip ID from registers 0x300A and 0x300B */
    chip_id = CAMERA_IO_Read(DeviceAddr, 0x300A) << 8;
    chip_id |= CAMERA_IO_Read(DeviceAddr, 0x300B);

    return chip_id;
}

/**
 * @brief Set OV5640 pixel format
 * @param DeviceAddr: I2C device address
 * @param format: Pixel format (OV5640_PIXEL_FORMAT_Y8, RGB565, RGB888, YUV422)
 * @retval 0 on success, -1 on error
 */
int OV5640_SetPixelFormat(uint16_t DeviceAddr, uint32_t format)
{
    /* Validate format */
    if (format != 0x10 && format != 0x6F && format != 0x23 && format != 0x30) {
        return -1; /* Invalid format */
    }

    /* Write format to register 0x4300 (format control) */
    CAMERA_IO_Write(DeviceAddr, 0x4300, (uint8_t)format);

    return 0;
}

/* ============================================================================
 * STM32 PLATFORM-SPECIFIC CAMERA INTERFACE
 * ============================================================================ */

/* External HAL handles */
extern DCMI_HandleTypeDef hdcmi;
extern DMA2D_HandleTypeDef hdma2d;

/* Public variables */
volatile uint8_t frame_capture_complete = 0;

/* Private variables */
static CAMERA_DrvTypeDef *camera_driv;
static ImageFormat current_camera_format = IMAGE_FORMAT_RGB565;

/* Internal types */
typedef enum {
    CAMERA_OK = 0x00,
    CAMERA_ERROR = 0x01,
    CAMERA_TIMEOUT = 0x02,
    CAMERA_NOT_DETECTED = 0x03,
    CAMERA_NOT_SUPPORTED = 0x04
} Camera_StatusTypeDef;

/* Forward declarations for internal functions */
static void CAMERA_PwrUp(void);
static void CAMERA_PwrDown(void);
static int32_t map_format_to_ov5640(ImageFormat format);
static int validate_resolution(ImageResolution resolution);
static int camera_init(ImageResolution resolution, ImageFormat format);
static int camera_capture(captureMode mode, Image *inImg);
static int camera_stop(void);
static int camera_setRes(ImageResolution resolution);

/**
 * @brief  CAMERA power up
 * @retval None
 */
static void CAMERA_PwrUp(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable GPIO clock */
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /*** Configure the GPIO ***/
    /* Configure DCMI GPIO as alternate function */
    gpio_init_structure.Pin = GPIO_PIN_13;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);

    /* De-assert the camera POWER_DOWN pin (active high) */
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_13, GPIO_PIN_RESET);

    HAL_Delay(3); /* POWER_DOWN de-asserted during 3ms */
}

/**
 * @brief  CAMERA power down
 * @retval None
 */
static void CAMERA_PwrDown(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable GPIO clock */
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /*** Configure the GPIO ***/
    /* Configure DCMI GPIO as alternate function */
    gpio_init_structure.Pin = GPIO_PIN_13;
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);

    /* Assert the camera POWER_DOWN pin (active high) */
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
 * @brief Map ImageFormat to OV5640 pixel format
 * @param format The ImageFormat enum value
 * @return OV5640 pixel format constant, or -1 if unsupported
 */
static int32_t map_format_to_ov5640(ImageFormat format)
{
    switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
        return OV5640_PIXEL_FORMAT_Y8;
    case IMAGE_FORMAT_RGB565:
        return OV5640_PIXEL_FORMAT_RGB565;
    case IMAGE_FORMAT_RGB888:
        return OV5640_PIXEL_FORMAT_RGB888;
    case IMAGE_FORMAT_YUV:
        return OV5640_PIXEL_FORMAT_YUV422;
    case IMAGE_FORMAT_HSI:
    case IMAGE_FORMAT_MASK:
    default:
        return -1;  // Unsupported format
    }
}

/**
 * @brief Validate if resolution is supported by OV5640
 * @param resolution The ImageResolution enum value
 * @return 0 if valid, -1 if unsupported
 */
static int validate_resolution(ImageResolution resolution)
{
    // OV5640 supports: QQVGA, QVGA, WQVGA, VGA
    switch (resolution) {
    case IMAGE_RES_QQVGA:  // 160x120
    case IMAGE_RES_QVGA:   // 320x240
    case IMAGE_RES_WQVGA:  // 480x272
    case IMAGE_RES_VGA:    // 640x480
        return 0;          // Valid
    default:
        return -1;  // Unsupported resolution
    }
}

/**
 * @brief Initialize camera with specified resolution and format
 * @param resolution Image resolution
 * @param format Image format
 * @return 0 on success, -1 on error
 */
static int camera_init(ImageResolution resolution, ImageFormat format)
{
    // Validate resolution
    if (validate_resolution(resolution) != 0) {
        return -1;  // Invalid resolution
    }

    // Validate and map format
    int32_t ov5640_format = map_format_to_ov5640(format);
    if (ov5640_format < 0) {
        return -1;  // Invalid format
    }

    // Store the current format for setRes
    current_camera_format = format;

    CAMERA_PwrDown();
    CAMERA_PwrUp();
    HAL_Delay(1000);

    /* Read ID of Camera module via I2C */
    if (ov5640_ReadID(CAMERA_I2C_ADDRESS) == OV5640_ID) {
        camera_driv = &ov5640_drv;
        /* Initialize the camera driver structure */
        camera_driv->Init(CAMERA_I2C_ADDRESS, resolution);
        HAL_DCMI_DisableCROP(&hdcmi);
        HAL_Delay(500);
    } else {
        return -1;  // Camera not detected
    }

    // Set the validated pixel format
    if (OV5640_SetPixelFormat(CAMERA_I2C_ADDRESS, (uint32_t)ov5640_format) != 0) {
        return -1;  // Failed to set pixel format
    }

    HAL_Delay(1000);  // Delay for the camera to output correct data
    __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC);

    return 0;
}

/**
 * @brief Capture image from camera
 * @param mode Capture mode (SINGLE or CONTINUOUS)
 * @param inImg Image structure to store captured data
 * @return 0 on success, -1 on error
 */
static int camera_capture(captureMode mode, Image *inImg)
{
    if (inImg == NULL || inImg->pixels == NULL) {
        return -1;
    }
    if (inImg->size == 0U || inImg->depth == 0U) {
        return -1;
    }

    // Clear the frame completion flag
    frame_capture_complete = 0;

    // Calculate DMA transfer size
    // DCMI DMA operates in 32-bit words
    // Total bytes = width × height × depth (depth: 1=GRAYSCALE, 2=RGB565, 3=RGB888)
    // DMA size in 32-bit words = total_bytes / 4
    uint32_t total_bytes = inImg->size * inImg->depth;
    uint32_t dma_size_words = total_bytes / 4;

    // Start DMA transfer
    HAL_StatusTypeDef hal_status =
        HAL_DCMI_Start_DMA(&hdcmi,
                           mode == CONTINUOUS ? DCMI_MODE_CONTINUOUS : DCMI_MODE_SNAPSHOT,
                           (uint32_t)inImg->pixels,
                           dma_size_words);
    if (hal_status != HAL_OK) {
        return -1;
    }

    // For SINGLE mode, wait for frame completion
    if (mode == SINGLE) {
        uint32_t timeout = HAL_GetTick() + 5000;  // 5 second timeout
        while (!frame_capture_complete) {
            if (HAL_GetTick() > timeout) {
                HAL_DCMI_Stop(&hdcmi);
                return -1;  // Timeout error
            }
        }
    }

    // Set image data state to indicate raw pixel data is in the pixels buffer
    inImg->log = IMAGE_DATA_PIXELS;

    return 0;
}

/**
 * @brief Stop camera capture
 * @return 0 on success
 */
static int camera_stop(void)
{
    HAL_DCMI_Stop(&hdcmi);
    return 0;
}

/**
 * @brief Change camera resolution
 * @param resolution New resolution
 * @return 0 on success, -1 on error
 */
static int camera_setRes(ImageResolution resolution)
{
    // Validate resolution
    if (validate_resolution(resolution) != 0) {
        return -1;  // Invalid resolution
    }

    // Get the stored format to preserve it
    int32_t ov5640_format = map_format_to_ov5640(current_camera_format);
    if (ov5640_format < 0) {
        return -1;  // Invalid format
    }

    CAMERA_PwrDown();
    CAMERA_PwrUp();

    /* Read ID of Camera module via I2C */
    if (ov5640_ReadID(CAMERA_I2C_ADDRESS) == OV5640_ID) {
        camera_driv = &ov5640_drv;
        /* Initialize the camera driver structure */
        camera_driv->Init(CAMERA_I2C_ADDRESS, resolution);
        HAL_DCMI_DisableCROP(&hdcmi);
        HAL_Delay(500);
    } else {
        return -1;  // Camera not detected
    }

    // Reconfigure pixel format
    if (OV5640_SetPixelFormat(CAMERA_I2C_ADDRESS, (uint32_t)ov5640_format) != 0) {
        return -1;  // Failed to set pixel format
    }

    HAL_Delay(1000);  // Delay for the camera to output correct data
    __HAL_DCMI_DISABLE_IT(&hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC);

    return 0;
}

/**
 * @brief STM32 OV5640 camera interface structure
 */
camera_t stm32_ov5640 = {.init = camera_init,
                         .capture = camera_capture,
                         .stop = camera_stop,
                         .setRes = camera_setRes};

/**
 * @brief DCMI frame event callback - called when a complete frame is captured
 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
    (void)hdcmi;
    frame_capture_complete = 1;
}

#endif /* DEVICE_OV5640 */

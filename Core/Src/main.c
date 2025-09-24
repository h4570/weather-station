/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "epd3in7_driver.h"
#include "epd3in7_lvgl_adapter.h"
#include "lvgl/lvgl.h"
#include "hasto_logo.h"
#include "bmpxx80.h"
#include <stdlib.h> /* rand */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */

// Buffer for the full black/white image (1bpp)
#define STRIDE_BYTES ((EPD3IN7_WIDTH + 7) / 8)
#define DISPLAY_BUFFER_SIZE (STRIDE_BYTES * EPD3IN7_HEIGHT)
// I1: 2 colors * 4 bytes (ARGB32)
#define LVGL_PALETTE_BYTES 8

static LV_ATTRIBUTE_MEM_ALIGN uint8_t lvgl_buffer[LVGL_PALETTE_BYTES + DISPLAY_BUFFER_SIZE];
static uint8_t epd3in7_adapter_work_buffer[DISPLAY_BUFFER_SIZE];

static void draw_corners(const int32_t image_padding_x, const int32_t image_padding_y)
{
  lv_obj_t *scr = lv_screen_active();
  const int pad = 6;

  // Clear
  lv_obj_clean(scr);

  // White background
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t *tl = lv_label_create(scr);
  lv_label_set_text(tl, "TL");
  lv_obj_set_style_text_color(tl, lv_color_black(), 0);
  lv_obj_align(tl, LV_ALIGN_TOP_LEFT, pad, pad);

  lv_obj_t *tr = lv_label_create(scr);
  lv_label_set_text(tr, "TR");
  lv_obj_set_style_text_color(tr, lv_color_black(), 0);
  lv_obj_align(tr, LV_ALIGN_TOP_RIGHT, -pad, pad);

  lv_obj_t *bl = lv_label_create(scr);
  lv_label_set_text(bl, "BL");
  lv_obj_set_style_text_color(bl, lv_color_black(), 0);
  lv_obj_align(bl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);

  lv_obj_t *br = lv_label_create(scr);
  lv_label_set_text(br, "BR");
  lv_obj_set_style_text_color(br, lv_color_black(), 0);
  lv_obj_align(br, LV_ALIGN_BOTTOM_RIGHT, -pad, -pad);

  // https://github.com/lvgl/lvgl/blob/master/scripts/LVGLImage.py
  // python .\LVGLImage.py --cf I1 --ofmt C .\hasto_logo.png
  lv_obj_t *img = lv_img_create(scr);
  lv_img_set_src(img, &hasto_logo);
  lv_obj_center(img);
  lv_obj_align(img, LV_ALIGN_CENTER, image_padding_x, image_padding_y);

  // lv_obj_t *text = lv_label_create(scr);
  // lv_label_set_text(text, "HELLO E-PAPER");
  // lv_obj_set_style_text_color(text, lv_color_black(), 0);
  // lv_obj_align(text, LV_ALIGN_CENTER, 0, 20);
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits = COM_STOPBITS_1;
  BspCOMInit.Parity = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  epd3in7_driver_handle epd3in7_drv = epd3in7_driver_create((epd3in7_driver_pins){
                                                                .reset_port = DISP_RST_GPIO_Port,
                                                                .reset_pin = DISP_RST_Pin,
                                                                .dc_port = DISP_DC_GPIO_Port,
                                                                .dc_pin = DISP_DC_Pin,
                                                                .busy_port = DISP_BUSY_GPIO_Port,
                                                                .busy_pin = DISP_BUSY_Pin,
                                                                .cs_port = DISP_CS_GPIO_Port,
                                                                .cs_pin = DISP_CS_Pin},
                                                            &hspi2, true);

  epd3in7_lvgl_adapter_handle epd3in7_adapter = epd3in7_lvgl_adapter_create(
      &epd3in7_drv,
      epd3in7_adapter_work_buffer,
      10,
      EPD3IN7_DRIVER_MODE_A2,
      32);

  lv_init();
  lv_tick_set_cb(HAL_GetTick);
  lv_display_t *display = lv_display_create(EPD3IN7_WIDTH, EPD3IN7_HEIGHT);
  lv_display_set_driver_data(display, &epd3in7_adapter);
  lv_display_set_buffers(display, lvgl_buffer, NULL, sizeof(lvgl_buffer), LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(display, epd3in7_lvgl_adapter_flush);
  lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);

  HAL_TIM_Base_Start(&htim1);
  // BMPxx_init(BME280_CS_GPIO_Port, BME280_CS_Pin);
  // BME280_Init(&hspi2, BME280_TEMPERATURE_16BIT, BME280_PRESSURE_ULTRALOWPOWER, BME280_HUMIDITY_STANDARD, BME280_NORMALMODE);
  // BME280_SetConfig(BME280_STANDBY_MS_10, BME280_FILTER_OFF);

  int32_t random_offset_x = 0;
  int32_t random_offset_y = -20;
  float temperature, humidity;
  int32_t pressure;

  while (1)
  {
    draw_corners(random_offset_x, random_offset_y);

    random_offset_x = (rand() % 161) - 80;
    random_offset_y = (rand() % 161) - 80;

    lv_timer_handler();

    // BME280_ReadTemperatureAndPressureAndHumidity(&temperature, &pressure, &humidity);

    HAL_Delay(1000);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

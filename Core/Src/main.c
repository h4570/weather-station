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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "epd_3in7.h"

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
static UBYTE frame_bw[(EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT) / 8];

// Rectangle parameters
#define RECT_W 80
#define RECT_H 80
#define RECT_X ((EPD_3IN7_WIDTH - RECT_W) / 2)
#define RECT_Y ((EPD_3IN7_HEIGHT - RECT_H) / 2)
#define STRIP_H (RECT_Y + RECT_H) // We are sending 0..(RECT_Y + RECT_H - 1)

static UBYTE strip_buf[(EPD_3IN7_WIDTH * STRIP_H) / 8];

// Helper: bytes per row in the full image
static inline UWORD bytes_per_row(void) { return (EPD_3IN7_WIDTH / 8); }

// Helper: bytes for given number of rows
static inline UWORD rows_to_bytes(UWORD rows) { return rows * bytes_per_row(); }

// Copy from the shadow buffer to the strip buffer
static void strip_copy_from_shadow(UBYTE *dst_strip, const UBYTE *shadow, UWORD rows)
{
  memcpy(dst_strip, shadow, rows_to_bytes(rows));
}

// Copy back the TOP strip to the shadow buffer
static void strip_copy_to_shadow(UBYTE *shadow, const UBYTE *src_strip, UWORD rows)
{
  memcpy(shadow, src_strip, rows_to_bytes(rows));
}

// Draw a filled square (black/white) on the strip buffer
static void draw_filled_square_on_strip(UBYTE *strip, UBYTE is_black)
{
  for (UWORD y = 0; y < RECT_H; y++)
  {
    UWORD gy = RECT_Y + y;
    UWORD row_base = gy * bytes_per_row();
    for (UWORD x = 0; x < RECT_W; x++)
    {
      UWORD gx = RECT_X + x;
      UWORD idx = row_base + (gx >> 3);
      UBYTE mask = (0x80 >> (gx & 7));
      if (is_black)
      {
        strip[idx] &= ~mask; // black (bit=0)
      }
      else
      {
        strip[idx] |= mask; // white (bit=1)
      }
    }
  }
}

// Draw a checkerboard pattern on the full shadow buffer
static void draw_checker_full(void)
{
  memset(frame_bw, 0xFF, sizeof(frame_bw)); // start with all white

  for (UWORD y = 0; y < EPD_3IN7_HEIGHT; y++)
  {
    for (UWORD x = 0; x < EPD_3IN7_WIDTH; x++)
    {
      if (((x >> 4) ^ (y >> 4)) & 1)
      {
        UWORD idx = (y * (EPD_3IN7_WIDTH / 8)) + (x >> 3);
        frame_bw[idx] &= ~(0x80 >> (x & 7));
      }
    }
  }
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

  EPD_3IN7_1Gray_Init();
  EPD_3IN7_1Gray_Clear(1); // 1 = GC

  UBYTE black = 1;              // starting with black square
  UBYTE partial_since_full = 0; // count partial updates since last full refresh
  UBYTE toggles = 0;            // total square color toggles

  // One-time "top full" push so partials won't blank the rest
  draw_checker_full();
  EPD_3IN7_1Gray_Display(frame_bw, 1); // GC
  EPD_3IN7_1Gray_Display(frame_bw, 2); // DU

  while (1)
  {
    // 1) Base of the partial = current screen content (shadow buffer)
    strip_copy_from_shadow(strip_buf, frame_bw, STRIP_H);

    // 2) Draw the new filled square (black/white) on the strip
    draw_filled_square_on_strip(strip_buf, black);

    // 3) Send the partial update (top stripe only)
    EPD_3IN7_1Gray_Display_Top(strip_buf, STRIP_H);

    // 4) Update shadow buffer so the next partial starts from the latest image
    strip_copy_to_shadow(frame_bw, strip_buf, STRIP_H);

    // 5) Toggle color for the next iteration
    black ^= 1;
    toggles++;
    partial_since_full++;

    // 6) Every 5 partials, do a single full refresh to mitigate ghosting
    if (partial_since_full >= 5)
    {
      // Full-screen refresh with the current shadow buffer
      EPD_3IN7_1Gray_Display(frame_bw, 2); // DU
      partial_since_full = 0;
    }

    // 7) After 20 toggles, put the panel to sleep and stop using it
    if (toggles >= 20)
    {
      EPD_3IN7_Sleep();
      break; // exit the loop; do not touch the panel anymore
    }

    HAL_Delay(500);

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

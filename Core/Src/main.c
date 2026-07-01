/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body — HX711 + USART1 (Bluetooth) + USART2 (CH340 debug)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
/* USER CODE END Includes */

UART_HandleTypeDef huart1;   // Bluetooth — PA9 (TX) / PA10 (RX)
UART_HandleTypeDef huart2;   // CH340 debug — PA2 (TX) / PA3 (RX)

/* USER CODE BEGIN PV */
#define HX711_SCALE       29400.0f
#define AVG_SAMPLES       10
#define HX711_TIMEOUT_MS  200

static volatile uint8_t tare_requested = 0;
static uint8_t rx_byte;
static char rx_buffer[16];
static uint8_t rx_index = 0;
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

/**
 * Leitura única do HX711, com timeout (CORREÇÃO #1).
 * Sem timeout, se o módulo desconectar/perder alimentação, este while
 * trava para sempre e mata o loop principal — inclusive o envio Bluetooth.
 */
static int32_t hx711_read(uint8_t *error)
{
    *error = 0;
    uint32_t timeout = HAL_GetTick() + HX711_TIMEOUT_MS;

    while (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) == GPIO_PIN_SET)
    {
        if (HAL_GetTick() > timeout)
        {
            *error = 1;
            return 0;
        }
    }

    uint32_t raw = 0;
    for (int i = 0; i < 24; i++)
    {
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
        raw = raw << 1;
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
        if (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) == GPIO_PIN_SET)
            raw |= 1;
    }

    // 25º pulso — configura Canal A Gain 128
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);

    // Sign extension: 24 bits two's complement → int32_t
    if (raw & 0x800000)
        raw |= 0xFF000000;

    return (int32_t)raw;
}

/**
 * Média de N leituras. Ignora amostras com erro de timeout.
 * Se TODAS as amostras falharem, retorna erro.
 */
static int32_t hx711_read_average(int samples, uint8_t *error)
{
    int64_t sum = 0;
    int valid = 0;
    uint8_t err;

    for (int i = 0; i < samples; i++)
    {
        int32_t v = hx711_read(&err);
        if (!err)
        {
            sum += v;
            valid++;
        }
    }

    if (valid == 0)
    {
        *error = 1;
        return 0;
    }

    *error = 0;
    return (int32_t)(sum / valid);
}
/* USER CODE END 0 */

/**
 * CORREÇÃO #2: callback de RX do USART1 (Bluetooth).
 * Acumula bytes até '\r' ou '\n'; se o comando for "TARE", seta a flag
 * que o loop principal usa pra re-zerar o offset. Isso é o que falta pra
 * atender RF14/CT12 (tara via aplicativo).
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rx_byte == '\n' || rx_byte == '\r')
        {
            rx_buffer[rx_index] = '\0';
            if (strncmp(rx_buffer, "TARE", 4) == 0)
                tare_requested = 1;
            rx_index = 0;
        }
        else if (rx_index < sizeof(rx_buffer) - 1)
        {
            rx_buffer[rx_index++] = rx_byte;
        }

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();  // ← USART2 primeiro
    MX_USART1_UART_Init();  // ← USART1 depois

    HAL_Delay(2000);

    // Debug após os dois inits
    HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart1, (uint8_t*)"BOOT\r\n", 6, 1000);
    char dbg[32];
    sprintf(dbg, "USART1 ret=%d\r\n", ret);
    HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 100);
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    int32_t raw_value = 0;
    float peso_kg = 0.0f;
    char buffer[96];
    uint8_t hx_error = 0;

    // Tara automática no boot
    HAL_Delay(500);
    float offset = (float)hx711_read_average(AVG_SAMPLES, &hx_error);

    // Habilita recepção de comandos via Bluetooth (ex.: "TARE\n")
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    /* USER CODE END 2 */

    while (1)
    {
        if (tare_requested)
        {
            offset = (float)hx711_read_average(AVG_SAMPLES, &hx_error);
            tare_requested = 0;
        }

        raw_value = hx711_read_average(AVG_SAMPLES, &hx_error);
        peso_kg = ((float)raw_value - offset) / HX711_SCALE;

        // CORREÇÃO #3: status real, não mais hardcoded "OK"
        const char *status = "OK";
        if (hx_error)
            status = "ERR_HX711";
        else if (raw_value == 8388607 || raw_value == -8388608)
            status = "ERR_SAT"; // saturação — undervoltage

        // CORREÇÃO #4: protocolo sem o campo extra "raw", igual ao documentado
        int n = sprintf(buffer, "{\"gas_kg\":%.2f,\"nivel_pct\":%.0f,\"status\":\"%s\"}\r\n",
                         peso_kg, (peso_kg / 13.0f) * 100.0f, status);

        // Mesmo JSON pros dois lados: o que aparece no CH340 é exatamente
        // o que vai pro Bluetooth.
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, n, 100);
        HAL_UART_Transmit(&huart2, (uint8_t*)buffer, n, 100);

        HAL_Delay(500);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();

    // Necessário pro HAL_UART_Receive_IT funcionar (recepção de "TARE")
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * CH340 debug, agora separado do Bluetooth (USART1).
 * Baud rate 115200 — ajuste se seu terminal serial usar outro valor.
 */
static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    // HX711 — SCK saída push-pull
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = HX711_SCK_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(HX711_SCK_GPIO_Port, &GPIO_InitStruct);

    // HX711 — DT entrada
    GPIO_InitStruct.Pin  = HX711_DT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(HX711_DT_GPIO_Port, &GPIO_InitStruct);


    // PA2 — TX CH340 (AF push-pull)
    GPIO_InitStruct.Pin   = GPIO_PIN_2;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA3 — RX CH340 (entrada flutuante)
    GPIO_InitStruct.Pin  = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
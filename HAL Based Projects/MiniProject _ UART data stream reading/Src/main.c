/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define Buffer_len 2 // Received UART message have a maximum length of 3 (when setting servo-motors speed).
#define ARR_value 255 // TIM2 Auto Reload Register set to . (2^16)

#define FL GPIO_PIN_5 // GPIOA_PIN5 Front Light
#define BL GPIO_PIN_4 // GPIOA_PIN4 Back Light

// Right motor
#define RM_GPIO GPIOC
#define enB TIM_CHANNEL_1
#define in3 GPIO_PIN_1
#define in4 GPIO_PIN_2

// Left motor
#define LM_GPIO GPIOB
#define enA TIM_CHANNEL_2
#define in1 GPIO_PIN_1
#define in2 GPIO_PIN_2

#define Turning_Speed1 60
#define Turning_Speed2 100 // Speed difference is set to 40 in order to perform a smooth turn.

#define TRIG_PIN GPIO_PIN_0
#define TRIG_PORT GPIOC
#define ECHO TIM_CHANNEL_1



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint16_t CNT_Ticks = 0;

uint8_t RX_BUFFER[Buffer_len]={0};
int RX_index;
uint16_t x_speed = 0;
uint32_t x_OCC;
uint32_t IC_Val1 = 0;
uint32_t IC_Val2 = 0;
uint32_t Difference = 0;
uint8_t Is_First_Captured = 0;  // is the first value captured ?
float Distance  = 0.0;
char MSG[35] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *); // Callback function: We are reading from UART in Interrupt mode.
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

uint32_t MAP(uint32_t au32_IN, uint32_t au32_INmax, uint32_t au32_OUTmax); // This is a modified version from DeepBLue Math Library (I have considered a 0 min for both input and output intervals).
void delay_us(uint16_t delay);

void HCSR04_Read(void);
void Front_Light_Toggle();
void Back_Light_Toggle();
void Turn_Left();
void Turn_Right();
void Stop();
float Range_to_obstacle(uint8_t); // Consider transforming this function on a Alert function (cause with the HCSR04_Read direct reading capabilities it becomes useless).
void ServoA_Speed_Calibration(uint16_t);
void ServoB_Speed_Calibration(uint16_t);
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
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim6);

  //Start the PWM mode on the channel of timer1
  HAL_TIM_PWM_Start(&htim2, enA);

  //Start the PWM mode on the channel of timer2
  HAL_TIM_PWM_Start(&htim2, enB);

  //Start timer 3 in input capture mode
  HAL_TIM_IC_Start_IT(&htim3, ECHO);

  //Setup the trigger for the first reading from the UART.
  HAL_UART_Receive_IT(&huart1, RX_BUFFER, Buffer_len);

  //Initialize Right motors direction to forward
  HAL_GPIO_WritePin(RM_GPIO, in3, GPIO_PIN_SET);
  HAL_GPIO_WritePin(RM_GPIO, in4, GPIO_PIN_RESET);

  //Initialize Left motors direction to forward
  HAL_GPIO_WritePin(LM_GPIO, in1, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LM_GPIO, in2, GPIO_PIN_RESET);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	HCSR04_Read();

	sprintf(MSG, "%f cm\r\n", Distance);

	HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 100);

	HAL_Delay(20);

	/*
	switch (RX_BUFFER[0]){

	      case 'M':
	    	  Front_Light_Toggle();
	    	  sprintf(MSG,"Command M: Front_Light_Toggle() \r\n");
	    	  HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 50);
			  break;

	      case 'N':
	    	  sprintf(MSG,"Command N: Back_Light_Toggle() \r\n");
	    	  HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 50);
	    	  Back_Light_Toggle();
	    	  break;

	      case 'L':
	    	  Turn_Left();
	    	  break;

	      case 'R':
	    	  Turn_Right();
	    	  break;

	      case 'U':
	    	  Forword();
	    	  break;

	      case 'D':
	    	  Backword();
	    	  break;

	      case 'E':

	    	break;

	      case 'C':

	        break;

	      case 'Z':

	        break;

	      default :
	    	  //Print a message on the LCD to mention no action is specified.
	    	break;
	    }
	*/

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 36;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 881 ;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 255;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 72-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 72-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 65535;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, TRIG_Pin|IN3_Pin|IN4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, BL_Pin|FL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IN1_Pin|IN2_Pin|ALERT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TRIG_Pin IN3_Pin IN4_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin|IN3_Pin|IN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BL_Pin FL_Pin */
  GPIO_InitStruct.Pin = BL_Pin|FL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin IN2_Pin ALERT_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|IN2_Pin|ALERT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	// Check the Cplt callback function call belongs to the UART1 instance.
    if (huart == &huart1) {
    	HAL_UART_Receive_IT(&huart1, RX_BUFFER, Buffer_len);
    }
}


void Front_Light_Toggle(){
	HAL_GPIO_TogglePin(GPIOA,FL);
}

void Back_Light_Toggle(){
	HAL_GPIO_TogglePin(GPIOA,BL);
}

void Stop(){
	  //Stop Right motors direction to forward
	  HAL_GPIO_WritePin(RM_GPIO, in3, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(RM_GPIO, in4, GPIO_PIN_RESET);

	  //Stop Left motors direction to forward
	  HAL_GPIO_WritePin(LM_GPIO, in1, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(LM_GPIO, in2, GPIO_PIN_RESET);
}

// Code to calibrate the speed of Servo A based on the received speed parameter
void ServoA_Speed_Calibration(uint16_t speed) {

	uint32_t OCC_Value = 0;
    // Use the received speed value to set the PWM duty cycle or pulse width
    // on the pin controlling Servo A (PB1 in this example)
    // Note: You may need to use a PWM library or HAL functions for precise control

	OCC_Value = MAP((uint32_t)speed, ARR_value, 180);
	x_OCC = OCC_Value;
	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1, OCC_Value); // Channel 1 to control MA.
}

// Similar to ServoA_Speed_Calibration, adjust the logic for Servo B (PB2 in this example)
void ServoB_Speed_Calibration(uint16_t speed) {

	uint32_t OCC_Value = 0;
    // Use the received speed value to set the PWM duty cycle or pulse width
    // on the pin controlling Servo B
	// MAp the speed with the available CCR1 range. (This step is crucial to cover the output capture/compare full range).

	OCC_Value = MAP((uint32_t)speed, ARR_value, 180);
	x_OCC = OCC_Value;
	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2, OCC_Value); // Channel 1 to control MA.
}

// Servo-Motor A is attached to the left side.
// Servo-Motor B is attached to the right side.

//Turning left: SPEED_SERVO_B > SPEED_SERVO_A
void Turn_Left(){
	// Set ServoA to Turning_Speed1 and ServoB to Turning_Speed2
	ServoA_Speed_Calibration(Turning_Speed1);
	ServoB_Speed_Calibration(Turning_Speed2);
}

//Turning left: SPEED_SERVO_A > SPEED_SERVO_B
void Turn_Right(){
	// Set ServoB to Turning_Speed1 and ServoA to Turning_Speed2
	ServoB_Speed_Calibration(Turning_Speed1);
	ServoA_Speed_Calibration(Turning_Speed2);
}

void Forword(){
	// Set ServoB to Turning_Speed1 and ServoA to Turning_Speed2
	ServoB_Speed_Calibration(Turning_Speed1);
	ServoA_Speed_Calibration(Turning_Speed2);
}

void Backword(){
	// Set ServoB to Turning_Speed1 and ServoA to Turning_Speed2
	ServoB_Speed_Calibration(Turning_Speed1);
	ServoA_Speed_Calibration(Turning_Speed2);
}




//MAP function to Scale the giving results in order to match CCR Full range.
uint32_t MAP(uint32_t au32_IN, uint32_t au32_INmax, uint32_t au32_OUTmax)
{
    return au32_IN * (au32_OUTmax/au32_INmax);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)  // if the interrupt source is channel1
	{
		if (Is_First_Captured==0) // if the first value is not captured
		{
			IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1); // read the first value
			Is_First_Captured = 1;  // set the first captured as true
			// Now change the polarity to falling edge
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
		}

		else if (Is_First_Captured==1)   // if the first is already captured
		{
			IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);  // read second value
			__HAL_TIM_SET_COUNTER(htim, 0);  // reset the counter

			if (IC_Val2 > IC_Val1)
			{
				Difference = IC_Val2-IC_Val1;
			}

			else if (IC_Val1 > IC_Val2)
			{
				Difference = (0xffff - IC_Val1) + IC_Val2;
			}

			Distance = Difference * .034/2;
			Is_First_Captured = 0; // set it back to false

			// set polarity to rising edge
			__HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
			// To avoid reading next echos (this routine will be executed ones we need an update): refer to HCSR04_Read()
			__HAL_TIM_DISABLE_IT(&htim3, TIM_IT_CC1);
		}
	}
}


/*the function responsible of reading HCSR04 (this function will set the HCSR04 trig pin high for 3 ms):
result will be stored in "distance" global variable handled within the IC callback function
*/
void HCSR04_Read(void)
{
	HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);  // pull the TRIG pin HIGH
	delay_us(5);  // wait for 3s us (minimum trig pulse is 2us)
	HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);  // pull the TRIG pin low

	__HAL_TIM_ENABLE_IT(&htim3, TIM_IT_CC1);
}


/*

void SysTick_CallBack(void)
{
    SysTicks++;
    if(SysTicks == 20) // Each 15msec: we are using the HCSR04 in non blocking mode
    {
    	HCSR04_Read();
    	SysTicks = 0;
    }
}

*/

void delay_us(uint16_t delay){

	TIM6->CNT = 0;
	while (TIM6->CNT < delay);
}

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

#ifdef  USE_FULL_ASSERT
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

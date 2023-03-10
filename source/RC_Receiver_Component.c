#include "RC_Receiver_Component.h"

typedef struct
{
	uint16_t header;
	uint16_t ch1;
	uint16_t ch2;
	uint16_t ch3;
	uint16_t ch4;
	uint16_t ch5;
	uint16_t ch6;
	uint16_t ch7;
	uint16_t ch8;
} RC_Values;

SemaphoreHandle_t rc_hold_semaphore;
TaskHandle_t rc_task_handle;

void setupRCReceiverComponent()
{
	setupRCPins();

	setupUART_RC();

    /*************** RC Task ***************/
	rc_hold_semaphore = xSemaphoreCreateBinary();
	status = xTaskCreate(rcTask, "RC task", 200, NULL, 2, NULL);
	if (status != pdPASS)
	{
		PRINTF("Task creation failed!.\r\n");
		while (1)
			;
	}
}

void setupRCPins()
{
	//Configure RC pins
	CLOCK_EnableClock(kCLOCK_PortC);
	PORT_SetPinMux(PORTC, 3U, kPORT_MuxAlt3);
}

void setupUART_RC()
{
	uart_config_t config;
	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = 115200;
	config.enableTx = false;
	config.enableRx = true;
	UART_Init(RC_UART, &config, CLOCK_GetFreq(kCLOCK_CoreSysClk));
}

void rcTask(void* pvParameters)
{
	//RC task implementation
	uart_config_t config;
	RC_Values rc_values;
	uint8_t* ptr = (uint8_t*) &rc_values;
	UART_GetDefaultConfig(&config);
	config.baudRate_Bps = 115200;
	config.enableTx = false;
	config.enableRx = true;
	UART_Init(UART1, &config, CLOCK_GetFreq(kCLOCK_CoreSysClk));

	DC_Motor_Values dc_motor_values;

	while (1)
	{
		xSemaphoreTake(rc_hold_semaphore, portMAX_DELAY);
		UART_ReadBlocking(UART1, ptr, 1);
		if(*ptr != 0x20)
		continue;
		UART_ReadBlocking(UART1, &ptr[1], sizeof(rc_values) - 1);
		if(rc_values.header == 0x4020)
		{
			printf("Channel 1 = %d\t", rc_values.ch1);
			printf("Channel 2 = %d\t", rc_values.ch2);
			printf("Channel 3 = %d\t", rc_values.ch3);
			printf("Channel 4 = %d\t", rc_values.ch4);
			printf("Channel 5 = %d\t", rc_values.ch5);
			printf("Channel 6 = %d\t", rc_values.ch6);
			printf("Channel 7 = %d\t", rc_values.ch7);
			printf("Channel 8 = %d\r\n", rc_values.ch8);

			// fill dc_motor_values struct
			dc_motor_values.dir = rc_values.ch6;
			dc_motor_values.mode = rc_values.ch7;
			dc_motor_values.speed = rc_values.ch3;

			// push to queues
			// dc motor
			status = xQueueSendToBack(motor_queue, (void*) &dc_motor_values, portMAX_DELAY);
			if (status != pdPASS)
			{
				PRINTF("Queue Send failed!.\r\n");
				while (1);
			}

			// servo motor
			status = xQueueSendToBack(angle_queue, (void*) &rc_values.ch1, portMAX_DELAY);
			if (status != pdPASS)
			{
				PRINTF("Queue Send failed!.\r\n");
				while (1);
			}

			// led
			status = xQueueSendToBack(led_queue, (void*) &rc_values.ch7, portMAX_DELAY);
			if (status != pdPASS)
			{
				PRINTF("Queue Send failed!.\r\n");
				while (1);
			}

		}
	}
}



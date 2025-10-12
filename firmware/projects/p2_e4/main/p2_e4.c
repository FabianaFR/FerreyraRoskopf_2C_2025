/*! @mainpage Guia 2 Actividad 3 - Petracchi, F. M.
 *
 * @section genDesc General Description
 *
 * Se diseñó e implementó una aplicación, basada en el driver analog_io_mcu.h y el driver de transmisión serie uart_mcu.h, que digitaliza una señal
 * analógica y la transmite a un graficador de puerto serie de la PC. Se toma la entrada CH1 del conversor AD y la transmisión se
 * realizó por la UART conectada al puerto serie de la PC, en un formato compatible con un graficador por puerto serie.
 *
 * Se convierte una señal digital de un ECG (provista por la cátedra) en una señal analógica y se puede visualizar esta señal utilizando el osciloscopio
 * implementado. Se sugiere utilizar el potenciómetro para conectar la salida del DAC a la entrada CH1 del AD.
 *
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|   Description                 |
 * |:--------------:|:-------------:|:------------------------------|
 * | Analog Input 	| 	GPIO_01		|   ADC_CH1 (Oscilloscope)      |
 * | PWM DAC Output	| 	GPIO_00		|   DAC_CH0	(Signal generator)  |
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 13/10/2025 | Document creation		                         |
 *
 * @author Fabiana F. Roskopf (fabianafroskopf@gmail.com)
 *
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define ADC_CHANNEL CH1

#define UART_PORT UART_PC

#define UART_BAUDRATE 115200

#define TIMER_PERIOD_US 10 * 1000

#define SIGNAL_SIZE 231

#define DEBUG_MODE 0 // 1: Enable debug mode, 0: Disable debug mode

/*==================[internal data definition]===============================*/
/*
 * @brief Task handles para la tarea analogReadAndSena
 * @note These handles son usadas para gestionar las tareas en FreeRTOS.
 */
TaskHandle_t analogReadAndSendTask_handle = NULL;
/*
 * @brief Task handles para la tarea analogWrite
 * @note These handles son usadas para gestionar las tareas en FreeRTOS.
 */
TaskHandle_t analogWriteTask_handle = NULL;
/*
 * @brief Variable para almacenar el valor analógico leído del ADC
 */
uint16_t analog_value = 0;
/*
 * @brief Variable para almacenar el valor de la señal analógica a enviar
 * @note Esta variable se usa para almacenar el valor de la señal analógica que se enviará al DAC.
 */
uint16_t signal_value = 0; // Variable to store the value of the ADC
/*
 * @brief Variable para almacenar el índice de la muestra de la señal
 * @note Esta variable se usa para llevar un registro del índice de la muestra actual de la señal.
 */
uint16_t sample_index = 0; // Index counter of Signal sample number

const char test_signal[SIGNAL_SIZE] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
    161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
    221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231};

const char ecg[SIGNAL_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76};

const char otro_ecg[256] = {
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 17, 17,
    16, 16, 16, 16, 17, 17, 18, 18, 18, 17, 17, 17, 17, 18, 18, 19, 21, 22, 24, 25, 26, 27, 28, 29, 31, 32, 33, 34, 34, 35,
    37, 38, 37, 34, 29, 24, 19, 15, 14, 15, 16, 17, 17, 17, 16, 15, 14, 13, 13, 13, 13, 13, 13, 13, 12, 12, 10, 6, 2, 3,
    15, 43, 88, 145, 199, 237, 252, 242, 211, 167, 117, 70, 35, 16, 14, 22, 32, 38, 37, 32, 27, 24, 24, 26, 27, 28, 28, 27, 28, 28,
    30, 31, 31, 31, 32, 33, 34, 36, 38, 39, 40, 41, 42, 43, 45, 47, 49, 51, 53, 55, 57, 60, 62, 65, 68, 71, 75, 79, 83, 87,
    92, 97, 101, 106, 111, 116, 121, 125, 129, 133, 136, 138, 139, 140, 140, 139, 137, 133, 129, 123, 117, 109, 101,
    92, 84, 77, 70, 64, 58, 52, 47, 42, 39, 36, 34, 31, 30, 28, 27, 26, 25, 25, 25, 25, 25, 25, 25, 25, 24, 24, 24, 24, 25,
    25, 25, 25, 25, 25, 25, 24, 24, 24, 24, 24, 24, 24, 24, 23, 23, 22, 22, 21, 21, 21, 20, 20, 20, 20, 20, 19, 19, 18, 18,
    18, 19, 19, 19, 19, 18, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17};

/*==================[internal functions declaration]=========================*/
/*
 * @brief funcion que se ejecuta cuando el timer A genera una interrupción
 * @note Esta función se encarga de notificar a la tarea analogReadAndSendTask
 */
static void analogReadAndSend(void *param)
{
    vTaskNotifyGiveFromISR(analogReadAndSendTask_handle, pdFALSE);
}

/*
 * @brief Tarea que lee el valor analógico del ADC y lo envía por UART
 * @note Esta tarea se ejecuta periódicamente y lee el valor analógico del ADC, luego lo envía por UART en un formato específico.
 */
static void analogReadAndSendTask(void *param)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        AnalogInputReadSingle(ADC_CHANNEL, &analog_value);

        UartSendString(UART_PORT, ">");
        // UartSendString(UART_PORT, "min:0,");
        // UartSendString(UART_PORT, "max:3345,");
        UartSendString(UART_PORT, "analog_voltage:");
        UartSendString(UART_PORT, (char *)UartItoa(analog_value, 10));

        UartSendString(UART_PORT, "\r\n");
    }
}
/*
 * @brief Función que muestrea la señal analógica
 * @note Esta función se encarga de muestrear la señal analógica y devolver el valor correspondiente.
 */
char sampleSignal(void)
{
    signal_value = otro_ecg[sample_index];
#if DEBUG_MODE
    printf("sample_signal [%d]: %d\n", sample_index, signal_value);
#endif
    if (sample_index < (sizeof(otro_ecg) / sizeof(otro_ecg[0])) - 1)
    {
        sample_index++;
    }
    else
    {
        sample_index = 0;
    }
    return ((uint16_t)signal_value);
}
/*
 * @brief Función que se ejecuta cuando el timer B genera una interrupción
 * @note Esta función se encarga de notificar a la tarea analogWriteTask
 */
static void analogWrite(void *param)
{
    vTaskNotifyGiveFromISR(analogWriteTask_handle, pdFALSE);
}
/*
 * @brief Tarea que escribe el valor analógico en el DAC
 * @note Esta tarea se ejecuta periódicamente y escribe el valor analógico en el DAC.
 */
static void analogWriteTask(void *param)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        AnalogOutputWrite(sampleSignal()); // sampleSignal()
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    /* Configurar entrada analógica CH1 en modo simple */
    analog_input_config_t adc_config = {
        .input = ADC_CHANNEL,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0 // No usado en modo simple
    };
    AnalogInputInit(&adc_config);

    AnalogOutputInit();

    serial_config_t uart_config = {
        .port = UART_PORT,
        .baud_rate = UART_BAUDRATE,
        .func_p = NULL,
        .param_p = NULL};
    UartInit(&uart_config);
    /* Configurar timer para muestreo periódico */
    timer_config_t analogReadAndSendTimer_config = {
        .timer = TIMER_A,
        .period = TIMER_PERIOD_US,
        .func_p = analogReadAndSend,
        .param_p = NULL};
    /* Configurar timer para escritura periódica */
    timer_config_t analogWriteTimer_config = {
        .timer = TIMER_B,
        .period = TIMER_PERIOD_US,
        .func_p = analogWrite,
        .param_p = NULL};
    xTaskCreate(analogWriteTask, "AnalogWrite", 2048, NULL, 5, &analogWriteTask_handle);
    xTaskCreate(analogReadAndSendTask, "AnalogReadAndSend", 2048, NULL, 5, &analogReadAndSendTask_handle);

    TimerInit(&analogWriteTimer_config);
    TimerStart(analogWriteTimer_config.timer);
    TimerInit(&analogReadAndSendTimer_config);
    TimerStart(analogReadAndSendTimer_config.timer);
}
/*==================[end of file]============================================*/
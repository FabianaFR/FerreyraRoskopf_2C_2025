/*! @mainpage Guia 2 Actividad 3 - Petracchi, F. M.
 *
 * @section genDesc General Description
 *
 * Se diseñó el firmware de manera que cumple con las siguientes funcionalidades:
 * <ol>
 * 		<li>Mostrar distancia medida utilizando los leds de la siguiente manera: </li>
 * - Si la distancia es menor a 10 cm, apagar todos los LEDs.
 * - Si la distancia está entre 10 y 20 cm, encender el LED_1.
 * - Si la distancia está entre 20 y 30 cm, encender el LED_2 y LED_1.
 * - Si la distancia es mayor a 30 cm, encender el LED_3, LED_2 y LED_1.
 * 		<li>Mostrar el valor de distancia en cm utilizando el display LCD. </li>
 * 		<li>Usar TEC1 para activar y detener la medición. </li>
 * 		<li>Usar TEC2 para mantener el resultado (“HOLD”): sin pausar la medición. </li>
 * 		<li>Refresco de medición: 1 s (1000 ms). </li>
 *
 * Además debe ser posible controlar la EDU-ESP de la siguiente manera:
 *  - Con las teclas “O” y “H”, replicar la funcionalidad de las teclas 1 y 2 de la EDU-ESP.
 *  - Usar “I” para cambiar la unidad de trabajo de "cm" a "pulgadas".
 *  - Usar “M” para implementar la visualización del máximo.
 *  - Usar “F” para aumentar la velocidad de lectura, reduciendo de a 100 milisegundos el tiempo de lectura.
 *  - Usar “S” para disminuir la velocidad de lectura, aumentando de a 100 milisegundos el tiempo de lectura.
 *
 * <a href="https://drive.google.com/file/d/1yIPn12GYl-s8fiDQC3_fr2C4CjTvfixg/view"> Example operation </a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  | 	ESP32-C6	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIGGER	 	| 	GPIO_2		|
 * | 	Vcc		 	| 	+5V			|
 * | 	GND		 	| 	GND			|
 * |  LED_1 (GREEN)	| 	GPIO_11		|
 * |  LED_2	(YELL.)	| 	GPIO_10		|
 * |  LED_3 (RED) 	| 	GPIO_5		|
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
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
/**
 * @def CONFIG_MEASURE_PERIOD
 * @brief Define la tasa de refresco de la tarea de medición y muestra.
 * @details 1000 ms
 */
#define CONFIG_MEASURE_PERIOD 1000 * 1000    // ms entre mediciones
#define CONFIG_READING_PERIOD 20             // ms entre lecturas de teclas
#define TIEMPO_DE_LECTURA_MINIMO 100 * 1000  // 100 ms
#define TIEMPO_DE_LECTURA_MAXIMO 2000 * 1000 // 2000 ms
#define TIEMPO_DE_LECTURA_STEP 100           // 100 ms

#define MIN_DIST 10 // cm
#define MED_DIST 20
#define MAX_DIST 30
/*==================[internal data declaration]==============================*/
void Medir_MostrarPantalla(void *pvParameter);
void leer_teclado(void *param);

/*==================[internal data definition]===============================*/
/**
 * @brief identificador de tipo TaskHandle_t de la tarea "Medir_MostrarPantalla".
 * @details Se utiliza para la creación de la tarea y para su control.
 */
TaskHandle_t Medir_MostrarPantalla_task_handle = NULL;

/**
 * @brief Flag que indica si se debe medir o no.
 * @details Se utiliza para habilitar o deshabilitar la medición.
 */
bool medir = true;
/**
 * @brief Flag que indica si se debe mantener mostrando un resultado o no.
 * @details Se utiliza para habilitar o deshabilitar la modificación de los LEDs y valores mostrados en el LCD.
 */
bool hold = false;

bool pulgadas = false;

timer_config_t timer_medir = {
    .timer = TIMER_A,
    .period = CONFIG_MEASURE_PERIOD,
    .func_p = Medir_MostrarPantalla,
    .param_p = NULL,
};

/**
 * @brief Estructura de configuración del UART.
 */
serial_config_t my_uart = {
    .port = UART_PC,        // usa puerto hacia PC
    .baud_rate = 19200,     // velocidad
    .func_p = leer_teclado, // sin interrupciones, vamos a leer en una tarea
    .param_p = NULL,
};

/*==================[internal functions declaration]=========================*/

void encenderLedSegunDistancia(uint32_t distance)
{
    if (distance < MIN_DIST)
    { // <10 cm
        LedsOffAll();
    }
    else if (distance >= MIN_DIST && distance < MED_DIST)
    { // 10–20 cm
        LedOn(LED_1);
        LedOff(LED_2);
        LedOff(LED_3);
    }
    else if (distance >= MED_DIST && distance < MAX_DIST)
    { // 20–30 cm
        LedOn(LED_1);
        LedOn(LED_2);
        LedOff(LED_3);
    }
    else if (distance >= MAX_DIST)
    { // >30 cm
        LedOn(LED_1);
        LedOn(LED_2);
        LedOn(LED_3);
    }
}

void Medir_MostrarPantalla(void *pvParameter)
{
    vTaskNotifyGiveFromISR(Medir_MostrarPantalla_task_handle, pdFALSE);
}

void tecla_1()
{
    medir = !medir; // toggle medir
    if (medir == false)
    {
        LedsOffAll();
        LcdItsE0803Off(); // limpiar pantalla
    }
}

void tecla_2()
{
    hold = !hold; // toggle hold
}

void leer_teclado(void *param)
{
    uint8_t letra;
    UartReadByte(UART_PC, &letra);
    UartSendByte(UART_PC, (char *)&letra); // eco
    switch (letra)
    {
    case 'O':
        tecla_1();
        break;
    case 'H':
        tecla_2();
        break;
    case 'I':
        pulgadas = !pulgadas; // toggle pulgadas
        break;
    case 'M':
        break;
    case 'F':
        if (timer_medir.period > TIEMPO_DE_LECTURA_MINIMO)
        {
            timer_medir.period -= TIEMPO_DE_LECTURA_STEP;
            TimerUpdatePeriod(timer_medir.timer, timer_medir.period);
        }
        break;
    case 'S':
        if (timer_medir.period < TIEMPO_DE_LECTURA_MAXIMO)
        {
            timer_medir.period += TIEMPO_DE_LECTURA_STEP;
            TimerUpdatePeriod(timer_medir.timer, timer_medir.period);
        }
        break;
    default:
        break;
    }
}

void mandar_distancia(uint16_t distancia)
{
    // 1. Enviar el texto "Distancia: "
    UartSendString(UART_PC, "Distancia: ");

    // 2. Convertir número a cadena (UartItoa devuelve un puntero a un buffer con el número)
    const char *numero = UartItoa(distancia, 10);

    // 3. Enviar el número convertido
    UartSendString(UART_PC, numero);

    // 4. Enviar la unidad
    if (pulgadas == true)
    {
        UartSendString(UART_PC, " in\r\n");
    }
    else
    {
        UartSendString(UART_PC, " cm\r\n");
    }
}

/*==================[tasks definition]=======================================*/
static void Medir_MostrarPantalla_Task(void *pvParameter)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint16_t distancia = 0;
        if (medir == true && pulgadas == false)
        {
            distancia = HcSr04ReadDistanceInCentimeters(); // medir distancia centrimetros
            mandar_distancia(distancia);
        }
        else if (medir == true && pulgadas == true)
        {                                             // 10–20 cm
            distancia = HcSr04ReadDistanceInInches(); // medir distancia pulgadas
            mandar_distancia(distancia);
        }

        if (hold == false && medir == true)
        {
            encenderLedSegunDistancia(distancia);
            LcdItsE0803Write(distancia);
        }
        else if (hold == true && medir == true)
        {
            // mantener la última medición, no hacer nada
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    LedsInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2);
    SwitchesInit();
    TimerInit(&timer_medir);
    UartInit(&my_uart);

    SwitchActivInt(SWITCH_1, tecla_1, NULL);
    SwitchActivInt(SWITCH_2, tecla_2, NULL);

    xTaskCreate(&Medir_MostrarPantalla_Task, "MEDIR_Y_MOSTRAR", 2048, NULL, 5, &Medir_MostrarPantalla_task_handle);

    TimerStart(timer_medir.timer);
}

/*==================[end of file]============================================*/

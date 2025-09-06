/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * Este fichero contiene funciones para:
 *  - convertir un entero a arreglo BCD,
 *  - mapear un dígito BCD a 4 pines GPIO (b0..b3),
 *  - multiplexar y mostrar un número en un display de 3 dígitos.
 *
 * Requiere la capa de abstracción GPIO: GPIOInit(), GPIOOn(), GPIOOff().
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Fabiana F Roskopf (fabianafroskopf@gmail.com)
 *
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
#define numberToDisplay 127 /** @brief Número que se mostrará por defecto en el ejemplo. */
#define LCDDigits 3			/** @brief Cantidad de dígitos física del LCD (multiplexado). */
#define nBits 4				/** @brief Cantidad de BITs del numero a codificar, en este caso 4 por ser BCD. */

/*==================[internal data definition]===============================*/
/**
 * @brief Configuración de un GPIO.
 *
 * Representa un pin y su dirección. Usado tanto para las líneas de datos (b0..b3)
 * como para los selectores de dígito.
 *
 * @var gpioConf_t::pin  Número/identificador del pin GPIO (tipo gpio_t).
 * @var gpioConf_t::dir  Dirección: '0' = IN, '1' = OUT (tipo io_t).
 */
typedef struct // estructura para definir los pines gpios
{
	gpio_t pin; // numero de pin
	io_t dir;	// direcciones gpio '0' IN;  '1' OUT
} gpioConf_t;	// guardo la estructura en una variable

/*==================[internal functions declaration]=========================*/
/**
 * @brief Convierte un entero de 32 bits en un arreglo de dígitos decimales (BCD).
 *
 * @param[in]  data       Valor a convertir (ej. 127).
 * @param[in]  digits     Número de dígitos que se requieren (ej. 3).
 * @param[out] bcd_number Puntero a un arreglo ya reservado donde se guardarán
 *                        los dígitos. El dígito más significativo se coloca en
 *                        bcd_number[0].
 * @return 0 si la conversión fue correcta, -1 si hubo error (p. ej. bcd_number == NULL).
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

/**
 * @brief Establece el estado de 4 pines GPIO a partir de un dígito BCD (0..9).
 *
 * Recorre los 4 bits (b0..b3) y pone cada GPIO correspondiente en ON/OFF.
 *
 * @param[in] bcd_digit  Dígito decimal (0..9) que se representará en BCD.
 * @param[in] gpio_array Puntero a un arreglo de 4 elementos gpioConf_t donde
 *                       gpio_array[0] corresponde a b0, gpio_array[1] a b1, etc.
 */
void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array);

/**
 * @brief Muestra un número en un display multiplexado.
 *
 * Convierte el número a BCD (llenando un arreglo local), luego para cada dígito:
 *  - pone los 4 bits en las líneas de datos (gpio_array),
 *  - activa el selector del dígito correspondiente (digit_array[i]) durante un breve periodo.
 *
 * @param[in] data        Número entero a mostrar.
 * @param[in] digits      Cantidad de dígitos a mostrar.
 * @param[in] gpio_array  Arreglo que mapea bits b0..b3 a GPIOs (4 elementos).
 * @param[in] digit_array Arreglo que mapea dígitos físicos a GPIOs (digits elementos).
 */
void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array);

/*==================[external functions definition]==========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	// digits -1 inicializa con el ultimo digito del arreglo, se recorre el
	// arerglode atrás para adelante hasta llegar a 0, se le resta 1 en cada vuelta
	for (int i = digits - 1; i >= 0; i--)
	{
		bcd_number[i] = data % 10; /* extrae el último dígito decimal */
		data /= 10;				   /* descarta ese dígito */
	}
	return 0;
}

// vector de pines de datos (son las líneas de datos b0 a b3 que llevan el numero bcd)
/**
 * @brief Mapa de pines que corresponden a los bits b0..b3.
 *
 * gpio_map[0] -> b0 -> GPIO_20
 * gpio_map[1] -> b1 -> GPIO_21
 * gpio_map[2] -> b2 -> GPIO_22
 * gpio_map[3] -> b3 -> GPIO_23
 */
gpioConf_t gpio_map[nBits] = {
	{GPIO_20, 1}, /* b0 */
	{GPIO_21, 1}, /* b1 */
	{GPIO_22, 1}, /* b2 */
	{GPIO_23, 1}  /* b3 */
};

// vector para los pines de seleccion de digito
/**
 * @brief Pines de selección de dígitos (digit multiplexing).
 *
 * digit_map[0] -> dígito 1 -> GPIO_19
 * digit_map[1] -> dígito 2 -> GPIO_18
 * digit_map[2] -> dígito 3 -> GPIO_9
 */
gpioConf_t digit_map[LCDDigits] = {
	{GPIO_19, 1}, // digito 1 (centenas)
	{GPIO_18, 1}, // digito 2 (decenas)
	{GPIO_9, 1}	  // digito 3 (unidades)
};

// funcion para poner los bits del bcd en los gpios
void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array)
{
	/* Inicializo las salidas — opcional si ya se inicializó en app_main */
	for (int i = 0; i < nBits; i++)
	{
		GPIOInit(gpio_array[i].pin, gpio_array[i].dir);
	}

	/* Recorro cada bit del dígito y actualizo el pin correspondiente */
	for (int i = 0; i < nBits; i++)
	{ // con (bcd_digit >> i) & 0001 corro el numero i lugares a la derecha
		// y hago una mascara para quedarme solo con el bit que me interesa
		uint8_t bit_val = (bcd_digit >> i) & 0001;
		if (bit_val == 1)
		{
			GPIOOn(gpio_array[i].pin);
			printf("GPIO ON %d\n", gpio_array[i].pin);
		}
		else
		{
			GPIOOff(gpio_array[i].pin);
			printf("GPIO OFF %d\n", gpio_array[i].pin);
		}
	}
}

// convierte el numero dado en un arreglo
void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array)
{
	/* bcd_digits es un VLA: tamaño depende de 'digits' (C99) */
	uint8_t bcd_digits[digits]; // vector llamado bcd_digits
	// cada casilla del vector es un numero entero de 8 bits
	/* 1) convertimos el número a un arreglo de dígitos */
	convertToBcdArray(data, digits, bcd_digits); // uso la funcion para llenar el arreglo (vector) con los digitos bcd

	// recorro los pines y los voy apagando con gpiooff
	/* 2) apagar todos los selectores de dígito para evitar ghosting */
	for (int j = 0; j < digits; j++)
	{ // donde digit_array es como digit_map
		GPIOInit(digit_array[j].pin, digit_array[j].dir);
		GPIOOff(digit_array[j].pin);
		printf("GPIO OFF %d\n", digit_array[j].pin);
	}

	/* 3) multiplexado: para cada dígito cargar datos, encender selector, delay, apagar selector */
	for (int i = 0; i < digits; i++) // recorro cada digito
	{
		// cargo el valor BCD en las líneas de datos gpio 20 a 23
		/* Encender el dígito i (mostrarlo) */
		setGpioFromBcd(bcd_digits[i], gpio_array);
		GPIOOn(digit_array[i].pin);
		/* Apagar el dígito antes de pasar al siguiente */
		GPIOOff(digit_array[i].pin);
		printf("GPIO ON %d\n", digit_array[i].pin);
		// se multiplexean los digitos ya que se seleccionan y muestran de a uno
		// como el for se recorre tan rapido, en el display se ve como si los 3 estuvieran prendidos al mismo tiempo
	}
}

void app_main(void)
{
	// inicializo todos los pines gpios
	/* Inicializo pines de datos y selectores (una sola vez) */
	for (int i = 0; i < nBits; i++)
	{
		GPIOInit(gpio_map[i].pin, gpio_map[i].dir);
	}
	for (int i = 0; i < LCDDigits; i++)
	{
		GPIOInit(digit_map[i].pin, digit_map[i].dir);
	}

	uint32_t numero = numberToDisplay;
	uint8_t digitos = LCDDigits;

	// pa mostrar el numero
	/* Mostrar el número (una pasada; para que se vea continuamente replicar en bucle) */
	displayNumber(numero, digitos, gpio_map, digit_map);
}
/*==================[end of file]============================================*/
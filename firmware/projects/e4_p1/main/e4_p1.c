/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
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
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
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
#define numberToDisplay 127
#define LCDDigits 3
#define nBits 4
/*==================[internal data definition]===============================*/
typedef struct // estructura para definir los pines gpios
{
	gpio_t pin; // numero de pin
	io_t dir;	//< GPIO direction '0' IN;  '1' OUT
} gpioConf_t;	// guardo la estructura en una variable

/*==================[internal functions declaration]=========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);
void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array);
void displayNumber(uint32_t data, uint8_t digits, gpioConf_t *gpio_array, gpioConf_t *digit_array);

/*==================[external functions definition]==========================*/
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	// digits -1 inicializa con el ultimo digito del arreglo, se recorre el
	// arerglode atrás para adelante hasta llegar a 0, se le resta 1 en cada vuelta
	for (int i = digits - 1; i >= 0; i--)
	{
		bcd_number[i] = data % 10; // tomo el ultimo digito (en la division quedo con el resto)
		data /= 10;				   // lo elimino, y como es de tipo unit32, es un entero
	}
	return 0;
}

// vector de pines de datos (son las líneas de datos b0 a b3 que llevan el numero bcd)
gpioConf_t gpio_map[nBits] = {
	{GPIO_20, 1}, // b0
	{GPIO_21, 1}, // b1
	{GPIO_22, 1}, // b2
	{GPIO_23, 1}  // b3
};

// vector para los pines de seleccion de digito
gpioConf_t digit_map[LCDDigits] = {
	{GPIO_19, 1}, // digito 1 (centenas)
	{GPIO_18, 1}, // digito 2 (decenas)
	{GPIO_9, 1}	  // digito 3 (unidades)
};

// funcion para poner los bits del bcd en los gpios
void setGpioFromBcd(uint8_t bcd_digit, gpioConf_t *gpio_array)
{
	for (int i = 0; i < nBits; i++)
	{
		GPIOInit(gpio_array[i].pin, gpio_array[i].dir);
	}

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
	uint8_t bcd_digits[digits]; // vector llamado bcd_digits
	// cada casilla del vector es un numero entero de 8 bits
	convertToBcdArray(data, digits, bcd_digits); // uso la funcion para llenar el arreglo (vector) con los digitos bcd

	// recorro los pines y los voy apagando con gpiooff
	for (int j = 0; j < digits; j++)
	{ // donde digit_array es como digit_map
		GPIOInit(digit_array[j].pin, digit_array[j].dir);
		GPIOOff(digit_array[j].pin);
		printf("GPIO OFF %d\n", digit_array[j].pin);
	}

	for (int i = 0; i < digits; i++) // recorro cada digito
	{
		// cargo el valor BCD en las líneas de datos gpio 20 a 23
		setGpioFromBcd(bcd_digits[i], gpio_array);

		// activo el digito que corresponde
		GPIOOn(digit_array[i].pin);
		GPIOOff(digit_array[i].pin);
		printf("GPIO ON %d\n", digit_array[i].pin);
		// se multiplexean los digitos ya que se seleccionan y muestran de a uno
		// como el for se recorre tan rapido, en el display se ve como si los 3 estuvieran prendidos al mismo tiempo
	}
}

void app_main(void)
{
	// inicializo todos los pines gpios
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
	displayNumber(numero, digitos, gpio_map, digit_map);
}
/*==================[end of file]============================================*/
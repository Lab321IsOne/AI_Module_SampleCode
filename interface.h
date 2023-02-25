/** InstAI Co. (Public Version)
    Description: Platform GPIO (digital & SPI) API implementation
    Modified Date: Feb 03, 2023
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

// uncomment the following line if your host platform is Raspberry Pi
//#define PLATFORM_RASPI
// uncomment the following line if your host platform is Arduino
#define PLATFORM_ARDUINO

/** include the GPIO library to perform GPIO operations on your platform */
#ifdef PLATFORM_RASPI
    #include <bcm2835.h>

    // define SPI specification
    #define SPI_MODE        BCM2835_SPI_MODE3
    #define SPI_CLK_SPEED   2000000     // set spi transmission speed as 2 MHz

    // define the basic GPIO function on the platform Raspberry Pi
    #define interface_gpio_input(pin)               bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT)
    #define interface_gpio_output(pin)              bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP)
    #define interface_digital_read(pin)             (bcm2835_gpio_lev(pin) & 0x01)
    #define interface_digital_write(pin, level)     bcm2835_gpio_write(pin, level & 0x01);

#elif defined PLATFORM_ARDUINO
    #include <Arduino.h>
    #include <SPI.h>

    // define SPI specification
    #define SPI_MODE        SPI_MODE3
    #define SPI_CLK_SPEED   18000000      // set spi transmission speed as 18 MHz

    // define the basic GPIO function on the platform Arduino
    #define interface_gpio_input(pin)               pinMode(pin, INPUT)
    #define interface_gpio_output(pin)              pinMode(pin, OUTPUT)
    #define interface_digital_read(pin)             (digitalRead(pin) & 0x01)
    #define interface_digital_write(pin, level)     digitalWrite(pin, level & 0x01)

#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif // PLATFORM_RASPI
/***/


/**
    @brief initialize SPI pins on your platform
    @param
        (NONE)
        on Arduino: pass SPIClass object which has been initialized
    @return
        return true if GPIO and SPI interface initialized successfully
        otherwise, return false

    @warning
        on Arduino: this function must be called after SPI.begin()
    @remark
        on Arduino: if you're using Arduino platform, you can either
            pass the initialized SPIClass object pointer, or pass
            NULL to use default SPI object
*/
#ifdef PLATFORM_RASPI
bool interface_spi_init();
#elif defined PLATFORM_ARDUINO
bool interface_spi_init(SPIClass *spi_class);
#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif


/**
    @brief write address and data to AI module through SPI
    @param
        pin_cs: specify digital pin number of AI Module's SPI chip select Pin
        address: 8-bit address value of AI module
        data: 8-bit data value to AI module
    @return
        (NONE)
*/
void interface_spi_write(uint8_t pin_cs, uint8_t address, uint8_t data);
/**
    @brief read data value from specified address of AI module through SPI
    @param
        pin_cs: specify digital pin number of AI Module's SPI chip select Pin
        address: 8-bit address value of AI module
    @return
        8-bit retrieved data value on specified address of AI module
*/
uint8_t interface_spi_read(uint8_t pin_cs, uint8_t address);

#endif  // INTERFACE_H

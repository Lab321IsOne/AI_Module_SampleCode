/** InstAI Co. (Public Version)
    Description: Platform GPIO (digital & SPI) API implementation
    Modified Date: Feb 03, 2023
*/
#include "interface.h"

#ifdef PLATFORM_ARDUINO
static SPIClass *_spi = NULL;
#endif

// initialize SPI Interface with settings SPI Mode = 3, SPI clock speed < 20 MHz
#ifdef PLATFORM_RASPI
bool interface_spi_init()
{
    if(bcm2835_init() == 0)
        return false;

    if(bcm2835_spi_begin() == 0)
        return false;

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    uint16_t speed_div = bcm2835_aux_spi_CalcClockDivider(SPI_CLK_SPEED);
    bcm2835_spi_setClockDivider(speed_div);

    return true;
}
#elif defined PLATFORM_ARDUINO
bool interface_spi_init(SPIClass *spi_class)
{
    // initialize your SPI class with the following configurations before calling this function:
    /**
     * SPI.begin(); // put custom SPI pin numbers inside of the function
    */
    _spi = spi_class;
    return true;
}
#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif

void interface_spi_write(uint8_t pin_cs, uint8_t address, uint8_t data)
{   // pull low CS pin, transfer 8-bit address & 8-bit data on MOSI, pull high CS pin
    
    /** SPI data transfer */
#ifdef PLATFORM_RASPI
    interface_digital_write(pin_cs, LOW);
        bcm2835_spi_transfer(address);
        bcm2835_spi_transfer(data);
    interface_digital_write(pin_cs, HIGH);

#elif defined PLATFORM_ARDUINO
    if(_spi == NULL)
    {   // use the default SPI object
        SPI.beginTransaction(SPISettings(SPI_CLK_SPEED, MSBFIRST, SPI_MODE));
        interface_digital_write(pin_cs, LOW);
            SPI.transfer(address);
            SPI.transfer(data);
        SPI.endTransaction();
        interface_digital_write(pin_cs, HIGH);
    }
    else
    {   // use the custom SPIClass object
        _spi->beginTransaction(SPISettings(SPI_CLK_SPEED, MSBFIRST, SPI_MODE));
        interface_digital_write(pin_cs, LOW);
            _spi->transfer(address);
            _spi->transfer(data);
        _spi->endTransaction();
        interface_digital_write(pin_cs, HIGH);
    }
#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif // PLATFORM_RASPI
    /***/
}

uint8_t interface_spi_read(uint8_t pin_cs, uint8_t address)
{   // pull low CS pin, transfer 8-bit address with MSB HIGH, record response data on MISO, pull high CS pin
    // and return the response 8-bit data from this function
    
    /** SPI data transfer and read */
    uint8_t val = (1 << 7) | address;   // ensure to make MSB HIGH when reading
#ifdef PLATFORM_RASPI
    interface_digital_write(pin_cs, LOW);
        bcm2835_spi_transfer(val);
        val = bcm2835_spi_transfer(0);
    interface_digital_write(pin_cs, HIGH);
#elif defined PLATFORM_ARDUINO
    if(_spi == NULL)
    {   // use the default SPI object
        SPI.beginTransaction(SPISettings(SPI_CLK_SPEED, MSBFIRST, SPI_MODE));
        interface_digital_write(pin_cs, LOW);
            SPI.transfer(val);
            val = SPI.transfer(0);
        SPI.endTransaction();
        interface_digital_write(pin_cs, HIGH);
    }
    else
    {   // use the custom SPIClass object
        _spi->beginTransaction(SPISettings(SPI_CLK_SPEED, MSBFIRST, SPI_MODE));
        interface_digital_write(pin_cs, LOW);
            _spi->transfer(val);
            val = _spi->transfer(0);
        _spi->endTransaction();
        interface_digital_write(pin_cs, HIGH);
    }
#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif // PLATFORM_RASPI
    /***/
    return val;
}

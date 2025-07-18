#ifndef ADC_H
#define ADC_H

// Class for handling the MCP3564 24 bit - 4 channel ADC:
// https://www.mouser.de/datasheet/2/268/MCP3561_2_4_Family_Data_Sheet_DS20006181C-2257924.pdf
// developed by Tobias Gomer

/* TIMER REGISTER */
const uint8_t _TIMER_ = 0x08;                 // TIMER Register Address.
    // TIMER Settings
    #define TIMER_SCAN 0x000000               // Set TIMER_SCAN value according to Register 8-9 definition in the MCP3x6x(R) DS.
    #define TIMER_CFG (0x00000000)
/* OFFSETCAL REGISTER */
const uint8_t _OFFSETCAL_ = 0x09;             // OFFSETCAL Register Address.
    // OFFSETCAL Settings
    #define OFFSETCAL_CFG (0x0000003E)        // Set OFFSETCAL value according to Register 8-10 definition in the MCP3x6x(R) DS.
/* GAINCAL REGISTER */
const uint8_t _GAINCAL_ = 0x0A;               // GAINCAL Register Address.
    // GAINCAL Settings
    #define GAINCAL_CFG (0x007DEE80)          // Set GAINCAL value according to Register 8-11 definition in the MCP3x6x(R) DS.
/* RESERVED REGISTER */
const uint8_t _RESERVED_B_ = 0x0B;            // Reserved B Register Address.
    // RESERVED B Setting
    #define RESERVED_B_CFG (0x900000)         // Reserved B Register default value is 0x900000 according to Register 8-12 definition in the MCP3x6x(R) DS.
/* RESERVED REGISTER */
const uint8_t _RESERVED_C_ = 0x0C;            // Reserved C Register Address.
    // RESERVED C Setting
    #define RESERVED_C_CFG (0x50)             // Resereved C Register default value is 0x50 according to Register 8-13 definition in the MCP3x6x(R) DS.
/* LOCK REGISTER */
const uint8_t _LOCK_ = 0x0D;                  // LOCK Register Address.
    // LOCK Settings
    #define LOCKED 0b00000000                 // LOCK Register <> A5 "Locks' register-write access.
    #define UNLOCKED 0b10100101               // LOCK Register = A5 "Un-Locks' register-write access.
    #define LOCK_CFG (UNLOCKED)               // Set LOCK value to "Un-Lock" for register-write ccess.
/* RESERVED REGISTER */
const uint8_t _RESERVED_E_ = 0x0E;            // Reserved E Register Address.
    // RESERVED E Setting
    #define RESERVED_E_CFG (0x000F)           // Resereved E Register default value is 0x000F (MCP3564) according to Register 8-15 definition in the MCP3x6x(R) DS.
/* CRCCFG REGISTER */
const uint8_t _CRCCFG_ = 0x0F;                // CRCCFG Register Address.
    // CRCCFG Settings
    #define CRCCFG_CFG (0x0000)               // Register Map CRC-Register default value is 0x0000 according to Register 8-16 definition in the MCP3x6x(R) DS.


#include <SPI.h>

#define CS 15
#define SPIMODE SPI_MODE0
#define SPISPEED 1400000

#define REFVSS 2.5

class ADC {
	private: SPIClass *hspi;
    int lastchannel;
    int32_t offset;
	
	public: void begin() {
		hspi = new SPIClass(HSPI);
		hspi->begin();
		pinMode(CS, OUTPUT);
		init_adc();
    lastchannel = 1;
    updateOffset();
	};

  public: void updateOffset(){
    offset = 0;
    for (int i=0;i<10;i++)
      offset += getIntValue(2, false);
    offset = offset/10;
  }

	public: void printAllRegisters(){
		uint8_t retval1 = 0x0;
		uint8_t retval2 = 0x0;

		hspi->beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPIMODE));
		for (int i = 1; i < 0x10; i++){
			digitalWrite(CS, LOW);

			retval1 = hspi->transfer(i << 2 | 0x41);
			retval2 = hspi->transfer(0x0);
			digitalWrite(CS, HIGH);
			Serial.print("Register: ");
			Serial.print(i, HEX);
			Serial.print(" ");
			Serial.print(retval1, HEX);
			Serial.print(" ");
			Serial.println(retval2, HEX);
		}		
		hspi->endTransaction(); 
	};

  public: int32_t getIntValue(int channel, bool withOffset){
		digitalWrite(CS, LOW);
		hspi->beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPIMODE));

    if (lastchannel != channel){
        hspi->transfer(0x06 << 2 | 0x42);
        hspi->transfer( 16*channel | 0x08);
        digitalWrite(CS, HIGH);
        digitalWrite(CS, LOW);
        delay(100);
        lastchannel = channel;
    }
		hspi->transfer(0x00 << 2 | 0x41);
		uint32_t voltage1 = hspi->transfer(0x00);
		uint32_t voltage2 = hspi->transfer(0x00);
		uint32_t voltage3 = hspi->transfer(0x00);

		digitalWrite(CS, HIGH);
		hspi->endTransaction();
		int32_t voltage = ((long)voltage1 << 16) + ((long)voltage2 <<8) + (long)voltage3;
		if (voltage1 & 0x80)    // sign
			voltage |= 0xFF000000; 
    if(withOffset)
      voltage = voltage - this->offset;
     return voltage;
  };

	public: double getValue(int channel, bool withOffset = false){
    return (double) (2 * getIntValue(channel, withOffset) * REFVSS / 16777215);    // 24Bit with sign
	};
	
	public: void init_adc(){
		digitalWrite(CS, LOW);
		hspi->beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPIMODE));
		hspi->transfer(0x01 << 2 | 0x42 );
		hspi->transfer(0x33);  	// Use internal Masterclock, provide it (for testing purposes) 
								// 11 = ADC Conversion mode
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(0x02 << 2 | 0x42);
		hspi->transfer(0x2C);   // AMC = MCLK OSR= 24576  0010 1100
    // hspi->transfer(0x4C);   // AMC = MCLK/2 OSR = 256 0100 1100
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(0x03 << 2 | 0x42);
		//const byte Config2= 0b10001011;
		hspi->transfer(0x8b);  // BIAS = 1,  gain=1
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(0x04 << 2 | 0x42);
		//const byte Config3= 0b10000000;
		hspi->transfer(0xC0);  // Continous conversion (CO)
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(0x05 << 2 | 0x42);
		//const byte ConfigIRQ= 0b01000010;
		hspi->transfer(0x07);
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(0x06 << 2 | 0x42);
		//const byte ConfigMUX= 0b00110010;
		hspi->transfer(0x18);   // CH1 vs. AGnd Refin+ = 2.5 Refin- = AGnd
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(_OFFSETCAL_ << 2 | 0x42);
		hspi->transfer(OFFSETCAL_CFG);   
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(_GAINCAL_ << 2 | 0x42);
		// hspi->transfer(GAINCAL_CFG);   
    hspi->transfer(GAINCAL_CFG >> 16 & 0xFF);   
    hspi->transfer(GAINCAL_CFG >> 8 & 0xFF);   
    hspi->transfer(GAINCAL_CFG & 0xFF);   
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(_RESERVED_B_ << 2 | 0x42);
		// hspi->transfer(RESERVED_B_CFG);   
    hspi->transfer(RESERVED_B_CFG >> 16 & 0xFF);   
    hspi->transfer(RESERVED_B_CFG >> 8 & 0xFF);   
    hspi->transfer(RESERVED_B_CFG & 0xFF);   
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(_RESERVED_C_ << 2 | 0x42);
		hspi->transfer(RESERVED_C_CFG);   
		digitalWrite(CS, HIGH);
		digitalWrite(CS, LOW);
		hspi->transfer(_RESERVED_E_ << 2 | 0x42);
		//hspi->transfer(RESERVED_E_CFG);   
    hspi->transfer(RESERVED_E_CFG >> 8 & 0xFF);   
    hspi->transfer(RESERVED_E_CFG & 0xFF);   
		digitalWrite(CS, HIGH);
		hspi->endTransaction();
	};
	
};
#endif
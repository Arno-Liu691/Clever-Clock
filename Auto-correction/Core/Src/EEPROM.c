#include "EEPROM.h"
#include "LCD_I2C.h"
#include "Clock.h"

const uint8_t TESTVAL = 124;    // Expected value to identify proper EEPROM initialization (using a small value)

void EEPROM_WriteUInt64(uint16_t virtAddressHIGH, uint16_t virtAddressLOW, uint64_t value) {
    EE_Status status;

    // Split the 64-bit value into high and low 32 bits
    uint32_t high = (uint32_t)(value >> 32);      // High 32 bits
    uint32_t low = (uint32_t)(value & 0xFFFFFFFF); // Low 32 bits

    // Write the high part to EEPROM
    status = EE_WriteVariable32bits(virtAddressHIGH, high);
    if (status != EE_OK) {
		if(status == EE_CLEANUP_REQUIRED){
			if(EE_CleanUp() != EE_OK){
			    LCD_Set_Cursor(3,0);
			    LCD_Send_String("EEPROM CLEANUP ERR");
			}
		}
		else{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("EEPROM WRITE ERR");
		}
    }

    // Write the low part to EEPROM
    status = EE_WriteVariable32bits(virtAddressLOW, low);
    if (status != EE_OK) {
		if(status == EE_CLEANUP_REQUIRED){
			if(EE_CleanUp() != EE_OK){
			    LCD_Set_Cursor(3,0);
			    LCD_Send_String("EEPROM CLEANUP ERR");
			}
		}
		else{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("EEPROM WRITE ERR");
		}
    }
}

void EEPROM_ReadUInt64(uint16_t virtAddressHIGH,uint16_t virtAddressLOW, uint64_t* value) {
    EE_Status status;
    uint32_t high = 0, low = 0;

    // Read the high 32 bits from EEPROM
    status = EE_ReadVariable32bits(virtAddressHIGH, &high);
    if (status != EE_OK) {
		LCD_Set_Cursor(3,0);
		LCD_Send_String("EEPROM READ ERR");
        *value = 0; // Set default value if read fails
        return; // Return error if it occurs
    }

    // Read the low 32 bits from EEPROM
    status = EE_ReadVariable32bits(virtAddressLOW, &low);
    if (status != EE_OK) {
		LCD_Set_Cursor(3,0);
		LCD_Send_String("EEPROM READ ERR");
        *value = 0; // Set default value if read fails
        return; // Return error if it occurs
    }

    // Combine the high and low parts into a 64-bit value
    *value = ((uint64_t)high << 32) | low;
}

void EEPROM_WriteInt32(uint16_t virtAddress, int32_t value) {
	EE_Status status;
	status = EE_WriteVariable32bits(virtAddress, (uint32_t)value);
	if(status != EE_OK){
		if(status == EE_CLEANUP_REQUIRED){
			if(EE_CleanUp() != EE_OK){
			    LCD_Set_Cursor(3,0);
			    LCD_Send_String("EEPROM CLEANUP ERR");
			}
		}
		else{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("EEPROM WRITE ERR");
		}
	}
}

void EEPROM_ReadInt32(uint16_t virtAddress, int32_t* value) {
    EE_Status status;
    uint32_t tempValue;
    status = EE_ReadVariable32bits(virtAddress, &tempValue);
    if (status != EE_OK) {
		LCD_Set_Cursor(3,0);
		LCD_Send_String("EEPROM READ ERR");
    	*value = 0;
        return;
    }
    *value = (int32_t)tempValue;
}

void EEPROM_WriteUInt8(uint16_t virtAddress, uint8_t value) {
	EE_Status status;
	status = EE_WriteVariable8bits(virtAddress, value);
	if(status != EE_OK){
		if(status == EE_CLEANUP_REQUIRED){
			if(EE_CleanUp() != EE_OK){
			    LCD_Set_Cursor(3,0);
			    LCD_Send_String("EEPROM CLEANUP ERR");
			}
		}
		else{
			LCD_Set_Cursor(3,0);
			LCD_Send_String("EEPROM WRITE ERR");
		}
	}
}

void EEPROM_Init() {
    // Unlock the Flash memory to allow write operations
    HAL_FLASH_Unlock();
    uint8_t testval;
//     First, try to read the existing values to see if they match what we expect
    EE_Init(EE_CONDITIONAL_ERASE);
    EE_Status status = EE_ReadVariable8bits(VIRT_ADDRESS_TESTVAL, &testval);
	if(status != EE_OK){
		HAL_GPIO_TogglePin(Red_LED_GPIO_Port, Red_LED_Pin); //indicate error in eeprom initialisation
	    LCD_Set_Cursor(3,0);
	    LCD_Send_String("EEPROM READ ERR");
	}
    if (status == EE_OK && testval == TESTVAL) {
        // If values are correct, skip the erase
    	EEPROM_ReadInt32(VIRT_ADDRESS_PPM, &ppm);
        return; // EEPROM is already initialized correctly, no need for re-initialization or erasure
    }
    // If values are incorrect, initialize the EEPROM with forced erase
    EE_Status statusInit = EE_Init(EE_FORCED_ERASE);
    if (statusInit != EE_OK) {
	  LCD_Set_Cursor(3,0);
	  LCD_Send_String("EEPROM INIT ERROR");
	  return;
    }
    EEPROM_WriteInt32(VIRT_ADDRESS_PPM, ppm);
    EEPROM_WriteUInt8(VIRT_ADDRESS_TESTVAL, TESTVAL);
}

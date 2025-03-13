#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "main.h"
#include "eeprom_emul_conf.h"
#include "eeprom_emul_types.h"
#include "eeprom_emul.h"
#include "flash_interface.h"

#define VIRT_ADDRESS_TESTVAL 0x0001 // Virtual address for test value to check initialization
#define VIRT_ADDRESS_PPM 0x0002
//#define VIRT_ADDRESS_DRIFT    0x0002 // Virtual address for drift time
//#define VIRT_ADDRESS_ELAPSED_HIGH  0x0003 // Virtual address for the high 32 bits of uint64_t
//#define VIRT_ADDRESS_ELAPSED_LOW   0x0004 // Virtual address for the low 32 bits of uint64_t

void EEPROM_Init();
void EEPROM_ReadUInt8(uint16_t virtAddress, uint8_t* value);
void EEPROM_WriteUInt8(uint16_t virtAddress, uint8_t value);
void EEPROM_ReadInt32(uint16_t virtAddress, int32_t* value);
void EEPROM_WriteInt32(uint16_t virtAddress, int32_t value);
void EEPROM_WriteUInt64(uint16_t virtAddressHIGH,uint16_t virtAddressLOW, uint64_t value);
void EEPROM_ReadUInt64(uint16_t virtAddressHIGH,uint16_t virtAddressLOW, uint64_t* value);

#endif /* INC_EEPROM_H_ */

#include <stdio.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_flash.h"
#include "oid_flash.h"

uint8_t  sector_data_temp[4096];

void flash_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t i;
	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

	if (HAL_Flash_Read(PRJCONF_IMG_FLASH, addr, data, len) != HAL_OK) {
		FLASH_MSG_DBG("flash read failed\n");
		goto fail;
	}

	FLASH_MSG_DBG("flash read result:\n");
	for (i = 0; i < 256; i++) {
		FLASH_MSG_DBG("%d ", data[i]);
		if (i != 0 && (i % 16) == 0)
			FLASH_MSG_DBG("\n");
	}
	FLASH_MSG_DBG("\n");

fail:
	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*读一个STRUCT_SSID_PWD结构体
 *
 *约定:void flash_ssid_pwd_read(STRUCT_SSID_PWD *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行与第11行的 STRUCT_SSID_PWD  要改成相应的结构体名
 *     2:第11行的  FLASH_WLAN_SSID_PWD_ADDR  要改成相应的首地址
 */
void flash_ssid_pwd_read(STRUCT_SSID_PWD *int_p)
{
//    uint32_t i;

	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}
    
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_WLAN_SSID_PWD_ADDR, (uint8_t *)int_p, sizeof(STRUCT_SSID_PWD));

//	FLASH_MSG_DBG("flash read result:\n");
//	for (i = 0; i < 256; i++) {
//		printf("%d ", sector_data_temp[i]);
//		if (i != 0 && (i % 16) == 0)
//			printf("\n");
//	}
//	printf("\n");

	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*写一个STRUCT_SSID_PWD结构体
 *
 *约定:void flash_ssid_pwd_write(STRUCT_SSID_PWD *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行、第8行与第21行的 STRUCT_SSID_PWD  要改成相应的结构体名
 *     2:第22行的  FLASH_WLAN_SSID_PWD_ADDR  要改成相应的首地址
 *
 *注:该函数只能写地址FLASH_USER_DATA_START_ADDR开始的扇区，需要写其他地址用其他函数
 */
void flash_ssid_pwd_write(STRUCT_SSID_PWD *int_p)
{
	uint8_t i;
	uint8_t *temp = (uint8_t *)int_p;
	FlashEraseMode size_type;
	size_type = FLASH_ERASE_4KB;

    FLASH_MSG_DBG("STRUCT_SSID_PWD size:%d\n",sizeof(STRUCT_SSID_PWD));
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

    //读取扇区内容
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	//擦除扇区内容
    HAL_Flash_Erase(PRJCONF_IMG_FLASH, size_type, FLASH_USER_DATA_START_ADDR, 1);
    
	//写入数据
	for(i=0; i<sizeof(STRUCT_SSID_PWD); i++){
	    sector_data_temp[FLASH_WLAN_SSID_PWD_ADDR-FLASH_USER_DATA_START_ADDR+i] = temp[i];
	}
	HAL_Flash_Write(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*读一个STRUCT_FLASH_STATE结构体
 *
 *约定:void flash_ssid_pwd_read(STRUCT_FLASH_STATE *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行与第11行的 STRUCT_FLASH_STATE  要改成相应的结构体名
 *     2:第11行的  FLASH_STATE_ADDR  要改成相应的首地址
 */
void flash_state_read(STRUCT_FLASH_STATE *int_p)
{
//    uint32_t i;

	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}
    
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_STATE_ADDR, (uint8_t *)int_p, sizeof(STRUCT_FLASH_STATE));

//	FLASH_MSG_DBG("flash read result:\n");
//	for (i = 0; i < 256; i++) {
//		printf("%d ", sector_data_temp[i]);
//		if (i != 0 && (i % 16) == 0)
//			printf("\n");
//	}
//	printf("\n");

	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*写一个STRUCT_FLASH_STATE结构体
 *
 *约定:void flash_ssid_pwd_write(STRUCT_FLASH_STATE *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行、第8行与第21行的 STRUCT_FLASH_STATE  要改成相应的结构体名
 *     2:第22行的  FLASH_WLAN_SSID_PWD_ADDR  要改成相应的首地址
 *
 *注:该函数只能写地址FLASH_USER_DATA_START_ADDR开始的扇区，需要写其他地址用其他函数
 */
void flash_state_write(STRUCT_FLASH_STATE *int_p)
{
	uint8_t i;
	uint8_t *temp = (uint8_t *)int_p;
	FlashEraseMode size_type;
	size_type = FLASH_ERASE_4KB;

    FLASH_MSG_DBG("STRUCT_SSID_PWD size:%d\n",sizeof(STRUCT_FLASH_STATE));
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK){
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

    //读取扇区内容
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	//擦除扇区内容
    HAL_Flash_Erase(PRJCONF_IMG_FLASH, size_type, FLASH_USER_DATA_START_ADDR, 1);
    
	//写入数据
	for(i=0; i<sizeof(STRUCT_FLASH_STATE); i++){
	    sector_data_temp[FLASH_STATE_ADDR - FLASH_USER_DATA_START_ADDR + i] = temp[i];
	}
	HAL_Flash_Write(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*读一个STRUCT_IMEEK_ID结构体
 *
 *约定:void imeek_id_read(STRUCT_IMEEK_ID *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行与第11行的 STRUCT_IMEEK_ID  要改成相应的结构体名
 *     2:第11行的  FLASH_IMEEK_ID_ADDR  要改成相应的首地址
 */
void imeek_id_read(STRUCT_IMEEK_ID *int_p)
{
//    uint32_t i;

	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}
    
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_IMEEK_ID_ADDR, (uint8_t *)int_p, sizeof(STRUCT_IMEEK_ID));

//	FLASH_MSG_DBG("flash read result:\n");
//	for (i = 0; i < 256; i++) {
//		printf("%d ", sector_data_temp[i]);
//		if (i != 0 && (i % 16) == 0)
//			printf("\n");
//	}
//	printf("\n");

	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*写一个STRUCT_IMEEK_ID结构体
 *
 *约定:void imeek_id_write(STRUCT_IMEEK_ID *int_p)称为该函数的第一行
 *对于不同的结构体该函数要改动的地方有:
 *     1:第1行、第8行与第21行的 STRUCT_IMEEK_ID  要改成相应的结构体名
 *     2:第22行的  FLASH_IMEEK_ID_ADDR  要改成相应的首地址
 *
 *注:该函数只能写地址FLASH_USER_DATA_START_ADDR开始的扇区，需要写其他地址用其他函数
 */
void imeek_id_write(STRUCT_IMEEK_ID *int_p)
{
	uint8_t i;
	uint8_t *temp = (uint8_t *)int_p;
	FlashEraseMode size_type;
	size_type = FLASH_ERASE_4KB;

    FLASH_MSG_DBG("STRUCT_IMEEK_ID size:%d\n",sizeof(STRUCT_IMEEK_ID));
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK){
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

    //读取扇区内容
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	//擦除扇区内容
    HAL_Flash_Erase(PRJCONF_IMG_FLASH, size_type, FLASH_USER_DATA_START_ADDR, 1);
    
	//写入数据
	for(i=0; i<sizeof(STRUCT_IMEEK_ID); i++){
	    sector_data_temp[FLASH_IMEEK_ID_ADDR - FLASH_USER_DATA_START_ADDR + i] = temp[i];
	}
	HAL_Flash_Write(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*读一个int型数据*/
void flash_int_read(int start_addr, uint32_t *int_p)
{
//    uint32_t i;

	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}
    
	HAL_Flash_Read(PRJCONF_IMG_FLASH, start_addr, (uint8_t *)int_p, sizeof(int));

//	FLASH_MSG_DBG("flash read result:\n");
//	for (i = 0; i < 256; i++) {
//		printf("%d ", sector_data_temp[i]);
//		if (i != 0 && (i % 16) == 0)
//			printf("\n");
//	}
//	printf("\n");

	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}

}

/*写入一个int型数据*/
void flash_int_write(int start_addr, uint32_t *int_p)
{
	uint8_t i;
	uint8_t *temp = (uint8_t *)int_p;
	FlashEraseMode size_type;
	size_type = FLASH_ERASE_4KB;

    FLASH_MSG_DBG("int size:%d\n",sizeof(int));
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

    //读取扇区内容
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	//擦除扇区内容
    HAL_Flash_Erase(PRJCONF_IMG_FLASH, size_type, FLASH_USER_DATA_START_ADDR, 1);
    
	//写入数据
	for(i=0; i<sizeof(int); i++){
	    sector_data_temp[start_addr - FLASH_USER_DATA_START_ADDR + i] = temp[i];
	}
	HAL_Flash_Write(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}

/*读一个char型数据*/
void flash_char_read(int start_addr, uint8_t *char_p)
{
//    uint32_t i;

	FLASH_MSG_DBG("flash driver open...\n");
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}
    
	HAL_Flash_Read(PRJCONF_IMG_FLASH, start_addr, (uint8_t *)char_p, sizeof(char));

//	FLASH_MSG_DBG("flash read result:\n");
//	for (i = 0; i < 256; i++) {
//		printf("%d ", sector_data_temp[i]);
//		if (i != 0 && (i % 16) == 0)
//			printf("\n");
//	}
//	printf("\n");

	FLASH_MSG_DBG("flash driver close.\n");
	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}

}

/*写入一个char型数据*/
void flash_char_write(int start_addr, uint8_t *char_p)
{
	uint8_t i;
	uint8_t *temp = (uint8_t *)char_p;
	FlashEraseMode size_type;
	size_type = FLASH_ERASE_4KB;

    FLASH_MSG_DBG("int size:%d\n",sizeof(int));
	if (HAL_Flash_Open(PRJCONF_IMG_FLASH, 5000) != HAL_OK) {
		FLASH_MSG_DBG("flash driver open failed\n");
		return;
	}

    //读取扇区内容
	HAL_Flash_Read(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	//擦除扇区内容
    HAL_Flash_Erase(PRJCONF_IMG_FLASH, size_type, FLASH_USER_DATA_START_ADDR, 1);
    
	//写入数据
	for(i=0; i<sizeof(char); i++){
	    sector_data_temp[start_addr - FLASH_USER_DATA_START_ADDR + i] = temp[i];
	}
	HAL_Flash_Write(PRJCONF_IMG_FLASH, FLASH_USER_DATA_START_ADDR, sector_data_temp, 4096);

	if (HAL_Flash_Close(PRJCONF_IMG_FLASH) != HAL_OK) {
		FLASH_MSG_DBG("flash driver close failed\n");
		return;
	}
}


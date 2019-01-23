#include "ets_sys.h"
#include "osapi.h"
#include "lcd_sm1612.h"
#include "gpio.h"

uint8_t lcd_digPos_arr[] = { DIG1_ADDR, DIG2_ADDR, DIG3_ADDR, DIG4_ADDR,
		DIG5_ADDR, DIG6_ADDR };

#define LCD_SDA_MUX PERIPHS_IO_MUX_MTMS_U
#define LCD_SCL_MUX PERIPHS_IO_MUX_GPIO2_U
#define LCD_SDA_GPIO 14
#define LCD_SCL_GPIO 2
#define LCD_SDA_FUNC FUNC_GPIO14
#define LCD_SCL_FUNC FUNC_GPIO2


LOCAL void ICACHE_FLASH_ATTR
lcd_gpio_init(void)
{
	gpio_init();
    //ETS_GPIO_INTR_DISABLE() ;

    PIN_FUNC_SELECT(LCD_SDA_MUX, LCD_SDA_FUNC);
    PIN_FUNC_SELECT(LCD_SCL_MUX, LCD_SCL_FUNC);
    PIN_PULLUP_DIS(LCD_SDA_MUX);
    PIN_PULLUP_EN(LCD_SCL_MUX);


    //GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(LCD_SDA_GPIO)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(LCD_SDA_GPIO))) /*| GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)*/); //open drain;
    //GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << LCD_SDA_GPIO));
    //GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(LCD_SCL_GPIO)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(LCD_SCL_GPIO))) /*| GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE)*/); //open drain;
    //GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << LCD_SCL_GPIO));

    //ETS_GPIO_INTR_ENABLE() ;
}


void SM1685_Start(void)
{
	//SM1685__SDA_SET;    //_SDA=1
	//digitalWrite(_SDA, HIGH);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 1);
	os_delay_us(2);
	//SM1685__SCL_SET ;  //_SCL=1
	//digitalWrite(_SCL, HIGH);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
	os_delay_us(10); //_SCLÆµÂÊµÚ800KÒÔÏÂ¿ÉÒÔ²»ÓÃÑÓÊ±£¬ÒÔÏÂ³ÌÐòÍ¬ÑùÇ
	//SM1685__SDA_CLR;  //_SDA=0
	//digitalWrite(_SDA, LOW);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 0);
	os_delay_us(2);
	//SM1685__SCL_CLR;  //_SCL=1
	//digitalWrite(_SCL, LOW);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 0);
	os_delay_us(2);
}

void SM1685_Ack(void)
{
	//SM1685__SCL_CLR;
	//digitalWrite(_SCL, LOW);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 0);
	os_delay_us(10);
	//SM1685__SCL_SET ;
	//digitalWrite(_SCL, HIGH);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
	os_delay_us(2);
//	while(dio);  //¿É²»×öÅÐ¶Ï
	os_delay_us(10);
	//SM1685__SCL_CLR;
	//digitalWrite(_SCL, LOW);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 0);
	os_delay_us(2);
}

void SM1685_Stop(void)
{
	//SM1685__SDA_CLR;
	//digitalWrite(_SDA, LOW);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 0);
	os_delay_us(2);
	//SM1685__SCL_SET ;
	//digitalWrite(_SCL, HIGH);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
	os_delay_us(10);
	//SM1685__SDA_SET;
	//digitalWrite(_SDA, HIGH);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 1);
	os_delay_us(2);
}

void SM1685_Send_8bit(uint8_t DAT)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		//SM1685__SCL_CLR;
		//digitalWrite(_SCL, LOW);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 0);
		os_delay_us(2);
		if (DAT & 0x80) {
			//SM1685__SDA_SET;
			//digitalWrite(_SDA, HIGH);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 1);
		} else {
			//SM1685__SDA_CLR;
			//digitalWrite(_SDA, LOW);
			GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SDA_GPIO), 0);
		}
		os_delay_us(10);
		DAT = DAT << 1;
		//SM1685__SCL_SET ;
		//digitalWrite(_SCL, HIGH);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
		os_delay_us(10);
	}
	SM1685_Ack();
}

void SM1685_Command(uint8_t com)
{
	SM1685_Stop();
	//asm volatile("nop\n\t"::);
	os_delay_us(10);
	SM1685_Start();
	SM1685_Send_8bit(com);
}

void ICACHE_FLASH_ATTR
lcd_init(void) {
	uint8_t i;
	lcd_gpio_init();
	os_delay_us(5000);
	/*SM1685_Command(0x40);
	os_delay_us(500);
	SM1685_Command(0x03); //Ä£ÊœÑ¡Ôñ
	os_delay_us(500);
	SM1685_Command(0xc0); //µØÖ·Ñ¡Ôñ
	os_delay_us(500);
	for (i = 0; i < 12; i++) {
		SM1685_Send_8bit(data);
		os_delay_us(500);
	}
	SM1685_Command(0x8f);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
	os_delay_us(2000);*/
}

void ICACHE_FLASH_ATTR
lcd_data(uint8_t data, uint8_t postion) {
	uint8_t i;
	SM1685_Command(0x40);
		os_delay_us(500);
		SM1685_Command(0x03); //Ä£ÊœÑ¡Ôñ
		os_delay_us(500);
		SM1685_Command(0xc0); //µØÖ·Ñ¡Ôñ
		os_delay_us(500);
		for (i = 0; i < 12; i++) {
			SM1685_Send_8bit(data);
			os_delay_us(500);
		}
		SM1685_Command(0x8f);
		GPIO_OUTPUT_SET(GPIO_ID_PIN(LCD_SCL_GPIO), 1);
		os_delay_us(2000);

}

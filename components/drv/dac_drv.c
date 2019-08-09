#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/adc.h"
#include "soc/rtc_periph.h"
#include "soc/sens_reg.h"
#include "esp_adc_cal.h"

//#define DEFAULT_VREF    1100
//static esp_adc_cal_characteristics_t *adc_chars;

//void adc_init(void)
//{
//    adc1_config_width(ADC_WIDTH_BIT_12);
//    adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
//	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
//    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
//}

void dac_init(uint8_t channel)
{
//	REG_SET_FIELD(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, freq);
//	REG_SET_BIT(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

//	SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
	dac_output_enable(channel);
}

void dac_volt(uint8_t volt)
{
	dac_output_voltage(1,volt);
}

//uint32_t adc_get_raw(void)
//{
//	return adc1_get_raw(ADC1_CHANNEL_0);
//}
//
//uint32_t adc_get_volt(void)
//{
//	uint32_t temp,volt;
//	temp = adc1_get_raw(ADC1_CHANNEL_0);
//	volt = esp_adc_cal_raw_to_voltage(temp, adc_chars);
//	return volt;
//}


#include "driver/gpio.h"
#include "driver/dac.h"

#include "soc/rtc_periph.h"
#include "soc/sens_reg.h"

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

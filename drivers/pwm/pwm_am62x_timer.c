/*
 * Copyright (c) 2025 Minimal PWM Driver for AM62x.
 *
 * TIMER PWM driver for Zephyr's PWM model.
 */

 #define DT_DRV_COMPAT ti_am62x_timer_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ti_am62x_timer_pwm, CONFIG_PWM_LOG_LEVEL);


// TCLR register : masks and values
#define TIMER_TCLR_ST_SHIFT            		(0)
#define TIMER_TCLR_ST_MASK             		(0x1u << TIMER_TCLR_ST_SHIFT)
#define TIMER_TCLR_ST_STOP					(0x0u << TIMER_TCLR_ST_SHIFT)
#define TIMER_TCLR_ST_START					(0x1u << TIMER_TCLR_ST_SHIFT)

#define TIMER_TCLR_AR_SHIFT      			(1)
#define TIMER_TCLR_AR_MASK       			(0x1u << TIMER_TCLR_AR_SHIFT)
#define TIMER_TCLR_AR_ONE_SHOT       		(0x0u << TIMER_TCLR_AR_SHIFT)
#define TIMER_TCLR_AR_AUTORELOAD_TIMER      (0x1u << TIMER_TCLR_AR_SHIFT)

#define TIMER_TCLR_PTV_SHIFT              	(2)
#define TIMER_TCLR_PTV_MASK               	(0x7u << TIMER_TCLR_PTV_SHIFT)

#define TIMER_TCLR_PRE_SHIFT 				(5)
#define TIMER_TCLR_PRE_MASK  				(0x1u << TIMER_TCLR_PRE_SHIFT)
#define TIMER_TCLR_PRE_DISABLE				(0x0u << TIMER_TCLR_PRE_SHIFT)
#define TIMER_TCLR_PRE_ENABLE				(0x1u << TIMER_TCLR_PRE_SHIFT)

#define TIMER_TCLR_CE_SHIFT               	(6)
#define TIMER_TCLR_CE_MASK                	(0x1u << TIMER_TCLR_CE_SHIFT)
#define TIMER_TCLR_CE_DISABLE				(0x0u << TIMER_TCLR_CE_SHIFT)
#define TIMER_TCLR_CE_ENABLE				(0x1u << TIMER_TCLR_CE_SHIFT)

#define TIMER_TCLR_SCPWM_SHIFT            	(7)
#define TIMER_TCLR_SCPWM_MASK             	(0x1u << TIMER_TCLR_SCPWM_SHIFT)
#define TIMER_TCLR_SCPWM_POSITIVE       	(0x0u << TIMER_TCLR_SCPWM_SHIFT)
#define TIMER_TCLR_SCPWM_NEGATIVE      		(0x1u << TIMER_TCLR_SCPWM_SHIFT)

#define TIMER_TCLR_TRG_SHIFT              	(10)
#define TIMER_TCLR_TRG_MASK               	(0x3 << TIMER_TCLR_TRG_SHIFT)
#define TIMER_TCLR_TRG_DISABLE   			(0x0u << TIMER_TCLR_TRG_SHIFT)
#define TIMER_TCLR_TRG_OVERFLOW_ONLY      	(0x1u << TIMER_TCLR_TRG_SHIFT)
#define TIMER_TCLR_TRG_OVERFLOW_AND_MATCH 	(0x2u << TIMER_TCLR_TRG_SHIFT)

#define TIMER_TCLR_PT_SHIFT               	(12)
#define TIMER_TCLR_PT_MASK                	(0x1u << TIMER_TCLR_PT_SHIFT)
#define TIMER_TCLR_PT_PULSE               	(0x0u << TIMER_TCLR_PT_SHIFT)
#define TIMER_TCLR_PT_TOGGLE              	(0x1u << TIMER_TCLR_PT_SHIFT)

#define TIMER_TCLR_GPO_CFG_SHIFT          	(14)
#define TIMER_TCLR_GPO_CFG_MASK           	(0x1u << TIMER_TCLR_GPO_CFG_SHIFT)
#define TIMER_TCLR_GPO_CFG_PWM            	(0x0u << TIMER_TCLR_GPO_CFG_SHIFT)
#define TIMER_TCLR_GPO_CFG_TRIGGER        	(0x1u << TIMER_TCLR_GPO_CFG_SHIFT)

// IRQSTATUS _CLR register : values
#define TIMER_IRQSTATUS_CLR_MATCH_FLAG    	(0x1u)
#define TIMER_IRQSTATUS_CLR_OVERFLOW_FLAG 	(0x2u)
#define TIMER_IRQSTATUS_CLR_CAPTURE_FLAG 	(0x4u)

// TSICR register : masks and values
#define TIMER_TSICR_POSTED_SHIFT            (2)
#define TIMER_TSICR_POSTED_MASK           	(0x1u << TIMER_TSICR_POSTED_SHIFT)
#define TIMER_TSICR_POSTED_INACTIVE			(0x0u << TIMER_TSICR_POSTED_SHIFT)
#define TIMER_TSICR_POSTED_ACTIVE 			(0x1u << TIMER_TSICR_POSTED_SHIFT)

// TWPS register : masks and values
#define TIMER_TWPS_W_PEND_TCLR_SHIFT        (0)
#define TIMER_TWPS_W_PEND_TCLR_MASK        	(0x1u << TIMER_TWPS_W_PEND_TCLR_SHIFT)
#define TIMER_TWPS_W_PEND_TCLR_NOT_PENDING	(0x0u << TIMER_TWPS_W_PEND_TCLR_SHIFT)
#define TIMER_TWPS_W_PEND_TCLR_PENDING 		(0x1u << TIMER_TWPS_W_PEND_TCLR_SHIFT)


// Timer registers 
struct am62x_timer_regs 
{
	uint32_t tidr;				//Ofs 0h
	uint32_t __pad0[3];
	uint32_t tiocp_cfg;			//Ofs 10h
	uint32_t __pad1[3];
	uint32_t irq_eoi;			//Ofs 20h
	uint32_t irqstatus_raw;		//Ofs 24h
	uint32_t irqstatus;			//Ofs 28h
	uint32_t irqstatus_set;		//Ofs 2Ch
	uint32_t irqstatus_clr;		//Ofs 30h
	uint32_t irqwakeen;			//Ofs 34h
	uint32_t tclr;				//Ofs 38h
	uint32_t tcrr;				//Ofs 3Ch
	uint32_t tldr;				//Ofs 40h
	uint32_t ttgr;				//Ofs 44h
	uint32_t twps;				//Ofs 48h
	uint32_t tmar;				//Ofs 4Ch
	uint32_t tcar1;				//Ofs 50h
	uint32_t tsicr;				//Ofs 54h
	uint32_t tcar2;				//Ofs 58h
	uint32_t tpir;				//Ofs 5Ch
	uint32_t tnir;				//Ofs 60h	
	uint32_t tcvr;				//Ofs 64h
	uint32_t tocr;				//Ofs 68h
	uint32_t towr;				//Ofs 6Ch
};

#define DEV_CFG(_dev)     ((const struct pwm_am62x_timer_config *)(_dev)->config)
#define DEV_DATA(_dev)    ((struct pwm_am62x_timer_data *)(_dev)->data)
#define DEV_REG_BASE(dev) ((struct am62x_timer_regs *)DEVICE_MMIO_NAMED_GET(dev, reg_base))

// PWM configuration
struct pwm_am62x_timer_config 
{
	DEVICE_MMIO_NAMED_ROM(reg_base);
	/* Clock Frequency in Hz */
	uint32_t clock_freq;
	/* Clock Prescaler */
	uint32_t clock_prescaler;
	/* pinctrl configurations */
	const struct pinctrl_dev_config *pincfg;
};

// PWM data
struct pwm_am62x_timer_data
{
	DEVICE_MMIO_NAMED_RAM(reg_base);
	/* Default settings to configure TCLR register */
	uint32_t default_tclr;
};

// Write to TCLR register 
static inline void pwm_am62x_timer_tclr_wr(const struct device *dev, uint32_t value)
{
	volatile struct am62x_timer_regs *regs = DEV_REG_BASE(dev);

	// Write to TCLR register
	regs->tclr = value;
	
	// Check if posted mode active
	if ( (regs->tsicr & TIMER_TSICR_POSTED_MASK) == TIMER_TSICR_POSTED_ACTIVE)
	{
		// Posted mode active, wait until no writes are pending for the TCLR register
		while ( (regs->twps & TIMER_TWPS_W_PEND_TCLR_MASK) == TIMER_TWPS_W_PEND_TCLR_PENDING);
	}
}

// Set the period and pulse width for a single PWM output.
static int pwm_am62x_timer_set_cycles(const struct device *dev, uint32_t channel, uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
	volatile struct am62x_timer_regs *regs = DEV_REG_BASE(dev);
	struct pwm_am62x_timer_data *data = DEV_DATA(dev);
	uint32_t tclr, tldr, tmar, diff;
	// Single channel for each pwm device
	ARG_UNUSED(channel);

	// Get TCLR default settings  
	tclr = data->default_tclr;

    // Configure the PWM output pin level (active-high pulse or active-low pulse)
    tclr &= ~TIMER_TCLR_SCPWM_MASK;
	tclr |= ((flags & PWM_POLARITY_INVERTED) == PWM_POLARITY_INVERTED) ? (TIMER_TCLR_SCPWM_NEGATIVE) : (TIMER_TCLR_SCPWM_POSITIVE);

	if ((pulse == 0) || (period == 0))
	{ // Set PWM output pin to low level (PWM_POLARITY_NORMAL) or high level (PWM_POLARITY_INVERTED)  
		
		// Stop timer
		tclr &= ~TIMER_TCLR_ST_MASK;
		tclr |= TIMER_TCLR_ST_STOP;
	}
	else if (pulse >= period)
	{// Set PWM output pin to high level (PWM_POLARITY_NORMAL) or low level (PWM_POLARITY_INVERTED) 

		if (pulse > period)
		{
			LOG_WRN("Pulse (%d) is wider than the period (%d).", pulse, period);
		}

		tclr &= ~TIMER_TCLR_SCPWM_MASK;
		tclr |= ((flags & PWM_POLARITY_INVERTED) == PWM_POLARITY_INVERTED) ? (TIMER_TCLR_SCPWM_POSITIVE) : (TIMER_TCLR_SCPWM_NEGATIVE);

		// Stop timer
		tclr &= ~TIMER_TCLR_ST_MASK;
		tclr |= TIMER_TCLR_ST_STOP;
	}
	else
	{// Toggle PWM output pin

		tldr = (0xFFFFFFFFul - period) + 1ul;
		tmar = (0xFFFFFFFFul - (period - pulse)) + 1ul;
		//AM62x TRM: "In PWM mode, TIMER_TLDR must be maintained at less than or equal to 0xFFFF FFFD."
		//AM62x TRM: "The TIMER_TLDR and TIMER_TMAR must keep values below the overflow value (0xFFFF FFFF) by at least two units."
		tldr = (tldr > 0xFFFFFFFDul) ? (0xFFFFFFFDul) : tldr ;
		tmar = (tmar > 0xFFFFFFFDul) ? (0xFFFFFFFDul) : tmar ;
		//AM62x TRM: "If the PWM trigger events are both overflow and match, the difference between the values kept in the
		// TIMER_TMAR and the value in the TIMER_TLDR must be at least two units."
		diff = tmar - tldr;
		if (diff < 2ul)
		{
			diff = 2ul - diff;

			if (tldr >= diff)
			{
				tldr -= diff;
			}
			else
			{
				tmar += diff;
			}

			LOG_DBG("Adjust tmar (0x%x) & tldr (0x%x) to have at least 2 units difference.", tmar, tldr);
		}

		// Set Trigger mode: Trigger on overflow and match
		tclr &= ~TIMER_TCLR_TRG_MASK;
		tclr |= TIMER_TCLR_TRG_OVERFLOW_AND_MATCH;

		// Write period to TLDR register
		regs->tldr = tldr;

		// Write pulse to TMAR register
		regs->tmar = tmar;

		// Write period to TCRR register by writing to TTGR register (any write value will do)
		regs->ttgr = 0xFFFFFFFFul;
		
	}

	// Write to TCLR register
	pwm_am62x_timer_tclr_wr(dev, tclr);

	LOG_DBG("Period and pulse successfully set.");

    return 0;
}

// 	Get the clock rate (cycles per second) for a single PWM output.
static int pwm_am62x_timer_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct pwm_am62x_timer_config *config = DEV_CFG(dev);
	// Single channel for each pwm device
	ARG_UNUSED(channel);

	if ((!config->clock_prescaler) || ( config->clock_freq  < config->clock_prescaler))
	{
		LOG_ERR("Clock rate failed get.");
		return -EINVAL;
	}

	if (cycles) 
	{
		*cycles = (uint64_t)( config->clock_freq / config->clock_prescaler);
		LOG_DBG("Clock rate successfully get.");
	}
	else
	{
		LOG_ERR("Clock rate failed get.");
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
// Enable PWM period/pulse width capture for a single PWM input.
static int pwm_am62x_timer_enable_capture(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
		
	// Function not implemented.
	return -ENOSYS;
}

// Disable PWM period/pulse width capture for a single PWM input.
static int pwm_am62x_timer_disable_capture(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	
	// Function not implemented.
	return -ENOSYS;
}

// Configure PWM period/pulse width capture for a single PWM input.
static int pwm_am62x_timer_configure_capture(const struct device *dev, uint32_t channel, pwm_flags_t flags, pwm_capture_callback_handler_t cb, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(flags);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
			
	// Function not implemented.
	return -ENOSYS;
}

#endif // CONFIG_PWM_CAPTURE

//Capture a single PWM period/pulse width in clock cycles for a single PWM input.
int pwm_am62x_timer_capture_cycles(const struct device *dev, uint32_t channel, pwm_flags_t flags, uint32_t *period, uint32_t *pulse, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(flags);
	ARG_UNUSED(period);
	ARG_UNUSED(pulse);
	ARG_UNUSED(timeout);
	
	// Function not implemented.
	return -ENOSYS;
}

//Initialize PWM
static int pwm_am62x_timer_init(const struct device *dev)
{		
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	volatile struct am62x_timer_regs *regs = DEV_REG_BASE(dev);
	const struct pwm_am62x_timer_config *config = DEV_CFG(dev);
	struct pwm_am62x_timer_data *data = DEV_DATA(dev);
	uint32_t tclr;
	int ret;

	// Read TCLR register
	tclr = regs->tclr;

	// Stop Timer control
	tclr &= ~TIMER_TCLR_ST_MASK;
	tclr |= TIMER_TCLR_ST_STOP;
	// Write to TCLR register
	pwm_am62x_timer_tclr_wr(dev, tclr);

   	// ***  Default settings for TCLR register ***:
	// Set Trigger mode: No trigger
	tclr &= ~TIMER_TCLR_TRG_MASK;
	tclr |= TIMER_TCLR_TRG_DISABLE;
	// Set toggle mode: Toggle modulation
	tclr &= ~TIMER_TCLR_PT_MASK;
	tclr |= TIMER_TCLR_PT_TOGGLE;
	// Set General purpose output: PWM output
	tclr &= ~TIMER_TCLR_GPO_CFG_MASK;
	tclr |= TIMER_TCLR_GPO_CFG_PWM;
	// Set Compare mode: Enable
	tclr &= ~TIMER_TCLR_CE_MASK;
	tclr |= TIMER_TCLR_CE_ENABLE;
	// Set Prescaler: Disable
	tclr &= ~TIMER_TCLR_PRE_MASK;
	tclr |= TIMER_TCLR_PRE_DISABLE;
	// Set Autoreload mode: Autoreload timer
	tclr &= ~TIMER_TCLR_AR_MASK;
	tclr |= TIMER_TCLR_AR_AUTORELOAD_TIMER;
	// Set Timer: Start
    tclr &= ~TIMER_TCLR_ST_MASK;
    tclr |= TIMER_TCLR_ST_START;
	//Save default settings
	data->default_tclr = tclr;

   // Disable capture interrupt, 0overflow interrupt and match interrupt
   regs->irqstatus_clr = TIMER_IRQSTATUS_CLR_MATCH_FLAG | TIMER_IRQSTATUS_CLR_OVERFLOW_FLAG | TIMER_IRQSTATUS_CLR_CAPTURE_FLAG;		

	//Set PWM Pin control (Muxing)
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) 
	{
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

// Declare PWM API
static DEVICE_API(pwm, pwm_am62x_timer_driver_api) = {
	.set_cycles = pwm_am62x_timer_set_cycles,
	.get_cycles_per_sec = pwm_am62x_timer_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_am62x_timer_configure_capture,
	.enable_capture = pwm_am62x_timer_enable_capture,
	.disable_capture = pwm_am62x_timer_disable_capture
#endif /* CONFIG_PWM_CAPTURE */
};


#define PWM_AM62X_TIMER_DEFINE(idx)						       \
												       \
	PINCTRL_DT_INST_DEFINE(idx);					       \
														\
	static struct pwm_am62x_timer_data pwm_am62x_timer_data_##idx;				\
	static const struct pwm_am62x_timer_config pwm_am62x_timer_config_##idx = {	       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(idx)),				\
		.clock_freq = DT_INST_PROP(idx, clock_frequency),	       \
		.clock_prescaler = DT_INST_PROP(idx, clock_prescaler),	       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),		       \
	};	\
																\
	DEVICE_DT_INST_DEFINE(idx, &pwm_am62x_timer_init, NULL, &pwm_am62x_timer_data_##idx,     \
					&pwm_am62x_timer_config_##idx, POST_KERNEL,	       \
			      	CONFIG_PWM_INIT_PRIORITY,			       \
			      	&pwm_am62x_timer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_AM62X_TIMER_DEFINE)

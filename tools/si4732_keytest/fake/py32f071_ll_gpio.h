/* Enough of the vendor GPIO header to let App/driver/gpio.h compile on a host.
 * The Si4732 key handling never touches a pin; this only exists so the include
 * chain through audio.h resolves. */
#pragma once

#include <stdint.h>

typedef struct { volatile uint32_t DATA; } GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x1000)
#define GPIOB ((GPIO_TypeDef *)0x2000)
#define GPIOC ((GPIO_TypeDef *)0x3000)
#define GPIOF ((GPIO_TypeDef *)0x4000)

#define IOPORT_BASE     0x1000u
#define LL_GPIO_PIN_0   1u
#define LL_GPIO_PIN_1   2u
#define LL_GPIO_PIN_8   256u
#define LL_GPIO_PIN_10  1024u
#define LL_GPIO_PIN_13  8192u

static inline void     LL_GPIO_SetOutputPin  (GPIO_TypeDef *p, uint32_t m) { (void)p; (void)m; }
static inline void     LL_GPIO_ResetOutputPin(GPIO_TypeDef *p, uint32_t m) { (void)p; (void)m; }
static inline uint32_t LL_GPIO_IsInputPinSet (GPIO_TypeDef *p, uint32_t m) { (void)p; (void)m; return 0; }

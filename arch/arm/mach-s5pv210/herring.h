/*
 * arch/arm/mach-s5pv210/herring.h
 */

#include <asm/system.h>
#include <asm/mach-types.h>

#ifndef __HERRING_H__
#define __HERRING_H__

struct uart_port;

void herring_bt_uart_wake_peer(struct uart_port *port);
extern void s3c_setup_uart_cfg_gpio(unsigned char port);

#ifdef CONFIG_MACH_HERRING
# define herring_is_cdma_wimax_dev()	(machine_is_herring() &&	\
						((system_rev & 0xFFF0) == 0x20))
# define herring_is_cdma_wimax_rev(n)	(herring_is_cdma_wimax_dev() &&	\
				(system_rev & 0xF) == ((n) & 0xF))
# define herring_is_cdma_wimax_rev0() herring_is_cdma_wimax_rev(0)
# define herring_is_tft_dev() (machine_is_herring() && (system_rev >= 0x30))
#else
# define herring_is_cdma_wimax_dev() (0)
# define herring_is_cdma_wimax_rev0() (0)
# define herring_is_cdma_wimax_rev(n) (0)
# define herring_is_tft_dev() (0)
#endif

#ifdef CONFIG_PHONE_ARIES_CDMA
# define phone_is_aries_cdma() (1)
#else
# define phone_is_aries_cdma() (0)
#endif

extern struct s5p_panel_data herring_panel_data;
extern struct s5p_tft_panel_data herring_sony_panel_data;
extern struct s5p_tft_panel_data herring_hydis_panel_data;
extern struct s5p_tft_panel_data herring_hitachi_panel_data;
#endif

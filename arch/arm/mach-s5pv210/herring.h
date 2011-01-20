/*
 * arch/arm/mach-s5pv210/herring.h
 */

#ifndef __HERRING_H__
#define __HERRING_H__

struct uart_port;

void herring_bt_uart_wake_peer(struct uart_port *port);
extern void s3c_setup_uart_cfg_gpio(unsigned char port);

extern struct s5p_panel_data herring_panel_data;
extern struct s5p_tft_panel_data herring_sony_panel_data;
extern struct s5p_tft_panel_data herring_hydis_panel_data;
extern struct s5p_tft_panel_data herring_hitachi_panel_data;
#endif

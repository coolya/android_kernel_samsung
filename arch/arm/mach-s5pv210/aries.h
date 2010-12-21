/*
 * arch/arm/mach-s5pv210/aries.h
 */

#ifndef __ARIES_H__
#define __ARIES_H__

struct uart_port;

void aries_bt_uart_wake_peer(struct uart_port *port);
extern void s3c_setup_uart_cfg_gpio(unsigned char port);

extern struct s5p_panel_data aries_panel_data;

#endif

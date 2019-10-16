/*
** Filename: HOST_MIDI_iomux.c
**
** Automatically created by Application Wizard 2.0.2
** 
** Part of solution HOSTMIDI in project HOST_MIDI
**
** Comments: 
**
** Important: Sections between markers "FTDI:S*" and "FTDI:E*" will be overwritten by
** the Application Wizard
*/
#include "vos.h"

void iomux_setup(void)
{
	/* FTDI:SIO IOMux Functions */
	unsigned char packageType;
	
	packageType = vos_get_package_type();
	if (packageType == VINCULUM_II_48_PIN)
	{
		// Debugger to pin 11 as Bi-Directional.
		vos_iomux_define_bidi(199, IOMUX_IN_DEBUGGER, IOMUX_OUT_DEBUGGER);
		vos_iocell_set_config(11, 0, 0, 0, 2);
		//  to pin 12 output disabled.
		vos_iomux_disable_output(12);
		//  to pin 13 output disabled.
		vos_iomux_disable_output(13);
		//  to pin 14 output disabled.
		vos_iomux_disable_output(14);
		// SPI_Slave_0_CLK to pin 15 as Input.
		vos_iomux_define_input(15, IOMUX_IN_SPI_SLAVE_0_CLK);
		vos_iocell_set_config(15, 0, 0, 0, 0);
		// SPI_Slave_0_MOSI to pin 16 as Input.
		vos_iomux_define_input(16, IOMUX_IN_SPI_SLAVE_0_MOSI);
		vos_iocell_set_config(16, 0, 0, 0, 0);
		// SPI_Slave_0_MISO to pin 18 as Output.
		vos_iomux_define_output(18, IOMUX_OUT_SPI_SLAVE_0_MISO);
		vos_iocell_set_config(18, 0, 0, 0, 1);
		// SPI_Slave_0_CS to pin 19 as Input.
		vos_iomux_define_input(19, IOMUX_IN_SPI_SLAVE_0_CS);
		vos_iocell_set_config(19, 0, 0, 0, 2);
		// GPIO_Port_A_0 to pin 20 as Input.
		vos_iomux_define_input(20, IOMUX_IN_GPIO_PORT_A_0);
		vos_iocell_set_config(20, 0, 0, 0, 2);
		// GPIO_Port_A_1 to pin 21 as Input.
		vos_iomux_define_input(21, IOMUX_IN_GPIO_PORT_A_1);
		vos_iocell_set_config(21, 0, 0, 0, 2);
		// GPIO_Port_A_2 to pin 22 as Input.
		vos_iomux_define_input(22, IOMUX_IN_GPIO_PORT_A_2);
		vos_iocell_set_config(22, 0, 0, 0, 2);
		// GPIO_Port_A_3 to pin 23 as Output.
		vos_iomux_define_output(23, IOMUX_OUT_GPIO_PORT_A_3);
		vos_iocell_set_config(23, 0, 0, 0, 2);
		// GPIO_Port_B_0 to pin 31 as Input.
		vos_iomux_define_input(31, IOMUX_IN_GPIO_PORT_B_0);
		vos_iocell_set_config(31, 0, 0, 0, 2);
		// GPIO_Port_B_1 to pin 32 as Input.
		vos_iomux_define_input(32, IOMUX_IN_GPIO_PORT_B_1);
		vos_iocell_set_config(32, 0, 0, 0, 2);
		// GPIO_Port_B_2 to pin 33 as Input.
		vos_iomux_define_input(33, IOMUX_IN_GPIO_PORT_B_2);
		vos_iocell_set_config(33, 0, 0, 0, 2);
		// GPIO_Port_B_3 to pin 34 as Output.
		vos_iomux_define_output(34, IOMUX_OUT_GPIO_PORT_B_3);
		vos_iocell_set_config(34, 0, 0, 0, 2);
		// UART_TXD to pin 35 as Output.
		vos_iomux_define_output(35, IOMUX_OUT_UART_TXD);
		vos_iocell_set_config(35, 0, 0, 0, 2);
		// UART_RXD to pin 36 as Input.
		vos_iomux_define_input(36, IOMUX_IN_UART_RXD);
		vos_iocell_set_config(36, 0, 0, 0, 2);
		//  to pin 38 output disabled.
		vos_iomux_disable_output(38);
		// GPIO_Port_A_4 to pin 41 as Output.
		vos_iomux_define_output(41, IOMUX_OUT_GPIO_PORT_A_4);
		vos_iocell_set_config(41, 0, 0, 0, 1);
		//  to pin 42 output disabled.
		vos_iomux_disable_output(42);
		//  to pin 43 output disabled.
		vos_iomux_disable_output(43);
		//  to pin 44 output disabled.
		vos_iomux_disable_output(44);
		// GPIO_Port_B_4 to pin 45 as Output.
		vos_iomux_define_output(45, IOMUX_OUT_GPIO_PORT_B_4);
		vos_iocell_set_config(45, 0, 0, 0, 1);
		//  to pin 46 output disabled.
		vos_iomux_disable_output(46);

	
	}
	
	/* FTDI:EIO */
}

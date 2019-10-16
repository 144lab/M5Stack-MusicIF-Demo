/*
** Filename: HOST_MIDI.c
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

/* FTDI:SHF Header Files */
#include "USB.h"
#include "USBHost.h"
#include "ioctl.h"
#include "UART.h"
#include "SPISlave.h"
#include "GPIO.h"
#include "FAT.h"
#include "msi.h"
#include "BOMS.h"
#include "FirmwareUpdate.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"
/* FTDI:EHF */

#include "HOST_MIDI.h"


/* FTDI:STP Thread Prototypes */
vos_tcb_t *tcbUSB1;
vos_tcb_t *tcbUSB2;
vos_tcb_t *tcbMIDIO;
vos_tcb_t *tcbMIDII;

void usb1();
void usb2();
void midio();
void midii();
void usb(VOS_HANDLE);

VOS_HANDLE boms_attach(VOS_HANDLE, unsigned char);
VOS_HANDLE fat_attach(VOS_HANDLE, unsigned char);

void StartupDevices(void);

/* FTDI:ETP */

/* FTDI:SDH Driver Handles */
VOS_HANDLE hUSBHOST_1; // USB Host Port 1
VOS_HANDLE hUSBHOST_2; // USB Host Port 2
VOS_HANDLE hSPI_SLAVE_0; // SPISlave Port 0 Interface Driver
VOS_HANDLE hGPIO_PORT_A; // GPIO Port A Driver
VOS_HANDLE hGPIO_PORT_B; // GPIO Port B Driver
VOS_HANDLE hBOMS; // Bulk Only Mass Storage for USB disks
VOS_HANDLE hFAT_FILE_SYSTEM; // FAT File System for FAT32 and FAT16
/* FTDI:EDH */

/*Queue Setting*/
#define QUEUE_SIZE 1024
uint8 queue[QUEUE_SIZE] = {0x00}; //set init value 08Oct2019
uint16 queue_in = 0, queue_out = 0;
vos_mutex_t mQueue;

vos_semaphore_t DevicesStarted;							// signal all drivers are started
VOS_HANDLE clockmaster = NULL;							// store the USB port which transmit MIDI clock



/* Declaration for IOMUx setup function */
void iomux_setup(void);

/* Main code - entry point to firmware */
void main(void)
{
	/* FTDI:SDD Driver Declarations */

	// SPI Slave 0 configuration context
	spislave_context_t spisContext0;
	// GPIO Port A configuration context
	gpio_context_t gpioContextA;
	// GPIO Port B configuration context
	gpio_context_t gpioContextB;
	// USB Host configuration context
	usbhost_context_t usbhostContext;
	/* FTDI:EDD */

	/* FTDI:SKI Kernel Initialisation */
	vos_init(50, VOS_TICK_INTERVAL, VOS_NUMBER_DEVICES);
	vos_set_clock_frequency(VOS_48MHZ_CLOCK_FREQUENCY);
	vos_set_idle_thread_tcb_size(512);
	/* FTDI:EKI */

	iomux_setup();

	/* FTDI:SDI Driver Initialisation */
	
	// Initialise SPI Slave 0
	spisContext0.buffer_size = VOS_BUFFER_SIZE_128_BYTES;
	spislave_init(VOS_DEV_SPI_SLAVE_0,&spisContext0);
	
	// Initialise GPIO A
	gpioContextA.port_identifier = GPIO_PORT_A;
	gpio_init(VOS_DEV_GPIO_PORT_A,&gpioContextA);
	
	// Initialise GPIO B
	gpioContextB.port_identifier = GPIO_PORT_B;
	gpio_init(VOS_DEV_GPIO_PORT_B,&gpioContextB);
	
	// Initialise FAT File System Driver
	fatdrv_init(VOS_DEV_FAT_FILE_SYSTEM);	
	
	// Initialise BOMS Device Driver
	boms_init(VOS_DEV_BOMS);							
	
	// Initialise USB Host
	usbhostContext.if_count = 8;
	usbhostContext.ep_count = 16;
	usbhostContext.xfer_count = 2;
	usbhostContext.iso_xfer_count = 2;
	usbhost_init(VOS_DEV_USBHOST_1, VOS_DEV_USBHOST_2, NULL);
	// Initialise USB Host
	//usbhost_init(VOS_DEV_USBHOST_1, VOS_DEV_USBHOST_2, &usbhostContext);
	
	/* FTDI:EDI */

	/*intialise queue mutex to be unlocked*/
	//vos_init_mutex(&mQueue, 0);
	vos_init_mutex(&mQueue, VOS_MUTEX_UNLOCKED);
    vos_init_semaphore(&DevicesStarted, 0);
	
	/* FTDI:SCT Thread Creation */
	//tcbUSB_IN = vos_create_thread_ex(21, 4096, usb_in, "usb_in", 0);
	//tcbMIDI_OUT = vos_create_thread_ex(20, 4096, midi_out, "midi_out", 0);

	tcbMIDIO = vos_create_thread_ex(22, 1024, midio, "midi_out", 0);
//	tcbMIDII = vos_create_thread_ex(22, 1024, midii, "midi_in", 0);
	tcbUSB1 = vos_create_thread_ex(22, 1024, usb1, "usb_in1", 0);
	tcbUSB2 = vos_create_thread_ex(22, 1024, usb2, "usb_in2", 0);

	/* FTDI:ECT */
	
	vos_start_scheduler();

main_loop:
	goto main_loop;
}


/* FTDI:SSP Support Functions */

void usb1()
{
	vos_wait_semaphore(&DevicesStarted);
    vos_signal_semaphore(&DevicesStarted);
	usb(hUSBHOST_1);
}

void usb2()
{
	vos_wait_semaphore(&DevicesStarted);
    vos_signal_semaphore(&DevicesStarted);
	vos_delay_msecs(500);
	usb(hUSBHOST_2);
}

	
void usb(VOS_HANDLE hUSBHOST)
{
	int i;
	unsigned char status, connectstate, cin;
	usbhost_ioctl_cb_t hc_iocb;							// USB control block
	usbhost_device_handle_ex ifDev = 0;					// USB device handle to the next interface
	usbhost_ioctl_cb_class_t hc_iocb_class;				// USB Class control block
	usbhost_ep_handle_ex epHandle;						// Endpoint handle
	uint8 buf[64], t = 0;								// USB transfer buffer
	usbhost_xfer_t xfer;								// transfer block reference
	vos_semaphore_t semRead;							// used by vos_dev_read
	
	msi_ioctl_cb_t boms_iocb;							// used by firmware update
	fat_ioctl_cb_t fat_ioctl;							// used by firmware update
	FILE *file;											// used by firmware update

	while(1)
	{
		connectstate = PORT_STATE_DISCONNECTED;
		do												// wait for device to connect
		{
			vos_delay_msecs(500);
			hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_GET_CONNECT_STATE;
			hc_iocb.get        = &connectstate;
			vos_dev_ioctl(hUSBHOST, &hc_iocb);			// control request get USB host state
		} while (connectstate != PORT_STATE_ENUMERATED);

		hc_iocb_class.dev_class = USB_CLASS_AUDIO;
		hc_iocb_class.dev_subclass = USB_SUBCLASS_AUDIO_MIDISTREAMING;
		hc_iocb_class.dev_protocol = USB_PROTOCOL_ANY;
		hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_DEVICE_FIND_HANDLE_BY_CLASS;
		hc_iocb.handle.dif = NULL;
		hc_iocb.set = &hc_iocb_class;
		hc_iocb.get = &ifDev;
		vos_dev_ioctl(hUSBHOST, &hc_iocb);				// get class compliant MIDI controller
		
		if (ifDev)
		{
			hc_iocb.handle.dif = ifDev;
			epHandle = NULL;
			hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_DEVICE_GET_BULK_IN_ENDPOINT_HANDLE;
			hc_iocb.get = &epHandle;
			vos_dev_ioctl(hUSBHOST, &hc_iocb);			// get bulk endpoint of MIDI device

			if (epHandle)								// found bulk IN endpoint
			{
				memset(&xfer, 0, sizeof(usbhost_xfer_t));
				vos_init_semaphore(&semRead, 0);
				do										// read MIDI messages
				{
					xfer.buf = buf;
					xfer.len = 64;
					xfer.ep = epHandle;
					xfer.s = &semRead;
					xfer.cond_code = USBHOST_CC_NOTACCESSED;
					xfer.flags = USBHOST_XFER_FLAG_START_BULK_ENDPOINT_LIST;
					status = vos_dev_read(hUSBHOST, (unsigned char *)&xfer, sizeof(usbhost_xfer_t), NULL);
					if ((status == USBHOST_OK) && (xfer.cond_code == USBHOST_CC_NOERROR))
					{									// valid data block received
						for(i=0; i<64; i+=4)			// scan for MIDI messages
						{								// don't care about cable number
							cin=buf[i]&0x0f;			// get code index number
							if(buf[i]==0) i=64;			// empty data block
							
							if((cin==0x05)||(cin==0x0f)) 		// 1-byte-message
							{
								if (buf[i+1] == 0xf8)	// check if midi clock byte [F8]
								{
									if (clockmaster == NULL) clockmaster = hUSBHOST;	// make the local instance to master
									if (clockmaster == hUSBHOST)						// if master send f8
									{
										vos_lock_mutex(&mQueue);
											queue[queue_in] = buf[i+1];
											queue_in = (queue_in+1)%QUEUE_SIZE;
										vos_unlock_mutex(&mQueue);
										i=64; //read 4byte only
									}
								}
								else					// send midi byte if not f8
								{
									vos_lock_mutex(&mQueue);
										queue[queue_in] = buf[i+1];
										queue_in = (queue_in+1)%QUEUE_SIZE;
									vos_unlock_mutex(&mQueue);
									i=64; //read 4byte only
								}
							}
							if((cin==0x02)||(cin==0x06)||(cin==0x0c)||(cin==0x0d)) 	// 2-byte-message
							{
								vos_lock_mutex(&mQueue);
									queue[queue_in] = buf[i+1];
									queue_in = (queue_in+1)%QUEUE_SIZE;
									queue[queue_in] = buf[i+2];
									queue_in = (queue_in+1)%QUEUE_SIZE;
								vos_unlock_mutex(&mQueue);
								i=64; //read 4byte only
							}
							if((cin==0x03)||(cin==0x04)||(cin==0x07)||(cin==0x08)||(cin==0x09)||(cin==0x0a)||(cin==0x0b)||(cin==0x0e)) 	// 3-byte-message
							{
								vos_lock_mutex(&mQueue);
									queue[queue_in] = buf[i+1];
									queue_in = (queue_in+1)%QUEUE_SIZE;
									queue[queue_in] = buf[i+2];
									queue_in = (queue_in+1)%QUEUE_SIZE;
									queue[queue_in] = buf[i+3];
									queue_in = (queue_in+1)%QUEUE_SIZE;
								vos_unlock_mutex(&mQueue);
								i=64; //read 4byte only
							}
						}
					}
				} while(status != USBHOST_NOT_FOUND);	// MIDI device lost
				
				if (clockmaster == hUSBHOST) clockmaster = NULL;	// free clock master
		
				vos_delay_msecs(250);
			}
		}

		hBOMS = boms_attach(hUSBHOST, VOS_DEV_BOMS);	// firmware update procedure
		if (hBOMS != NULL){
			hFAT_FILE_SYSTEM = fat_attach(hBOMS, VOS_DEV_FAT_FILE_SYSTEM);
			if (hFAT_FILE_SYSTEM != NULL){
				fsAttach(hFAT_FILE_SYSTEM);
				file = fopen("usbmidi.rom", "r");	//specified firmware file open
				if (file != NULL){
					FirmwareUpdateFATFileFeedback(file, 0x1f000, NULL);	//to be decided how to know resulting update
					fclose(file);
				}
				fat_ioctl.ioctl_code = FAT_IOCTL_FS_DETACH;
				vos_dev_ioctl(hFAT_FILE_SYSTEM, &fat_ioctl);
				vos_dev_close(hFAT_FILE_SYSTEM);
			}
			boms_iocb.ioctl_code = MSI_IOCTL_BOMS_DETACH;
			vos_dev_ioctl(hBOMS, &boms_iocb);
			vos_dev_close(hBOMS);
			vos_halt_cpu();
		}
		vos_delay_msecs(100);
	}													// get back waiting again for a device
}

void midio()
{
	unsigned short num_written;							// for vos_dev_write
	uint8 midi_byte, i, midi_status = 0;
	uint16 dummy;
	
    StartupDevices();

	while(1)
	{
		vos_lock_mutex(&mQueue);
		dummy = queue_in;
		vos_unlock_mutex(&mQueue);

		if(dummy != queue_out)							// check if something is in the queue
		{
			midi_byte = queue[queue_out];				// get 1 byte out
			queue_out = (queue_out+1)%QUEUE_SIZE;
/*			if ((midi_byte & 0x80) == 0x80)				// midi status byte found
			{
				if (midi_byte != midi_status)			// cut status byte if already active
				{
					midi_status = midi_byte;
	
					vos_dev_write(hSPI_SLAVE_0, &midi_byte, 1, &num_written);	// output MIDI byte
	
				}
			}
			else
			{
*/	
				vos_dev_write(hSPI_SLAVE_0, &midi_byte, 1, &num_written);		// output MIDI byte
	
/*			}
*/
		}
	}
}

void midii()											// hidden DIN MIDI in (not yet in use)
{
	unsigned short num_read;							// for vos_dev_write
	uint8 midi_byte, status, i;
	common_ioctl_cb_t spis_iocb; 						// SPI iocb for getting bytes available
	unsigned short dataAvail = 0;       				// How much data is available to be read?
	
	vos_wait_semaphore(&DevicesStarted);
    vos_signal_semaphore(&DevicesStarted);
	
	vos_delay_msecs(500);
	
	while(1)
	{
		spis_iocb.ioctl_code = VOS_IOCTL_COMMON_GET_RX_QUEUE_STATUS;
		vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);
		dataAvail = spis_iocb.get.queue_stat; 			// How much data to read?        
		if (dataAvail != 0)
		{
			status = vos_dev_read(hSPI_SLAVE_0, &midi_byte, 1, &num_read);
			vos_lock_mutex(&mQueue);
				queue[queue_in] = midi_byte;
				queue_in = (queue_in+1)%QUEUE_SIZE;
			vos_unlock_mutex(&mQueue);
		}
	}
}

void StartupDevices(void) {
	//*****SPI****
	common_ioctl_cb_t spis_iocb;						// SPI control block
	
	hSPI_SLAVE_0 = vos_dev_open(VOS_DEV_SPI_SLAVE_0);					// open SPI_SLAVE[0] device

	spis_iocb.ioctl_code = VOS_IOCTL_SPI_SLAVE_SCK_CPHA;
	spis_iocb.set.param = SPI_SLAVE_SCK_CPHA_1;				// Clk leading edge:Data latch,Trailing edge:Data output
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);

	spis_iocb.ioctl_code = VOS_IOCTL_SPI_SLAVE_SCK_CPOL;
	spis_iocb.set.param = SPI_SLAVE_SCK_CPOL_0;				//SCK low in idle
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);

	spis_iocb.ioctl_code = VOS_IOCTL_SPI_SLAVE_DATA_ORDER;
	spis_iocb.set.param = SPI_SLAVE_DATA_ORDER_MSB;		//Data order: MSB first
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);

	spis_iocb.ioctl_code = VOS_IOCTL_SPI_SLAVE_SET_ADDRESS;
	spis_iocb.set.param = 1;							//May be no need
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);

	spis_iocb.ioctl_code = VOS_IOCTL_SPI_SLAVE_SET_MODE;
	spis_iocb.set.param = SPI_SLAVE_MODE_UNMANAGED;		//SPI Mode
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);
	
	spis_iocb.ioctl_code = VOS_IOCTL_COMMON_ENABLE_DMA;	// DMA enable for SPI_SLAVE
	vos_dev_ioctl(hSPI_SLAVE_0, &spis_iocb);
	
	//*****GPIO****
    hGPIO_PORT_A = vos_dev_open(VOS_DEV_GPIO_PORT_A);	// open GPIO device
	hGPIO_PORT_B = vos_dev_open(VOS_DEV_GPIO_PORT_B);

	vos_gpio_set_pin_mode(GPIO_A_3, 1); 				// configure pin as 1=output
	vos_gpio_set_pin_mode(GPIO_B_3, 1); 				// configure pin as 1=output
	vos_gpio_set_pin_mode(GPIO_A_4, 1); 				// configure pin as 1=output
	vos_gpio_set_pin_mode(GPIO_B_4, 1); 				// configure pin as 1=output

	vos_gpio_write_pin(GPIO_A_3, 1);					// Vbus Enable	
	vos_gpio_write_pin(GPIO_B_3, 1);					// Vbus Enable	
	vos_gpio_write_pin(GPIO_A_4, 0);					// CC Pulled Up	
	vos_gpio_write_pin(GPIO_B_4, 0);					// CC Pulled Up	

	//*****USB****
	
	hUSBHOST_1 = vos_dev_open(VOS_DEV_USBHOST_1);		// open USB host 1 device
	hUSBHOST_2 = vos_dev_open(VOS_DEV_USBHOST_2);		// open USB host 2 device

	// Let other threads know that devices are initialized
    vos_signal_semaphore(&DevicesStarted);				
}

VOS_HANDLE boms_attach(VOS_HANDLE hUSB, unsigned char devBOMS)
{
	usbhost_device_handle_ex ifDisk;
	usbhost_ioctl_cb_t hc_iocb;
	usbhost_ioctl_cb_class_t hc_iocb_class;
	msi_ioctl_cb_t boms_iocb;
	boms_ioctl_cb_attach_t boms_att;
	VOS_HANDLE hBOMS;

	// find BOMS class device
	hc_iocb_class.dev_class = USB_CLASS_MASS_STORAGE;
	hc_iocb_class.dev_subclass = USB_SUBCLASS_MASS_STORAGE_SCSI;
	hc_iocb_class.dev_protocol = USB_PROTOCOL_MASS_STORAGE_BOMS;

	// user ioctl to find first hub device
	hc_iocb.ioctl_code = VOS_IOCTL_USBHOST_DEVICE_FIND_HANDLE_BY_CLASS;
	hc_iocb.handle.dif = NULL;
	hc_iocb.set = &hc_iocb_class;
	hc_iocb.get = &ifDisk;

	if (vos_dev_ioctl(hUSB, &hc_iocb) != USBHOST_OK)
	{
		return NULL;
	}

	// now we have a device, intialise a BOMS driver with it
	hBOMS = vos_dev_open(devBOMS);

	// perform attach
	boms_att.hc_handle = hUSB;
	boms_att.ifDev = ifDisk;

	boms_iocb.ioctl_code = MSI_IOCTL_BOMS_ATTACH;
	boms_iocb.set = &boms_att;
	boms_iocb.get = NULL;

	if (vos_dev_ioctl(hBOMS, &boms_iocb) != MSI_OK)
	{
		vos_dev_close(hBOMS);
		hBOMS = NULL;
	}

	return hBOMS;
}

VOS_HANDLE fat_attach(VOS_HANDLE hMSI, unsigned char devFAT)
{
	fat_ioctl_cb_t fat_ioctl;
	fatdrv_ioctl_cb_attach_t fat_att;
	VOS_HANDLE hFAT;

	// currently the MSI (BOMS or other) must be open
	// open the FAT driver
	hFAT = vos_dev_open(devFAT);

	// attach the FAT driver to the MSI driver
	fat_ioctl.ioctl_code = FAT_IOCTL_FS_ATTACH;
	fat_ioctl.set = &fat_att;
	fat_att.msi_handle = hMSI;
	fat_att.partition = 0;

	if (vos_dev_ioctl(hFAT, &fat_ioctl) != FAT_OK)
	{
		// unable to open the FAT driver
		vos_dev_close(hFAT);
		hFAT = NULL;
	}

	return hFAT;
}


	
/* FTDI:ESP */



/* Application Threads */


	
/*	

/******************************************************************************
* Copyright (C) 2010 - 2022 Xilinx, Inc.  All rights reserved.
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xusbps_intr_example.c
*
* This file contains an example of how to use the USB driver with the USB
* controller in DEVICE mode.
*
*
*<pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- ----------------------------------------------------
* 1.00a wgr/nm  10/09/10 First release
* 1.01a nm      03/05/10 Included xpseudo_asm.h instead of xpseudo_asm_gcc.h
* 1.04a nm      02/05/13 Fixed CR# 696550.
*		         Added template code for Vendor request.
* 1.06a kpc		11/11/13 Fixed CR#759458, cacheInvalidate size should be
*				 ailgned to cache line size.
* 2.1   kpc    04/28/14 Cleanup and removed unused functions
* 2.4   vak    04/01/19 Fixed IAR data_alignment warnings
* 2.8   pm     07/07/23 Added support for system device-tree flow
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/

#include "xparameters.h"		/* XPAR parameters */
#include "xusbps.h"			/* USB controller driver */
#include "xusbps_ch9.h"		/* Generic Chapter 9 handling code */
#include "xusbps_class_storage.h"	/* Storage class handling code */
#include "xil_exception.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"
#include "xil_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE (64 * 1024)
u8 Buffer[MEMORY_SIZE] ALIGNMENT_CACHELINE;
static volatile int NumIrqs = 0;
static volatile int NumReceivedFrames = 0;

static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum,
				   u8 EventType, void *Data);
static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum,
				   u8 EventType, void *Data);

int UsbStorage(XUsbPs *UsbInstancePtr, XUsbPs_Config* UsbConfigPtr)
{
	int	Status;
	u8	*MemPtr = NULL;
	int	ReturnStatus = XST_FAILURE;

	/* For this example we only configure 2 endpoints:
	 *   Endpoint 0 (default control endpoint)
	 *   Endpoint 1 (BULK data endpoint)
	 */
	const u8 NumEndpoints = 2;

	XUsbPs_DeviceConfig	DeviceConfig;

	/* Configuration of the DEVICE side of the controller happens in
	 * multiple stages.
	 *
	 * 1) The user configures the desired endpoint configuration using the
	 * XUsbPs_DeviceConfig data structure. This includes the number of
	 * endpoints, the number of Transfer Descriptors for each endpoint
	 * (each endpoint can have a different number of Transfer Descriptors)
	 * and the buffer size for the OUT (receive) endpoints.  Each endpoint
	 * can have different buffer sizes.
	 *
	 * 2) Request the required size of DMAable memory from the driver using
	 * the XUsbPs_DeviceMemRequired() call.
	 *
	 * 3) Allocate the DMAable memory and set up the DMAMemVirt and
	 * DMAMemPhys members in the XUsbPs_DeviceConfig data structure.
	 *
	 * 4) Configure the DEVICE side of the controller by calling the
	 * XUsbPs_ConfigureDevice() function.
	 */

	/*
	 * For this example we only configure Endpoint 0 and Endpoint 1.
	 *
	 * Bufsize = 0 indicates that there is no buffer allocated for OUT
	 * (receive) endpoint 0. Endpoint 0 is a control endpoint and we only
	 * receive control packets on that endpoint. Control packets are 8
	 * bytes in size and are received into the Queue Head's Setup Buffer.
	 * Therefore, no additional buffer space is needed.
	 */
	DeviceConfig.EpCfg[0].Out.Type		= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].Out.NumBufs	= 2;
	DeviceConfig.EpCfg[0].Out.BufSize	= 64;
	DeviceConfig.EpCfg[0].Out.MaxPacketSize	= 64;
	DeviceConfig.EpCfg[0].In.Type		= XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].In.NumBufs	= 2;
	DeviceConfig.EpCfg[0].In.MaxPacketSize	= 64;

	DeviceConfig.EpCfg[1].Out.Type		= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].Out.NumBufs	= 16;
	DeviceConfig.EpCfg[1].Out.BufSize	= 512;
	DeviceConfig.EpCfg[1].Out.MaxPacketSize	= 512;
	DeviceConfig.EpCfg[1].In.Type		= XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].In.NumBufs	= 16;
	DeviceConfig.EpCfg[1].In.MaxPacketSize	= 512;

	DeviceConfig.NumEndpoints = NumEndpoints;

	MemPtr = (u8 *)&Buffer[0];
	memset(MemPtr, 0, MEMORY_SIZE);
	Xil_DCacheFlushRange((unsigned int)MemPtr, MEMORY_SIZE);

	/* Finish the configuration of the DeviceConfig structure and configure
	 * the DEVICE side of the controller.
	 */
	DeviceConfig.DMAMemPhys = (u32) MemPtr;

	Status = XUsbPs_ConfigureDevice(UsbInstancePtr, &DeviceConfig);
	if (XST_SUCCESS != Status) {
		goto out;
	}

	/* Set the handler for handling endpoint 0 events. This is where we
	 * will receive and handle the Setup packet from the host.
	 */
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 0,
				     XUSBPS_EP_DIRECTION_OUT,
				     XUsbPs_Ep0EventHandler, UsbInstancePtr);

	/* Set the handler for handling endpoint 1 events.
	 *
	 * Note that for this example we do not need to register a handler for
	 * TX complete events as we only send data using static data buffers
	 * that do not need to be free()d or returned to the OS after they have
	 * been sent.
	 */
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 1,
				     XUSBPS_EP_DIRECTION_OUT,
				     XUsbPs_Ep1EventHandler, UsbInstancePtr);

	XUsbPs_IntrEnable(UsbInstancePtr, XUSBPS_IXR_UR_MASK |
			  XUSBPS_IXR_UI_MASK);

	/* Start the USB engine */
	XUsbPs_Start(UsbInstancePtr);

	ReturnStatus = XST_SUCCESS;

	out:
	return ReturnStatus;
}

/*****************************************************************************/
/**
* This function is registered to handle callbacks for endpoint 0 (Control).
*
* It is called from an interrupt context such that the amount of processing
* performed should be minimized.
*
*
* @param	CallBackRef is the reference passed in when the function
*		was registered.
* @param	EpNum is the Number of the endpoint on which the event occurred.
* @param	EventType is type of the event that occurred.
*
* @return	None.
*
******************************************************************************/
static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum,
				   u8 EventType, void *Data)
{
	XUsbPs			*InstancePtr;
	int			Status;
	XUsbPs_SetupData	SetupData;
	u8	*BufferPtr;
	u32	BufferLen;
	u32	Handle;


	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *) CallBackRef;

	switch (EventType) {

		/* Handle the Setup Packets received on Endpoint 0. */
		case XUSBPS_EP_EVENT_SETUP_DATA_RECEIVED:
			Status = XUsbPs_EpGetSetupData(InstancePtr, EpNum, &SetupData);
			if (XST_SUCCESS == Status) {
				/* Handle the setup packet. */
				(int) XUsbPs_Ch9HandleSetupPacket(InstancePtr,
								  &SetupData);
			}
			break;

		/* We get data RX events for 0 length packets on endpoint 0. We receive
		 * and immediately release them again here, but there's no action to be
		 * taken.
		 */
		case XUSBPS_EP_EVENT_DATA_RX:
			/* Get the data buffer. */
			Status = XUsbPs_EpBufferReceive(InstancePtr, EpNum,
							&BufferPtr, &BufferLen, &Handle);
			if (XST_SUCCESS == Status) {
				/* Return the buffer. */
				XUsbPs_EpBufferRelease(Handle);
			}
			break;

		default:
			/* Unhandled event. Ignore. */
			break;
	}
}


/*****************************************************************************/
/**
* This function is registered to handle callbacks for endpoint 1 (Bulk data).
*
* It is called from an interrupt context such that the amount of processing
* performed should be minimized.
*
*
* @param	CallBackRef is the reference passed in when the function was
*		registered.
* @param	EpNum is the Number of the endpoint on which the event occurred.
* @param	EventType is type of the event that occurred.
*
* @return	None.
*
* @note 	None.
*
******************************************************************************/
static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum,
				   u8 EventType, void *Data)
{
	XUsbPs *InstancePtr;
	int Status;
	u8	*BufferPtr;
	u32	BufferLen;
	u32 InavalidateLen;
	u32	Handle;


	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *) CallBackRef;

	switch (EventType) {
		case XUSBPS_EP_EVENT_DATA_RX:
			/* Get the data buffer.*/
			Status = XUsbPs_EpBufferReceive(InstancePtr, EpNum,
							&BufferPtr, &BufferLen, &Handle);
			/* Invalidate the Buffer Pointer */
			InavalidateLen =  BufferLen;
			if (BufferLen % 32) {
				InavalidateLen = (BufferLen / 32) * 32 + 32;
			}

			Xil_DCacheInvalidateRange((unsigned int)BufferPtr,
						  InavalidateLen);
			if (XST_SUCCESS == Status) {
				/* Handle the storage class request. */
				XUsbPs_HandleStorageReq(InstancePtr, EpNum,
							BufferPtr, BufferLen);
				/* Release the buffer. */
				XUsbPs_EpBufferRelease(Handle);
			}
			break;

		default:
			/* Unhandled event. Ignore. */
			break;
	}
}

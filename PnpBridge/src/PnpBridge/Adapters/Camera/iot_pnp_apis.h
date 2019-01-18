#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <DiscoveryAdapterInterface.h>
#include <PnpAdapterInterface.h>
#include <common.h>

#include "pch.h"

// Camera discovery API entry points.
int
CameraPnpStartDiscovery(
    _In_ PNPBRIDGE_NOTIFY_DEVICE_CHANGE DeviceChangeCallback,
    _In_ JSON_Object* deviceArgs, 
    _In_ JSON_Object* adapterArgs
    );

int 
CameraPnpStopDiscovery(
    );

// Interface functions.
int
CameraPnpInterfaceInitialize(
    _In_ JSON_Object* adapterArgs
    );

int
CameraPnpInterfaceShutdown(
    );

int
CameraPnpInterfaceBind(
    _In_ PNPADAPTER_INTERFACE_HANDLE Interface, 
    _In_ PNP_DEVICE_CLIENT_HANDLE pnpDeviceClientHandle, 
    _In_ PPNPBRIDGE_DEVICE_CHANGE_PAYLOAD payload
    );

int
CameraPnpInterfaceRelease(
    _In_ PNPADAPTER_INTERFACE_HANDLE Interface
    );

#ifdef __cplusplus
}
#endif

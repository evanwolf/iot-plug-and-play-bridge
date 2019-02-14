#include "pch.h"
#include "CameraIotPnpAPIs.h"

DISCOVERY_ADAPTER CameraPnpAdapter = {
    "camera-health-monitor",
    CameraPnpStartDiscovery,
    CameraPnpStopDiscovery
};

PNP_ADAPTER CameraPnpInterface = {
    "camera-health-monitor",
    CameraPnpInterfaceInitialize,
    CameraPnpInterfaceBind,
    CameraPnpInterfaceShutdown
};

// Camera discovery API entry points.
CameraPnpDiscovery*  g_pPnpDiscovery = nullptr;

int
CameraPnpStartDiscovery(
    _In_ PNPBRIDGE_NOTIFY_DEVICE_CHANGE DeviceChangeCallback,
    _In_ const char* deviceArgs,
    _In_ const char* adapterArgs
    )
{
    // Avoid using unique_ptr here, we want to play some pointer
    // swapping games...
    CameraPnpDiscovery* p = new CameraPnpDiscovery();

    if (nullptr != InterlockedCompareExchangePointer((PVOID*)&g_pPnpDiscovery, (PVOID)p, nullptr))
    {
        delete p;
        p = g_pPnpDiscovery;
    }

    RETURN_IF_FAILED (p->InitializePnpDiscovery(DeviceChangeCallback, deviceArgs, adapterArgs));

    return 0;
}

int 
CameraPnpStopDiscovery(
    )
{
    CameraPnpDiscovery* p = (CameraPnpDiscovery*) InterlockedExchangePointer((PVOID*)&g_pPnpDiscovery, nullptr);

    if (nullptr != p)
    {
        p->Shutdown();
        delete p;
        p = nullptr;
    }

    return 0;
}

// Interface bind functions.
int
CameraPnpInterfaceInitialize(
    _In_ const char* adapterArgs
    )
{
    return S_OK;
}

int
CameraPnpInterfaceShutdown(
    )
{
    return S_OK;
}

int
CameraPnpInterfaceBind(
    _In_ PNPADAPTER_CONTEXT adapterHandle,
    _In_ PNP_DEVICE_CLIENT_HANDLE pnpDeviceClientHandle,
    _In_ PPNPBRIDGE_DEVICE_CHANGE_PAYLOAD payload
    )
{
    CameraPnpDiscovery*                 p = nullptr;
    std::wstring                        cameraName;
    std::unique_ptr<CameraIotPnpDevice> pIotPnp;
    PNP_INTERFACE_CLIENT_HANDLE         pnpInterfaceClient;
    const char*                         interfaceId;
    JSON_Value*                         jmsg;
    JSON_Object*                        jobj;

	PNPADAPTER_INTERFACE_HANDLE adapterInterface = nullptr;

    if (nullptr == payload || nullptr == payload->Context)
    {
        return E_INVALIDARG;
    }

    p = (CameraPnpDiscovery*)payload->Context;
    RETURN_IF_FAILED (p->GetFirstCamera(cameraName));

    jmsg =  json_parse_string(payload->Message);
    jobj = json_value_get_object(jmsg);

    interfaceId = "http://windows.com/camera_health_monitor/v1"; //json_object_get_string(jobj, "InterfaceId");
    pnpInterfaceClient = PnP_InterfaceClient_Create(pnpDeviceClientHandle, interfaceId, nullptr, nullptr, nullptr);
    RETURN_HR_IF_NULL (E_UNEXPECTED, pnpInterfaceClient);

    // Create PnpAdapter Interface
    PNPADPATER_INTERFACE_INIT_PARAMS interfaceParams = { 0 };
    interfaceParams.releaseInterface = CameraPnpInterfaceRelease;

    RETURN_HR_IF (E_UNEXPECTED, 0 != PnpAdapterInterface_Create(adapterHandle, interfaceId, pnpInterfaceClient, &adapterInterface, &interfaceParams))

    pIotPnp = std::make_unique<CameraIotPnpDevice>();
    RETURN_IF_FAILED (pIotPnp->Initialize(PnpAdapterInterface_GetPnpInterfaceClient(adapterInterface), nullptr /* cameraName.c_str() */));
    RETURN_IF_FAILED (pIotPnp->StartTelemetryWorker());

    RETURN_HR_IF (E_UNEXPECTED, 0 != PnpAdapterInterface_SetContext(adapterInterface, (void*)pIotPnp.get()));

    // Our interface context now owns the object.
    pIotPnp.release();

    return S_OK;
}

int
CameraPnpInterfaceRelease(
    _In_ PNPADAPTER_INTERFACE_HANDLE Interface
    )
{
    CameraIotPnpDevice* p = (CameraIotPnpDevice*)PnpAdapterInterface_GetContext(Interface);

    if (p != nullptr)
    {
        // This will block until the worker is stopped.
        (VOID) p->StopTelemetryWorker();
        p->Shutdown();

        delete p;
    }

    // Clear our context to make sure we don't keep a stale
    // object around.
    (void) PnpAdapterInterface_SetContext(Interface, nullptr);

    PnpAdapterInterface_Destroy(Interface);

    return S_OK;
}


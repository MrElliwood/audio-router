#include <Windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <string>
#include <cassert>
#include "main.h"
#include "patcher.h"
#include "patch.h"
#include "common.h"

#pragma comment(lib, "uuid.lib")

// TODO/audiorouterdev: unify constants and structures between dll and gui
#define ROUTING_MASK (((DWORD)0x3) << (sizeof(DWORD) * 8 - 2))
#define SESSION_MASK ((~((DWORD)0)) >> 2)
#define GET_ROUTING_FLAG(a) (a >> (sizeof(DWORD) * 8 - 2))

#ifndef SAFE_RELEASE
# define SAFE_RELEASE(x) assert(strcmp("main.cpp: SAFE_RELEASE is not defined, check if wtl.h access", "") == 0)
#endif

/**
*	Definition of device_id_t struct
*/
struct device_id_t
{
    LPWSTR device_id_str;
	/**
	*	Constructor of device_id_t struct.
	*
	*	@param device_id_str: Its type is LPWSTR which is a string (WCHAR *). This is the device name.
	*/
    device_id_t(LPWSTR device_id_str) : device_id_str(device_id_str) {}

	/**
	*	Release a device_id_t member function.
	*	It release memory when out of scope.
	*/
    void Release()
    {
        delete[] this->device_id_str;
        delete this;
    }
};

typedef duplicate<device_id_t> device_id_duplicate;
typedef HRESULT (__stdcall * activate_t)(IMMDevice *, REFIID, DWORD, PROPVARIANT *, void **);
HRESULT __stdcall activate_patch(IMMDevice *, REFIID, DWORD, PROPVARIANT *, void **);

bool apply_explicit_routing();

patcher<activate_t> patch_activate(activate_patch);
CRITICAL_SECTION CriticalSection;
DWORD session_flag;
device_id_duplicate *device_ids = NULL;

// TODO/audiorouterdev: streamline device id parameter applying

/**
*	DllMain is the main entry point for this dll.
*
*	@param hinstDLL: Its type is HINSTANCE. Not used.
*	@param fdwReason: Its type is DWORD which is typedef for unsigned long.
*	Its value is matched against the following: DLL_PROCESS_ATTACH, DLL_THREAD_ATTACH,  DLL_PROCESS_DETACH
*	@param lpvReserved: Its type is LPVOID which is typedef for void *. Not used.
*	@returns BOOL: This is typedef for int. It returns TRUE(which means that the dll is to kept).
*	Otherwise we decrease the reference counter. This is part of collection process.
*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        HRESULT hr;
        IMMDeviceEnumerator *pEnumerator;
        IMMDevice *pDevice;

        if ((hr = CoInitialize(NULL)) != S_OK) {
            return FALSE;
        }

        if ((hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator)) != S_OK)
        {
            return FALSE;
        }

        if ((hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice)) != S_OK) {
            pEnumerator->Release();
            return FALSE;
        }

        DWORD *patch_activate_ptr = ((DWORD ***)pDevice)[0][3];

        pDevice->Release();
        pEnumerator->Release();

        InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400);
        patch_activate.patch(patch_activate_ptr);

        // patch only after the unloading of the library cannot happen
        if (!apply_explicit_routing() && !apply_implicit_routing()) {
            // (patches are automatically reverted)
            return FALSE;
        }
    }
    else if (fdwReason == DLL_THREAD_ATTACH) {
        apply_explicit_routing();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) {
        // TODO/audiorouterdev: include critical sections in the patcher
        // TODO/audiorouterdev: decide if delete device id str(practically does not have any effect)
        // (patches are automatically reverted)
        DeleteCriticalSection(&CriticalSection);
    }

    return TRUE;
} // DllMain

/**
*	Definition of apply_parameters function
*	
*	@param params(reference): Its type is local_routing_params 
*	@returns bool: It returns true if params.device_id_ptr is not NULL or params.session_guid_and_flag & ROUTING_MASK is zero.
*/
bool apply_parameters(const local_routing_params& params)
{
    const DWORD proc_id = GetCurrentProcessId();
    const DWORD target_id = params.pid;

    if (proc_id != target_id) {
        return false;
    }

    const DWORD session_flag = params.session_guid_and_flag;

    if ((session_flag & ROUTING_MASK) == 0) {
        // critical section necessary in case if another thread is currently
        // performing initialization
        EnterCriticalSection(&CriticalSection);

        patch_activate.revert();
        SafeDelete(device_ids);
        LeaveCriticalSection(&CriticalSection);

        return true;
    }
    else if (params.device_id_ptr) {
        // critical section necessary in case if another thread is currently
        // performing initialization
        EnterCriticalSection(&CriticalSection);

        ::session_flag = session_flag;

        std::wstring device_id;

        if (GET_ROUTING_FLAG(session_flag) == 1) {
            SafeDelete(device_ids);
        }

        for (const wchar_t *p = (const wchar_t *)params.device_id_ptr; *p; p++) {
            device_id += *p;
        }

        WCHAR *device_id_str = new WCHAR[device_id.size() + 1];
        memcpy(device_id_str, device_id.c_str(), device_id.size() * sizeof(WCHAR));
        device_id_str[device_id.size()] = 0;

        if (device_ids == NULL) {
            device_ids = new device_id_duplicate(new device_id_t(device_id_str));
        }
        else {
            device_ids->add(new device_id_t(device_id_str));
        }

        patch_activate.apply();

        LeaveCriticalSection(&CriticalSection);

        return true;
    }

    return false;
} // apply_parameters

/**
*	Definition of apply_explicit_routing
*	Checks to be done before loading audio router dll file.
*	
*	@returns bool which is true if audio dll is found , otherwise false.
*/
bool apply_explicit_routing()
{
    CHandle hfile(OpenFileMappingW(FILE_MAP_READ, FALSE, L"Local\\audio-router-file"));
    if (hfile == NULL) {
        return false;
    }

    unsigned char *buffer = (unsigned char *)MapViewOfFile(hfile, FILE_MAP_COPY, 0, 0, 0);
    if (buffer == NULL) {
        return false;
    }

    global_routing_params *params = rebase(buffer);
    const bool ret = apply_parameters(params->local);
    UnmapViewOfFile(buffer);
    return ret;
} // apply_explicit_routing

/**
*	Definition of activate_patch function
*	this_ object's method Activate uses the params to make function call which long
*	It would first try to use default, otherwise try other options that would return none zero value.
*
*	@param this_(pointer): Its type is IMMDevice struct.
*	@param iid: Its type is REFIID which is alias for const IID reference.
*	@param dwClsCtx: Its type is DWORD which is typedef for unsigned long.
*	@param pActivationParams: Its type is PROPVARIANT which is typedef for tagPROPVARIANT.
*	@param ppInterface(pointer to pointer): Its type is void.
*	@returns HRESULT is typedef long, if it 0x004 it is invalid error code.
*/
HRESULT __stdcall activate_patch(IMMDevice *this_, REFIID iid, DWORD dwClsCtx,  PROPVARIANT *pActivationParams, void **ppInterface)
{
    EnterCriticalSection(&CriticalSection);
    patch_activate.revert();

    // use default since default audio device has been requested
    if (device_ids == NULL) {
        HRESULT hr = this_->Activate(iid, dwClsCtx, pActivationParams, ppInterface);

        // patch_activate.apply();
        LeaveCriticalSection(&CriticalSection);
        return hr;
    }

    // // 7ED4EE07-8E67-4CD4-8C1A-2B7A5987AD42
    // if(iid != __uuidof(IAudioClient))
    // {
    //    HRESULT hr = this_->Activate(iid, dwClsCtx, pActivationParams, ppInterface);
    //    patch_activate.apply();
    //    LeaveCriticalSection(&CriticalSection);
    //    return hr;
    // }

    {
        HRESULT hr;
        IMMDeviceEnumerator *pEnumerator = NULL;
        IMMDeviceCollection *pDevices = NULL;
        IMMDevice *pDevice = NULL;

        if ((hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
            __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator)) != S_OK)
        {
            SAFE_RELEASE(pEnumerator);
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        if ((hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices)) != S_OK) {
            SAFE_RELEASE(pDevices);
            pEnumerator->Release();
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        UINT count;
        if ((hr = pDevices->GetCount(&count)) != S_OK) {
            pDevices->Release();
            pEnumerator->Release();
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        LPWSTR dev_id = NULL;
        this_->GetId(&dev_id);

        bool endpoint_found = dev_id ? false : true;
        for (ULONG i = 0; i < count; i++) {
            IMMDevice *pEndpoint = NULL;
            pDevices->Item(i, &pEndpoint);

            // Get the endpoint ID string.
            LPWSTR pwszID;
            pEndpoint->GetId(&pwszID);

            if (!pDevice && wcscmp(device_ids->proxy->device_id_str, pwszID) == 0) {
                pDevice = pEndpoint;
            }

            // check if the this_ endpoint device is in render group
            if (!endpoint_found && wcscmp(pwszID, dev_id) == 0) {
                endpoint_found = true;
            }

            CoTaskMemFree(pwszID);
            if (!pDevice) pEndpoint->Release();
        }

        CoTaskMemFree(dev_id);

        if (!endpoint_found) {
            HRESULT hr = this_->Activate(iid, dwClsCtx, pActivationParams, ppInterface);
            SAFE_RELEASE(pDevice);
            pDevices->Release();
            pEnumerator->Release();
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        // TODO/audiorouterdev: device id might be invalid in case if the endpoint has been disconnected

        /*if(cont)
            MessageBoxA(
            NULL, "Endpoint device was not found during routing process.",
            NULL, MB_ICONERROR);*/

        if (!pDevice ||
            (hr = pDevice->Activate(iid, dwClsCtx, pActivationParams, ppInterface)) != S_OK)
        {
            if (!pDevice)
                hr = AUDCLNT_E_DEVICE_INVALIDATED;

            SAFE_RELEASE(pDevice);
            pDevices->Release();
            pEnumerator->Release();
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        pDevice->Release();
        pDevices->Release();
        pEnumerator->Release();

        // (metro apps seem to use an undocumented interface)
        if (iid != __uuidof(IAudioClient)) {
            // the interface is initialized for another device which allows for routing,
            // but duplication will not be available since the interface is unknown
            patch_activate.apply();
            LeaveCriticalSection(&CriticalSection);
            return hr;
        }

        GUID *guid = new GUID;
        ZeroMemory(guid, sizeof(GUID));
        const DWORD session_guid = session_flag & SESSION_MASK;
        size_t starting_byte = 7;
        memcpy(((char *)guid) + starting_byte, &session_guid, sizeof(session_guid));

        patch_iaudioclient((IAudioClient *)*ppInterface, guid);
    }

    // add additional devices
    for (device_id_duplicate *next = device_ids->next; next != NULL; next = next->next) {
        IMMDeviceEnumerator *pEnumerator = NULL;
        IMMDeviceCollection *pDevices = NULL;
        IMMDevice *pDevice = NULL;
        IAudioClient *pAudioClient = NULL;

        if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator) != S_OK)
        {
            SAFE_RELEASE(pEnumerator);
            continue;
        }

        if (pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices) != S_OK) {
            SAFE_RELEASE(pDevices);
            pEnumerator->Release();
            continue;
        }

        UINT count;
        if (pDevices->GetCount(&count) != S_OK) {
            pDevices->Release();
            pEnumerator->Release();
            continue;
        }

        bool endpoint_found = false;
        IMMDevice *pEndpoint = NULL;
        for (ULONG i = 0; i < count && !endpoint_found; i++) {
            pDevices->Item(i, &pEndpoint);

            // Get the endpoint ID string.
            LPWSTR pwszID;
            pEndpoint->GetId(&pwszID);

            if (wcscmp(next->proxy->device_id_str, pwszID) == 0) {
                endpoint_found = true;
            }

            CoTaskMemFree(pwszID);
            if (!endpoint_found) {
                pEndpoint->Release();
            }
        }

        pDevice = pEndpoint;

        // TODO/audiorouterdev: device id might be invalid in case if the endpoint has been disconnected
        if (!endpoint_found) {
            MessageBoxA(NULL, "Endpoint device was not found during routing process.", NULL, MB_ICONERROR);
            pDevices->Release();
            pEnumerator->Release();
            continue;
        }
        else {
            // Device found, attempt to activate it.
            if (pDevice->Activate(iid, dwClsCtx, pActivationParams, (void **)&pAudioClient) != S_OK) {
                SAFE_RELEASE(pAudioClient);
                continue;
            }
            else {
                /*GUID* guid = new GUID;
                ZeroMemory(guid, sizeof(GUID));
                const DWORD session_guid = session_flag & SESSION_MASK;
                size_t starting_byte = 7;
                memcpy(((char*)guid) + starting_byte, &session_guid, sizeof(session_guid));*/

                get_duplicate((IAudioClient *)*ppInterface)->add(pAudioClient);
            }
            pDevice->Release();
            pDevices->Release();
            pEnumerator->Release();
        }
    }

    patch_activate.apply();
    LeaveCriticalSection(&CriticalSection);

    return S_OK;
} // activate_patch

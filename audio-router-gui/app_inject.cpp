/** 
 * app_inject.cpp
 * Purpose: ________.
 
 * @author ...
 * @version 0.0 00/00/2000
 */

#include "app_inject.h"
#include "wtl.h"
#include "policy_config.h"
#include "util.h"
#include "routing_params.h"
#include "..\audio-router\common.h"
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <cassert>

/** 
*	Symbolic Name and Abbrevation
*	Passes the argument guid, flag and returns result under the following formulation.
*/
#define MAKE_SESSION_GUID_AND_FLAG(guid, flag)\
    ((((DWORD)flag) << (sizeof(DWORD) * 8 - 2)) | (((DWORD)guid) & ~(3 << (sizeof(DWORD) * 8 - 2))))

/**
*	SESSION_GUID_BEGIN a constant with value 5
*/
#define SESSION_GUID_BEGIN /*8*/ 5

DWORD app_inject::session_guid = 1 << SESSION_GUID_BEGIN; /**> Value of session_guid */

/** 
*	get_session_guid_flag is a app_inject member function.
*
*	@param duplicate: Its type is bool.
*	@param saved_routing: Its type is bool.
*	@returns DWORD with incremented Session guid and a bool.
*/
DWORD app_inject::get_session_guid_and_flag(bool duplicate, bool saved_routing)
{
    return MAKE_SESSION_GUID_AND_FLAG(session_guid++, duplicate ? 2 : 1);

    // if(!saved_routing)
    //    return MAKE_SESSION_GUID_AND_FLAG(session_guid++, duplicate ? 2 : 1);
    // else
    // {
    //    const DWORD mod = (1 << 31) >> (SESSION_GUID_BEGIN + 2); // +2 because flags are included
    //    const DWORD guid = (DWORD)(rand() % mod) << (SESSION_GUID_BEGIN + 1);
    //    return MAKE_SESSION_GUID_AND_FLAG(guid, duplicate ? 2 : 1);
    // }
}

/**
*	Constructor for app_inject class.
*
*/
app_inject::app_inject()
{}

/** 
*	clear_devices is a app_inject member function
*	clear_devices: Releases the present devices and clears the list.
*	@param devices: Call by Reference i.e The static devices list is refrenced to its original location. 
*	@returns void.
*/
void app_inject::clear_devices(devices_t& devices)
{
    for (size_t i = 0; i < devices.size(); i++) {       /**> Traverse through all the devices */
        if (devices[i] != NULL) {                       /**> If Devices are present then it gets released */
            devices[i]->Release();
        }
    }
Member Function
    devices.clear();                                    /**> The list is cleared after traversing through */
}

/** 
*	get_devices is a app_inject member function.
*	get_devices.
* 
*	@returns void.
*/
void app_inject::get_devices(devices_t& devices)
{
    clear_devices(devices);                         /**> Calling Member function clear_devices */

    IMMDeviceEnumerator *pEnumerator;               /**> Pointer Variable of type IMMDeviceEnumerator */
    IMMDeviceCollection *pDevices;                  /**> Pointer Variable of type IMMDeviceCollection */

    if (CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, /**> ... */
        __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator) != S_OK)
    {
        SAFE_RELEASE(pEnumerator);                  /**> If true then calling SAFE_RELEASE with pEnumerator */
        return;
    }

    if (pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices) != S_OK) { /**> ... */
        SAFE_RELEASE(pDevices);                     /**> If true then calling SAFE_RELAESE with pDevices */
        pEnumerator->Release();                     /**> pEnumerator Releases */
        return;
    }
    pEnumerator->Release();                         /**> pEnumerator Releases */

    UINT count;                                     /**> Local Variable for Count */
    if (pDevices->GetCount(&count) != S_OK) {       /**> If count is under the requirements ... */
        pDevices->Release();                        /**> pDevices Releases */
        return;
    }

    IMMDevice *pEndpoint = NULL;                    /**> Pointer variable of type IMMDevice intialised with value NULL */
    for (ULONG i = 0; i < count; i++) {             /**> Loop Traverse count number of times */
        pDevices->Item(i, &pEndpoint);              /**> Access the Items in pDevices */
        devices.push_back(pEndpoint);               /**> Pushes the pEndPoint in devices */
    }

    pDevices->Release();                            /**> pDevices Releases */
} // get_devices


/**
*	populate_devicelist is a app_inject member function.
*	populate_devicelist ...
*
*	@returns void.
*/
void app_inject::populate_devicelist()
{
    this->device_names.clear();                 /**> Clearing device_names */

    devices_t devices;                          /**> Variable devices of type devices_t */
    this->get_devices(devices);                 /**> Calling get_devices paasing argument is devices*/

    IMMDevice *pEndpoint = NULL;                /**> Pointer variable pEndpoint intialised as NULL*/

    for (size_t i = 0; i < devices.size(); i++) {   /**> Traverse for all the devices */
        IPropertyStore *pProps;                 /**> Pointer variable pProps of type IPropertyStore */
        LPWSTR pwszID;                          /**> Local Variable pwszID of type LPWSTR  */
        pEndpoint = devices[i];                 /**> Variable is assigned each device every traverse to the pEndpoint   */

        pEndpoint->GetId(&pwszID);              /**> Get the endpoint ID string. */
        pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
        PROPVARIANT varName;                    /**> Local Variable varname of type PROPVARIANT */

        PropVariantInit(&varName);              /**> Initialize container for property value. */

        pProps->GetValue(PKEY_Device_FriendlyName, &varName); /**> Get the endpoint's friendly-name property. */

        this->device_names.push_back(varName.pwszVal); /**> Pushes ProVariants to Device Names */

        CoTaskMemFree(pwszID);
        PropVariantClear(&varName);
        pProps->Release();
    }

    this->clear_devices(devices);               /**> Calling clear_devices */
}

/**
*	inject is a app_inject member function.
*	inject: createprocessw lpcommandline must not be const literal.
*
*	@param process_id: It's type is DWORD and Id of the process.
*	@param x86: It's type is bool and results true if the system is of "x86" specification.
*	@param device_index: It's type is size_t() and Index of device.
*	@param flush: It's type is flush_t(An Enum) and Options: Soft, Hard and None.
*	@param duplicate: It's type is bool and Intialised with optional value of false.
*	@returns void.
*/
void app_inject::inject(DWORD process_id, bool x86, size_t device_index, flush_t flush, bool duplicate)
{
    IMMDevice *pEndpoint = NULL;                /**> Pointer Variable pEndpoint of type IMMDevice */
    LPWSTR pwszID = NULL;                       /**> Local Variable pwszID of type LPWSTR intiaslized value is NULL */
    

    global_routing_params routing_params;       /**> Declaring routing_params*/       
   
    routing_params.version = 0;
    routing_params.module_name_ptr = routing_params.next_global_ptr = NULL;
    routing_params.local.pid = process_id;

    
    if (device_index > 0) {                     /**> set routing params */
        devices_t devices;
        this->get_devices(devices);

        pEndpoint = devices[device_index - 1];  /**> Initializes interprocess arguments for routing audio to new device */
        pEndpoint->GetId(&pwszID);

        this->clear_devices(devices);           /**> Calling clear_devices */

        routing_params.local.session_guid_and_flag = get_session_guid_and_flag(duplicate);
        routing_params.local.device_id_ptr = (uint64_t)pwszID;
    }
    else {
        /**
         * Initializes interprocess arguments for routing audio to default device
         * (acts as deloading the audio routing functionality)
         */
        routing_params.local.session_guid_and_flag = 0; /**> unload dll flag */
        // MAKE_SESSION_GUID_AND_FLAG(session_guid++, 0); /**> unload dll flag*/
        routing_params.local.device_id_ptr = NULL;
    }

    /** Create file mapped object for ipc
     */
    security_attributes sec(FILE_MAP_ALL_ACCESS);
    CHandle hfile(CreateFileMapping(INVALID_HANDLE_VALUE, sec.get(), PAGE_READWRITE, 0,
            global_size(&routing_params), L"Local\\audio-router-file"));

    if (hfile == NULL || (pwszID && *pwszID == NULL)) {         /**> ... */
        CoTaskMemFree(pwszID);
        throw_errormessage(GetLastError());
    }

    unsigned char *buffer = (unsigned char *)MapViewOfFile(hfile, FILE_MAP_ALL_ACCESS, 0, 0, 0);        /**>  */

    if (buffer == NULL) {                                       /**> ... */
        CoTaskMemFree(pwszID);
        throw_errormessage(GetLastError());
    }

    serialize(&routing_params, buffer);                         /**> Calling serialize */

    UnmapViewOfFile(buffer);
    CoTaskMemFree(pwszID);

    if (pEndpoint != NULL || device_index == 0) {               /**> Check for pEndpoint Existence or if device index is zero */
        try {
            this->inject_dll(process_id, x86);                  /**> try and catch */
        }
        catch (std::wstring err) {
            throw err;
        }
        if (flush == SOFT) {                                    /**> if flush is SOFT then reset_all_devices is sent with false argument */
            reset_all_devices(false);
        }
        else if (flush == HARD && !reset_all_devices(true)) {   /**> Check the condition */
            throw std::wstring(L"Stream flush in target process failed.\n");
        }

        return;
    }

    assert(false);
} // inject

/** 
*	inject_dll is a app_inject member function.
*	inject_dll taking four argument.
*
*	@param id: It's type is DWORD and Acts as an Identification.
*	@param x86: It's type is Bool and results in true(1) if system is with specification of x86 and vice-versa.
*	@param tid: It's type is DWORD and Intiated with Optional value zero.
*	@param Flags: It's type is DWORD and Initiated with Optional value zero.
*	@returns void.
*/
void app_inject::inject_dll(DWORD pid, bool x86, DWORD tid, DWORD flags)
{
    /**
     * flag = 0: audio router dll is explicitly loaded
     * flag = 1: bootstrapper and audio router dll are implicitly loaded
     * flag = 2: bootstrapper is implicitly loaded
     * flag = 3: bootstrapper is explicitly loaded
     */
    assert(flags <= APP_INJECT_DLL_FLAG_BOOTSTRAPPER_EXPLICITLY_LOADED);
    assert((flags && flags <= APP_INJECT_DLL_FLAG_BOOTSTRAPPER_IMPLICITLY_LOADED) ? tid : true);
    assert(pid);

    /**
     * Retrieve the paths
     */
    WCHAR filepath[MAX_PATH] = {0};
    GetModuleFileName(NULL, filepath, MAX_PATH);
    CPath path(filepath);
    path.RemoveFileSpec();
    std::wstring folder = L"\"", exe = L"\"";
    folder += path;
    exe += path;
    exe += L"\\";
    exe += DO_EXE_NAME;
    exe += L"\"";
    folder += L"\"";

    // inject
    TCHAR buf[32] = {0};
    TCHAR buf2[32] = {0};
    TCHAR buf3[32] = {0};
    _itot((int)pid, buf, 10);
    _itot((int)tid, buf2, 10);
    _itot((int)flags, buf3, 10);

    std::wstring command_line = exe;
    command_line += L" ";
    command_line += buf;
    command_line += L" ";
    command_line += folder;
    command_line += L" ";
    command_line += buf2;
    command_line += L" ";
    command_line += buf3;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

/**
 * Debug
 */

#ifdef _DEBUG

# define CREATEPROCESS_FLAGS CREATE_SUSPENDED
# define RESUME_THREAD() ResumeThread(pi.hThread);
# define DO_EXE_WAIT_TIMEOUT INFINITE


#else


# define CREATEPROCESS_FLAGS 0
# define RESUME_THREAD() 0;
# define DO_EXE_WAIT_TIMEOUT 5000


#endif

    if (!CreateProcess(NULL, const_cast<LPWSTR>(command_line.c_str()),
            NULL, NULL, FALSE, CREATEPROCESS_FLAGS, NULL, NULL, &si, &pi))
    {
        throw_errormessage(GetLastError());
    }

    RESUME_THREAD()

    DWORD result = WaitForSingleObject(pi.hProcess, DO_EXE_WAIT_TIMEOUT);

    if (result == WAIT_OBJECT_0) {
        DWORD exitcode;
        GetExitCodeProcess(pi.hProcess, &exitcode);

        if (exitcode != 0) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            throw_errormessage(exitcode);
            return;
        }
    }
    else {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw std::wstring(L"Audio Router delegate did not respond in time.\n");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
} // inject_dll
/**
 * app_inject.h
 * Purpose: ________.

 * @author ...
 * @version 0.0 00/00/2000
 */

#pragma once

#include <mmdeviceapi.h>
#include <vector>
#include <string>

/** An enum type.
*	Flag Indicating the Inplicit and Explicit Audio Router Loading
*/
enum APP_INJECT_DLL_FLAG
{
    APP_INJECT_DLL_FLAG_AUDIO_ROUTER_EXPLICITLY_LOADED,                 /**> Enum value Audio Roter Explicitly Loaded */
    APP_INJECT_DLL_FLAG_BOOTSTRAPPER_AUDIO_ROUTER_IMPLICITLY_LOADED,    /**> Enum value Bootstrapper Audio Router Implicitly Loaded */
    APP_INJECT_DLL_FLAG_BOOTSTRAPPER_IMPLICITLY_LOADED,                 /**> Enum value Bootstrapper Implicitly Loaded*/
    APP_INJECT_DLL_FLAG_BOOTSTRAPPER_EXPLICITLY_LOADED,                 /**> Enum value Bootstrapper Explicity Loaded */
};
// TODO/audiorouterdev: change the order of parameters in inject dll

/**
*	Class: app_inject
*	Class Description ...
*/
class app_inject {
public:

    typedef std::vector<IMMDevice *> devices_t;     /**> Vector Devices_t and its type is IMMDevice  */
    /**
     *	An enum type
     *	flush_t indicates the assignements of Soft, Hard and None.
     */
    enum flush_t {
        SOFT = 0, HARD, NONE
    };

    /**	Static Member Function.
     *	get_devices: Function declaration.
     *	Static Member is common throughout the program as they are alloted storage once in a programm lifetime i.e static storage area.
     *
	 *	@notice Static function to get the devices.
     *	@param Type is devices_ t a call by reference.
     */
    static void get_devices(devices_t&);

    /**	Static Member Function.
     *	clear_devices: Function declaration.
	 *
     *	@notice Static function.
     *	@param Type is devices_t a call by reference.
     */
    static void clear_devices(devices_t&);

    // throws formatted last error message
    // TODO/audiorouterdev: use both as flag parameter

    /**	Static Member Function.
     *	inject_dll taking four argument.
	 *
     *	@param id: It's type is DWORD and Acts as an Identification.
     *	@param x86: It's type is Bool and results in true(1) if system is with specification of x86 and vice-versa.
     *	@param tid: It's type is DWORD and Intiated with Optional value zero.
     *	@param Flags: It's type is DWORD and Initiated with Optional value zero.
     */
    static void inject_dll(DWORD id, bool x86, DWORD tid = 0, DWORD flags = 0);

    /**	A Static Function variable.
     *	get_session_guid_and_flag.
	 *
     *	@param duplicate: It's type is Bool and results in true if it is duplicate and vice-versa.
     *	@param saved_routing: It's type is Bool and Initiated with Optional Value False.
     */
    static DWORD get_session_guid_and_flag(bool duplicate, bool saved_routing = false);

private:
    /**	A Private Static Member variable.
     *	session_guild: Type of variable is DWORD.
     */
    static DWORD session_guid;

public:

    std::vector<std::wstring> device_names; /**> A Vector and it's type is wstring*/

    /**	A constructor
     *	Only initated every once when Class Object is created.
     */
    app_inject();

    /**	Member Function
     *	populate_devicelist: List of devices which are populate.
     */
    void populate_devicelist();

    // device_index 0 is reserved for default device;
    // throws wstring;
    // duplication ignored on device_index 0

    /** Member Function
     *	inject ...
     *	@param process_id: It's type is DWORD and Id of the process.
     *	@param x86: It's type is bool and results true if the system is of "x86" specification.
     *	@param device_index: It's type is size_t() and Index of device.
     *	@param flush: It's type is flush_t(An Enum) and Options: Soft, Hard and None.
     *	@param duplicate: It's type is bool and Intialised with optional value of false.
     */
    void inject(DWORD process_id, bool x86, size_t device_index, flush_t flush, bool duplicate = false);
};
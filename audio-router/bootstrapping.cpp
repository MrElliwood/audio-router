#include "main.h"
#include <cassert>

/**
*	Definition of is_process function
*	It checkes if the given input which is a file path is a process file path
*
*	@param path: Its type is LPCWSTR which a typedef for const WCHAR *
*	@returns bool which indicates is input is a process path or not.
*/
bool is_process(LPCWSTR path)
{
    if (!path) {
        return false;
    }

    TCHAR this_path[MAX_PATH] = {0};
    GetModuleFileName(NULL, this_path, MAX_PATH);

    return (lstrcmpi(this_path, path) == 0);
}

/**
*	Definition of apply_implicit_routing
*	Checks to be done before loading audio router dll, it crawls a list
*
*	@returns bool which is true if audio dll is found , otherwise false.
*/
bool apply_implicit_routing()
{
    CHandle hfile(OpenFileMappingW(FILE_MAP_READ, FALSE, L"Local\\audio-router-file-startup"));
    if (hfile == NULL) {
        return false;
    }

    // initialize routing functionality if found on the list
    unsigned char *view_of_file = (unsigned char *)MapViewOfFile(hfile, FILE_MAP_COPY, 0, 0, 0);
    if (view_of_file == NULL) {
        return false;
    }

    bool found = false;

    for (global_routing_params *params = rebase(view_of_file);
        params != NULL;
        params = (global_routing_params *)params->next_global_ptr)
    {
        if (is_process((LPWSTR)params->module_name_ptr)) {
            params->local.pid = GetCurrentProcessId();

            // audio router dll won't be loaded if the implicit parameters are invalid
            found = apply_parameters(params->local);
        }
    }

    UnmapViewOfFile(view_of_file);

    return found;
} // apply_implicit_routing

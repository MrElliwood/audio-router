#include "patch.h"

/**
*	Definition of swap_vtable function.
*	Returns pointer to virtual function table. However updates the struct vtable.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@returns DWORD_PTR(pointer) which is typedef for ULONG_PTR.
*/
DWORD_PTR* swap_vtable(IAudioStreamVolume *this_)
{
    DWORD_PTR *old_vftptr = ((DWORD_PTR **)this_)[0];

    ((DWORD_PTR **)this_)[0] = ((DWORD_PTR ***)this_)[0][IAUDIOSTREAMVOLUME_VFTPTR_IND_OLD];
    return old_vftptr;
}

/**
*	Definition of release_patch function.
*	Releases memory.
*	This is part of memory management. When reference count is zero Release frees object's memory.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@returns HRESULT (long) which reference count number.
*/
HRESULT __stdcall release_patch(IAudioStreamVolume *this_)
{
    iaudiostreamvolume_duplicate *dup = get_duplicate(this_);
    IAudioStreamVolume *proxy = dup->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    ULONG result = this_->Release();

    if (result == 0) {
        dup->proxy = NULL;
        delete[] old_vftptr;
        delete dup;
    }
    else {
        ((DWORD_PTR **)this_)[0] = old_vftptr;
    }

    return result;
} // release_patch

/**
*	Definition of get_duplicate function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@returns iaudiostreamvolume_duplicate*.
*/
iaudiostreamvolume_duplicate* get_duplicate(IAudioStreamVolume *this_)
{
    return ((iaudiostreamvolume_duplicate ***)this_)[0][IAUDIOSTREAMVOLUME_VFTPTR_IND_DUP];
}

/**
*	Definition of getchannelcount_patch function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@param pdwCount(pointer): Its type is UINT32(unsigned int).
*	@returns HRESULT(alias for long).
*/
HRESULT __stdcall getchannelcount_patch(IAudioStreamVolume *this_, UINT32 *pdwCount)
{
    IAudioStreamVolume *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->GetChannelCount(pdwCount);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    /*for(iaudiostreamvolume_duplicate* next = get_duplicate(this_)->next;
        next != NULL; next = next->next)
       {
        UINT32 pdwcount;
        next->proxy->GetChannelCount(&pdwcount);
       }*/

    return hr;
}

/**
*	Definition of setchannelvolume_patch function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@param dwIndex: Its type is UINT32(unsigned int). This is device index.
*	@param pfLevel: Its type is const float.
*	@returns HRESULT(alias for long).
*/
HRESULT __stdcall setchannelvolume_patch(IAudioStreamVolume *this_, UINT32 dwIndex, const float fLevel)
{
    IAudioStreamVolume *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->SetChannelVolume(dwIndex, fLevel);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    for (iaudiostreamvolume_duplicate *next = get_duplicate(this_)->next;
        next != NULL; next = next->next)
    {
        next->proxy->SetChannelVolume(dwIndex, fLevel);
    }

    return hr;
}

/**
*	Definition of getchannelvolume_patch function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@param dwIndex: Its type is UINT32(unsigned int). This is device index.
*	@param pfLevel(pointers): Its type is float. 
*	@returns HRESULT(alias for long).
*/
HRESULT __stdcall getchannelvolume_patch(IAudioStreamVolume *this_, UINT32 dwIndex, float *pfLevel)
{
    IAudioStreamVolume *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->GetChannelVolume(dwIndex, pfLevel);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    /*for(iaudiostreamvolume_duplicate* next = get_duplicate(this_)->next;
        next != NULL; next = next->next)
       {
        float level;
        next->proxy->GetChannelVolume(dwIndex, &level);
       }*/

    return hr;
}

/**
*	Definition of setallvolumes_patch function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@param dwCount: Its type is UINT32(unsigned int). The number of volumes to be set.
*	@param pfVolumes(pointers): Its type is float. It is a container for array of volumes.
*	@returns HRESULT(alias for long).
*/
HRESULT __stdcall setallvolumes_patch(IAudioStreamVolume *this_, UINT32 dwCount, const float *pfVolumes)
{
    IAudioStreamVolume *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->SetAllVolumes(dwCount, pfVolumes);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    for (iaudiostreamvolume_duplicate *next = get_duplicate(this_)->next;
        next != NULL; next = next->next)
    {
        UINT32 channels = 2;
        next->proxy->GetChannelCount(&channels);

        if (channels > 0) {
            float *volumes = new float[channels];
            UINT32 i = 0;

            for (; i < dwCount; i++) {
                if (i < channels) {
                    volumes[i] = pfVolumes[i];
                }
            }

            for (; i < channels; i++) {
                volumes[i] = pfVolumes[0];
            }

            next->proxy->SetAllVolumes(channels, volumes);
            delete[] volumes;
        }
    }

    return hr;
} // setallvolumes_patch

/**
*	Definition of getallvolumes_patch function.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@param dwCount: Its type is UINT32(unsigned int). The number of volumes to be fetched.
*	@param pfVolumes(pointers): Its type is float. Sets to array of volumes.
*	@returns HRESULT(alias for long).
*/
HRESULT __stdcall getallvolumes_patch(IAudioStreamVolume *this_, UINT32 dwCount, float *pfVolumes)
{
    IAudioStreamVolume *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->GetAllVolumes(dwCount, pfVolumes);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    return hr;
}

/**
*	Definition of patch_iaudiostreamvolume function.
*	Creates new virtual table and populate it with functions.
*
*	@param this_(pointer): Its type is IAudioStreamVolume struct.
*	@returns void.
*/
void patch_iaudiostreamvolume(IAudioStreamVolume *this_)
{
    // create new virtual table and save old and populate new with default
    DWORD_PTR *old_vftptr = ((DWORD_PTR **)this_)[0]; // save old virtual table

    // create new virtual table (slot for old table ptr and for duplicate)
    ((DWORD_PTR **)this_)[0] = new DWORD_PTR[IAUDIOSTREAMVOLUME_VFTPTR_COUNT];
    memcpy(((DWORD_PTR **)this_)[0], old_vftptr, 8 * sizeof(DWORD_PTR));

    // create duplicate object
    iaudiostreamvolume_duplicate *dup = new iaudiostreamvolume_duplicate(this_);

    // patch routines
    DWORD_PTR *vftptr = ((DWORD_PTR **)this_)[0];
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_RELEASE]            = (DWORD_PTR)release_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_GET_CHANNEL_COUNT]  = (DWORD_PTR)getchannelcount_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_SET_CHANNEL_VOLUME] = (DWORD_PTR)setchannelvolume_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_GET_CHANNEL_VOLUME] = (DWORD_PTR)getchannelvolume_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_SET_ALL_VOLUMES]    = (DWORD_PTR)setallvolumes_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_GET_ALL_VOLUMES]    = (DWORD_PTR)getallvolumes_patch;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_OLD]                = (DWORD_PTR)old_vftptr;
    vftptr[IAUDIOSTREAMVOLUME_VFTPTR_IND_DUP]                = (DWORD_PTR)dup;
} // patch_iaudiostreamvolume

#include "patch.h"
#include <stdint.h>
#include <cassert>

/**
*	Definition of swap_vtable function.
*	Returns pointer to virtual function table. However updates the struct vtable.
*
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@returns DWORD_PTR(pointer) which is typedef for ULONG_PTR.
*/
DWORD_PTR* swap_vtable(IAudioRenderClient *this_)
{
    DWORD_PTR *old_vftptr = ((DWORD_PTR **)this_)[0];

    ((DWORD_PTR **)this_)[0] = ((DWORD_PTR ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_OLD];
    return old_vftptr;
}

/**
*	Definition of release_patch function.
*	Releases memory.
*	This is part of memory management. When reference count is zero Release frees object's memory.
*
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@returns HRESULT (long) which reference count number.
*/
HRESULT __stdcall release_patch(IAudioRenderClient *this_)
{
    iaudiorenderclient_duplicate *dup = get_duplicate(this_);
    IAudioRenderClient *proxy = dup->proxy;
    UINT32 *arg = ((UINT32 ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_NUM_FRAMES_REQUESTED];
    WORD *arg2 = ((WORD ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_FRAME_SIZE];
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    ULONG result = this_->Release();

    if (result == 0) {
        dup->proxy = NULL;
        delete[] old_vftptr;
        delete dup;
        delete arg;
        delete arg2;
    }
    else {
        ((DWORD_PTR **)this_)[0] = old_vftptr;
    }

    return result;
} // release_patch

/**
*	Definition of get_duplicate function.
*
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@returns iaudiorenderclient_duplicate*.
*/
iaudiorenderclient_duplicate* get_duplicate(IAudioRenderClient *this_)
{
    return ((iaudiorenderclient_duplicate ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_DUP];
}

/**
*	Definition of getbuffer_patch function.
*
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@param NumFramesRequested: Its type is UINT32 which is typedef for unsigned int.
*	@param ppData: Its type is BYTE which typedef for unsigned char.
*	@returns HRESULT which is typedef for long.
*/
HRESULT __stdcall getbuffer_patch(IAudioRenderClient *this_, UINT32 NumFramesRequested, BYTE **ppData)
{
    IAudioRenderClient *proxy = get_duplicate(this_)->proxy;
    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->GetBuffer(NumFramesRequested, ppData);

    ((DWORD_PTR **)this_)[0] = old_vftptr;

    if (ppData != NULL && hr == S_OK && NumFramesRequested > 0) {
        ((BYTE ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_BUFFER_PTR] = *ppData;
        *((UINT32 ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_NUM_FRAMES_REQUESTED] = NumFramesRequested;
    }
    else {
        ((BYTE ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_BUFFER_PTR] = NULL;
    }

    return hr;
} // getbuffer_patch

/**
*	Definition of copy_mem function.
*	This is a template with T type generic. T type is used for buffer type.
*
*	@param buffer2_(pointer): Its type is BYTE which is typedef for unsigned char.
*	@param buffer_(pointer): Its type is BYTE which is typedef for unsigned char.
*	@param frames_written: Its type is UINT32 which is typedef for unsigned int.
*	@returns void.
*/
template <typename T> void copy_mem(BYTE *buffer2_, const BYTE *buffer_, UINT32 frames_written)
{
    const T *buffer = (const T *)buffer_;
    T *buffer2 = (T *)buffer2_;

    for (UINT32 i = 0; i < frames_written; i++) {
        buffer2[i] = buffer[i];
    }
}

/**
*	Definition of releasebuffer_patch function.
*
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@param NumFramesWritten: Its type is UINT32 which is typedef for unsigned int.
*	@param dwFlags: Its type is DWORD which typedef for unsigned long.
*	@returns HRESULT which is typedef for long.
*/
HRESULT __stdcall releasebuffer_patch(IAudioRenderClient *this_, UINT32 NumFramesWritten, DWORD dwFlags)
{
    IAudioRenderClient *proxy = get_duplicate(this_)->proxy;
    const BYTE *buffer = ((BYTE ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_BUFFER_PTR];
    const UINT32 frames_req = *((UINT32 ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_NUM_FRAMES_REQUESTED];
    const WORD framesize = *((WORD ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_FRAME_SIZE];

    if (buffer != NULL) {
        for (iaudiorenderclient_duplicate *next = get_duplicate(this_)->next;
            next != NULL; next = next->next)
        {
            if (next->proxy) {
                BYTE *buffer2;
                HRESULT hr2 = next->proxy->GetBuffer(NumFramesWritten, &buffer2);

                if (hr2 == S_OK) {
                    DWORD flags = dwFlags;

                    switch (framesize) {
                    case 1:
                        copy_mem<uint8_t>(buffer2, buffer, NumFramesWritten);
                        break;
                    case 2:
                        copy_mem<uint16_t>(buffer2, buffer, NumFramesWritten);
                        break;
                    case 4:
                        copy_mem<uint32_t>(buffer2, buffer, NumFramesWritten);
                        break;
                    case 8:
                        copy_mem<uint64_t>(buffer2, buffer, NumFramesWritten);
                        break;
                    default:
                        flags = AUDCLNT_BUFFERFLAGS_SILENT;
                        break;
                    } // switch
                    next->proxy->ReleaseBuffer(NumFramesWritten, flags);
                }
                else if (hr2 == AUDCLNT_E_OUT_OF_ORDER) {
                    next->proxy->ReleaseBuffer(0, AUDCLNT_BUFFERFLAGS_SILENT);
                }
            }
        }
    }

    ((BYTE ***)this_)[0][IAUDIORENDERCLIENT_VFTPTR_IND_BUFFER_PTR] = NULL;

    DWORD_PTR *old_vftptr = swap_vtable(this_);
    HRESULT hr = proxy->ReleaseBuffer(NumFramesWritten, dwFlags);
    ((DWORD_PTR **)this_)[0] = old_vftptr;

    return hr;
} // releasebuffer_patch

/**
*	Definition of patch_iaudiorenderclient function.
*	Creates new virtual table and populate it with functions.
*	
*	@param this_(pointer): Its type is IAudioRenderClient struct.
*	@param block_align: Its type is WORD which is typedef for unsigned short.
*	@returns void.
*/
void patch_iaudiorenderclient(IAudioRenderClient *this_, WORD block_align)
{
    // create new virtual table and save old and populate new with default
    DWORD_PTR *old_vftptr = ((DWORD_PTR **)this_)[0]; // save old virtual table

    // create new virtual table (slot 5 for old table ptr and 6 for duplicate)
    ((DWORD_PTR **)this_)[0] = new DWORD_PTR[IAUDIORENDERCLIENT_VFTPTR_COUNT];
    memcpy(((DWORD_PTR **)this_)[0], old_vftptr, 5 * sizeof(DWORD_PTR));

    // created duplicate object
    iaudiorenderclient_duplicate *dup = new iaudiorenderclient_duplicate(this_);

    // patch routines
    DWORD_PTR *vftptr = ((DWORD_PTR **)this_)[0];
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_RELEASE]               = (DWORD_PTR)release_patch;
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_GET_BUFFER]            = (DWORD_PTR)getbuffer_patch;
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_RELEASE_BUFFER]        = (DWORD_PTR)releasebuffer_patch;
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_OLD]                   = (DWORD_PTR)old_vftptr;
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_DUP]                   = (DWORD_PTR)dup;
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_BUFFER_PTR]            = NULL; // buffer pointer
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_NUM_FRAMES_REQUESTED]  = (DWORD_PTR) new UINT32; // NumFramesRequested
    vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_FRAME_SIZE]            = (DWORD_PTR) new WORD; // size of audio frame
    *(WORD *)(vftptr[IAUDIORENDERCLIENT_VFTPTR_IND_FRAME_SIZE]) = block_align;
} // patch_iaudiorenderclient

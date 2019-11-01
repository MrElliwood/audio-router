#pragma once

#include <Windows.h>
#include <type_traits>
#include <stdint.h>

// TODO/audiorouterdev: atomicity mus be implemented with short jmp to code path in 2gb address space
// that will jump to the patched function in the 64 space

/**
*	Definition of patcher class
*	This is a template with generic type T.	
*/
template <typename T> class patcher {
public:

    typedef T func_t;
    typedef size_t address_t;
#pragma pack(push, 1)
    struct jmp_to
    {
        typedef typename std::conditional<
            std::is_same<address_t, uint32_t>::value,
            std::integral_constant<unsigned char, 0xb8>,
            std::integral_constant<WORD, 0xb848> >::type mov_ax_t;
        typename mov_ax_t::value_type mov_ax = mov_ax_t::value;
        address_t addr;
        WORD jmp_ax = 0xe0ff;
    };

#pragma pack(pop)

private:

    const func_t patched_func;
    void *original_func;
    jmp_to old_bytes;
    DWORD old_protect;
    CRITICAL_SECTION critical_section;

public:

	/**
	*	Constructor for patcher class
	*
	*	@param: patched_func: Its type is func_t
	*/
    patcher(func_t patched_func) : patched_func(patched_func), original_func(NULL)
    {
        InitializeCriticalSectionAndSpinCount(&this->critical_section, 0x00000400);
    }

	/**
	*	Destructor for patcher class.
	*	The heavy lifting is done by destroy method.
	*/
    ~patcher()
    {
        this->destroy();
    }

	/**
	*	destroy is a patcher member function.
	*	Revert changes and delete lock.
	*	
	*	@returns void.
	*/
    void destroy()
    {
        this->revert();
        DeleteCriticalSection(&this->critical_section);
    }

	/**
	*	is_patched is a patcher member function.
	*	If error occurs while reading, it returns 2, otherwise returns memcmp matched against 0 and cast to int.
	*
	*	@returns int.
	*/
    int is_patched() const
    {
        if (IsBadReadPtr(this->original_func, sizeof(jmp_to))) {
            return 2;
        }

        return (int)(memcmp(this->original_func, &this->old_bytes, sizeof(jmp_to)) != 0);
    }

	/**
	*	get_function is a patcher member function
	*
	*	@returns const void*.
	*/
    const void* get_function() const
    {
        return this->original_func;
    }

	/**
	*	patch is a patcher member function.
	*
	*	@param func_address(pointer): Its type is void.
	*	@returns int: 1 when param value is NULL, 2 when there is no Virtual protection, otherwise 0.
	*/
    int patch(void *func_address)
    {
        if (!func_address) {
            return 1;
        }

        // // patchable function must be 16 byte aligned to ensure atomic patching
        // if((address_t)func_address & 0xf)
        //    return 3;

// #ifdef _WIN64
//        const size_t size = 16;
// #else
//        const size_t size = 8;
// #endif
//        assert(size >= sizeof(jmp_to));
//
        if (!VirtualProtect(func_address, sizeof(jmp_to), PAGE_EXECUTE_READWRITE,
                &this->old_protect)) {
            return 2;
        }

        this->original_func = func_address;
        memcpy(&this->old_bytes, this->original_func, sizeof(jmp_to));
        this->apply();

        return 0;
    } // patch

	/**
	*	lock is patcher member function.
	*	This create something in the nature of Mutex lock when called.
	*	
	*	@returns void.
	*/
    void lock()
    {
        EnterCriticalSection(&this->critical_section);
    }

	/**
	*	unlock is patcher member function.
	*	Removes lock.
	*
	*	@returns void.
	*/
    void unlock()
    {
        LeaveCriticalSection(&this->critical_section);
    }

	/**
	*	revert is patcher member function.
	*	Check that original function is in the right position. Copy old_bytes value to orignal function.
	*	This means rolling back changes.
	*
	*	@returns void.
	*/
    void revert()
    {
        if (IsBadWritePtr(this->original_func, sizeof(jmp_to))) {
            return;
        }

        // if(this->patched)
        {
            // bad write ptr might happen if the dll that is patched
            // is unloaded before this dll is unloaded

            /*if(IsBadWritePtr(this->original_func, sizeof(jmp_to)))
                return;*/
            memcpy(this->original_func, &this->old_bytes, sizeof(jmp_to));

            // this->patched = false;
        }
    }

	/**
	*	apply is a patcher member function.
	*	Set the original functo to patched func address.
	*
	*	@returns void
	*/
    void apply()
    {
        if (IsBadWritePtr(this->original_func, sizeof(jmp_to))) {
            return;
        }

        // if(!this->patched)
        {
            jmp_to patch;
            patch.addr = (address_t) this->patched_func;
            memcpy(this->original_func, &patch, sizeof(jmp_to));

            // this->patched = true;
        }
    }
};

#pragma once

#include <Windows.h>
#ifdef _DEBUG
# include <cassert>
#else
# define assert(a) (0)
#endif

class CHandle {
public:

    CHandle() throw();
    CHandle(_Inout_ CHandle& h) throw();
    explicit CHandle(_In_ HANDLE h) throw();
    ~CHandle() throw();

    CHandle& operator=(_Inout_ CHandle& h) throw();

    operator HANDLE() const throw();

    // Attach to an existing handle (takes ownership).
    void Attach(_In_ HANDLE h) throw();

    // Detach the handle from the object (releases ownership).
    HANDLE Detach() throw();

    // Close the handle.
    void Close() throw();

public:

    HANDLE m_h;
};

/** Constructor Definition of CHandle with empty parameter.
*
*	Initialize CHandle to NULL.
*	@throw empty exception.
*
*/
inline CHandle::CHandle() throw() : m_h(NULL)
{}

/** Constructor Definition of CHandle with Chandle as parameter.
*
*	Move operation.
*	@param h: Its type is CHandle reference with _Inout_ prefix.
*	@throw empty exception.
*
*/
inline CHandle::CHandle(_Inout_ CHandle& h) throw() : m_h(NULL)
{
    Attach(h.Detach());
}

/** Constructor Definition of CHandle with Chandle as parameter.
*
*	Copy operation.
*	@param h: Its type is CHandle with _In_ prefix.
*	@throw empty exception.
*
*/
inline CHandle::CHandle(_In_ HANDLE h) throw() : m_h(h)
{}

/** De-constructor Definition of CHandle with empty parameter.
*
*	Close CHandle.
*	@throw empty exception.
*
*/
inline CHandle::~CHandle() throw()
{
    if (m_h != NULL) {
        Close();
    }
}

/** Move Assignment Constructor Definition for CHandle.
*
*	Move assignment.
*	@param h: Its type is CHandle with _Inout_ prefix.
*	@throw empty exception.
*
*/
inline CHandle& CHandle::operator=(_Inout_ CHandle& h) throw()
{
    if (this != &h) {
        if (m_h != NULL) {
            Close();
        }

        Attach(h.Detach());
    }

    return (*this);
}

/** Definition of CHandle operator HANDLE.
*
*	Gets CHandle.
*	@throw empty exception.
*
*/
inline CHandle::operator HANDLE() const throw()
{
    return (m_h);
}

/** Definition of member method, Attach.
*
*	Assignment if Chandle is NULL
*	@param h: Its type is CHandle with _In_ prefix.
*	@throw empty exception.
*
*/
inline void CHandle::Attach(_In_ HANDLE h) throw()
{
    assert(m_h == NULL);
    m_h = h;      // Take ownership
}

/** Definition of member method, Detach.
*
*	Move operation on CHandle.
*	@throw empty exception.
*
*/
inline HANDLE CHandle::Detach() throw()
{
    HANDLE h;

    h = m_h;      // Release ownership
    m_h = NULL;

    return (h);
}

/** Definition of member method, Close.
*
*	Release ownership and close handle.
*	@throw empty exception.
*
*/
inline void CHandle::Close() throw()
{
    if (m_h != NULL) {
        ::CloseHandle(m_h);
        m_h = NULL;
    }
}

#include "SysTray.h"
#include "window.h"

// TODO/wolfreak99: Create a common.h for stuff such as this
#include <assert.h>

/**
*	Constructor for SysTray.
*
*/
SysTray::SysTray()
{
    bInTray = false;
}

/**
*	Destructor for SysTray. This is empty function.
*
*/
SysTray::~SysTray()
{}

/**
*	Create is a SysTray member function.
*	Initialize NOTIFYICONDATA struct
*
*	@param parent: This is a window reference.
*	@param uid: Its type is UINT which is a typedef for unsigned int.
*	@returns void
*/
void SysTray::Create(window& parent, UINT uid)
{
    m_NotifyIconData.cbSize = NOTIFYICONDATAA_V1_SIZE;
    m_NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_NotifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;

    m_NotifyIconData.hWnd = parent.m_hWnd;
    m_NotifyIconData.uID = uid;
    m_NotifyIconData.hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
}

/**
*	Destroy is a SysTray member function. This is empty function.
*
*/
void SysTray::Destroy()
{}

/**
*	SetIcon is a SysTray member function. 
*	Update NOTIFYICONDATA struct  pointer to Icon handle.
*
*	@param hNewIcon: Its type is HICON.
*	@returns bool.
*
*/
BOOL SysTray::SetIcon(HICON hNewIcon)
{
    try {
        m_NotifyIconData.hIcon = hNewIcon;

        // Update the icon if it is visible.
        if (bInTray) {
            BOOL iRetVal;
            iRetVal = Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);

            if (iRetVal) {
                bInTray = true;
            }

            return iRetVal;
        }

        return 1;
    }
    catch (std::wstring err) {
        throw err;
        assert(false);
        return 0;
    }
} // SetIcon

/**
*	GetIcon is a SysTray member function.
*	Gets pointer  to Icon handle from NOTIFYICONDATA struct.
*	
*	@returns HICON which is point to Icon handle.
*
*/
HICON SysTray::GetIcon()
{
    return m_NotifyIconData.hIcon;
}

/**
*	SetTipText is a SysTray member function.
*	Update NOTIFYICONDATA struct  with Tiptext.
*
*	@param newTipTex: Its type is ATL::CString
*	@returns BOOL which is typedef of int. Its value tells if update works.
*
*/
BOOL SysTray::SetTipText(ATL::CString newTipText)
{
    try {
        _tcscpy_s(m_NotifyIconData.szTip, newTipText);

        // Update the icon if it is visible.
        if (bInTray) {
            BOOL iRetVal;
            iRetVal = Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);

            if (iRetVal) {
                bInTray = true;
            }

            return iRetVal;
        }

        return 1;
    }
    catch (std::wstring err) {
        throw err;
        assert(false);
        return 0;
    }
} // SetTipText

/**
*	GetTipText is a SysTray member function.
*
*	@returns char * however this is a contant value 'test'.
*
*/
char * SysTray::GetTipText()
{
    // TODO/wolfreak99: Find out how to make this text show up, and then try to get commented code to show.
    return "test"; // NotifyIconData.szTip;
}

/**
*	AddIcon is a SysTray member function.
*	Add Icon if it is not visible.
*
*	@returns BOOL which is typedef of int.
*
*/
BOOL SysTray::AddIcon()
{
    assert(m_NotifyIconData.cbSize);
    assert(!bInTray);

    if (!bInTray) {
        BOOL iRetVal = Shell_NotifyIcon(NIM_ADD, &m_NotifyIconData);
        assert(iRetVal);

        if (iRetVal) {
            bInTray = true;
        }

        return iRetVal;
    }

    return 0;
} // AddIcon

/**
*	RemoveIcon is a SysTray member function.
*	Remove Icon if it is not visible.
*	
*	@returns BOOL which is typedef of int.
*
*/
BOOL SysTray::RemoveIcon()
{
    assert(m_NotifyIconData.cbSize);
    assert(bInTray);

    if (bInTray) {
        BOOL iRetVal = Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
        assert(iRetVal);

        if (iRetVal) {
            bInTray = false;
        }

        return iRetVal;
    }

    return 0;
} // RemoveIcon

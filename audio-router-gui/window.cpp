#include "window.h"

// TODO/wolfreak99: Check audiorouterdevs "remove saved routing functionality", To determine if bootstrap related to feature.
#ifdef ENABLE_BOOTSTRAP
window::window(bootstrapper *bootstrap) : dlg_main_b(true), bootstrap(bootstrap)
#else
/**
*	Constructor creates a window object.
* 
*/
window::window() : dlg_main_b(true)
#endif
{
    this->dlg_main = new dialog_main(*this);
    this->form_view = new formview(*this);

    // our windows hWnd is still null, so we can only initialize sysTray partially.
    // the rest is handled in OnCreate. (what a bunch of shit)
    this->m_SysTray = new SysTray();
}

/**
*	Destructor for window of object.
*
*/
window::~window()
{
    if (this->dlg_main_b) {
        delete this->dlg_main;
    }

    delete this->form_view;
    delete this->m_SysTray;
}

/**
*	Oncreate is a window member function. Creates dialog and its visible, and set up system tray, window text and its icon.
*	
*	@param lpcs: Its type is LPCREATESTRUCT however this is not used.
*	@return int:  zero value on completion.
*
*/
int window::OnCreate(LPCREATESTRUCT lpcs)
{
    this->m_hWndClient = this->dlg_main->Create(this->m_hWnd);
    this->dlg_main->ShowWindow(SW_SHOW);

    // Set up handles for the system tray.
    m_SysTray->Create(*this, 1);

    ATL::CString sWindowText;
    GetWindowText(sWindowText);
    m_SysTray->SetTipText(sWindowText);
    m_SysTray->AddIcon();

    return 0;
} // OnCreate

/**
*	OnDestroy is a window member function, it removes icon from system tray.
*
*	@param uMsg: Its type is UINT which is a typedef for unsigned int. Not used.
*	@param wParam: Its type is WPARAM which is a typedef for UINT_PTR. Not used.
*	@param lParam: Its type is LPARAM which is a typedef for LONG_PTR. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns null pointer: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_SysTray->RemoveIcon();
    return 0;
}

/**
*	OnQuit is a window member function which destroys the window and post quit message
*
*	@param uMsg: Its type is UINT which is a typedef for unsigned int. Not used.
*	@param wParam: Its type is WPARAM which is a typedef for UINT_PTR. Not used.
*	@param lParam: Its type is LPARAM which is a typedef for LONG_PTR. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnQuit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    this->DestroyWindow();

    // TODO/wolfreak99: This may not even be needed.
    PostQuitMessage(0);
    return 0;
}

/**
*	OnNcHitTest is a window member function.
*
*	@param uMsg: Its type is UINT which is a typedef for unsigned int. Not used.
*	@param wParam: Its type is WPARAM which is a typedef for UINT_PTR. Not used.
*	@param lParam: Its type is LPARAM which is a typedef for LONG_PTR. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns HTCLOSE: It is a define, and alias for 20.
*
*/
LRESULT window::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return HTCLOSE;
}

/**
*	OnSysCommand is a window member function. Run close and restore commands.
*
*	@param uMsg: Its type is UINT which is a typedef for unsigned int. Not used.
*	@param wParam: Its type is WPARAM which is a typedef for UINT_PTR. Decides which command 0xF060 for close, and 0xF120 for restore.
*	@param lParam: Its type is LPARAM which is a typedef for LONG_PTR. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (wParam) {
    case SC_CLOSE:

        // Closing the window shows the icon while hiding it from the minimized windows bar.
        this->ShowWindow(SW_HIDE);

        // We do not need any further handling done, so set bHandled to TRUE
        bHandled = TRUE;
        break;
    case SC_RESTORE:

        // TODO/wolfreak99: This may not be needed, it's commented out eventually, but I'm keeping it here just incase
        for (dialog_main::dialog_arrays_t::iterator it = this->dlg_main->dialog_arrays.begin();
            it != this->dlg_main->dialog_arrays.end();
            it++)
        {
            for (dialog_array::dialog_controls_t::iterator jt = (*it)->dialog_controls.begin();
                jt != (*it)->dialog_controls.end();
                jt++)
            {
                (*jt)->set_display_name(false, false);
            }
        }

        ShowWindow(SW_SHOW);
        BringWindowToTop();

        // Have windows still handle the rest
        bHandled = FALSE;
        break;
    default:
        bHandled = false;
        break;
    } // switch

    return 0;
} // OnSysCommand

/**
*	OnSystemTrayIcon is a window member function.
*
*	@param uMsg: Its type is UINT which is a typedef for unsigned int. Not used.
*	@param wParam: Its type is WPARAM which is a typedef for UINT_PTR. Not used.
*	@param lParam: Its type is LPARAM which is a typedef for LONG_PTR. It determines if button is right or left. Rigth is 0x0202, Left is 0x0205.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ATLASSERT(wParam == 1);

    switch (lParam) {
    case WM_LBUTTONUP:
        ShowOrHideWindow();
        break;
    case WM_RBUTTONUP:

        // SetForegroundWindow(m_hWnd);

        CPoint pos;
        ATLVERIFY(GetCursorPos(&pos));

        CMenu menu;
        menu.LoadMenuW(IDR_TRAYMENU);
        CMenuHandle popupMenu = menu.GetSubMenu(0);

        popupMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
            pos.x,
            pos.y,
            this->m_hWnd);
        break;
    } // switch
    return 0;
} // OnSystemTrayIcon

/**
*	OnFileRefreshlist is a window member function. Checks that it is not main dialog, and refesh list.
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnFileRefreshlist(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!this->dlg_main_b) {
        this->form_view->refresh_list();
    }

    return 0;
}

/**
*	OnAbout is a window member function.
*	Show message about Audio Router.
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // TODO/wolfreak99: Update this about to be more accurate, CLAIM OUR STOLEN FUCKING TERRITORY, FEARLESS OF LAWSUITS.
    this->MessageBoxW(L"Audio Router, the 'Revived from the grave' version.\n" \
        L"I have taken over this project, due to inactivity by the original developer." \
        L"If you come across any bugs (especially related to, well, routing audio), " \
        L"Have a feature suggestion, or you are the original dev and wish to file a lawsuit, "\
        L"Feel free to post an issue on the github page and I will get back to you when I can!\n\n"\
        L"https://github.com/wolfreak99/audio-router", L"About", MB_ICONINFORMATION);
    return 0;
}

/**
*	OnFileSwitchview is a window member function.
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnFileSwitchview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    RECT rc;

    this->GetClientRect(&rc);

    if (this->dlg_main_b) {
        this->dlg_main->DestroyWindow();
        delete this->dlg_main;

        this->m_hWndClient = this->form_view->Create(*this);

        // this->form_view->ShowWindow(SW_SHOW);
        this->form_view->SetWindowPos(NULL, &rc, SWP_NOZORDER | SWP_SHOWWINDOW);
    }
    else {
        // TODO/wolfreak99: Should form_view be deleted as well?
        this->form_view->DestroyWindow();

        this->dlg_main = new dialog_main(*this);
        this->m_hWndClient = this->dlg_main->Create(*this);

        // this->dlg_main->ShowWindow(SW_SHOW);
        this->dlg_main->SetWindowPos(NULL, &rc, SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    this->dlg_main_b = !this->dlg_main_b;

    return 0;
} // OnFileSwitchview

/**
*	OnFileExit is a window member function. 
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns LRESULT: This indicates if file exits or not.
*
*/
LRESULT window::OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return Quit();
}

/**
*	OnTrayMenuExit is a window member function.
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns LRESULT: This indicates if menu exits or not.
*
*/
LRESULT window::OnTrayMenuExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return Quit();
}

/**
*	OnTrayMenuShowHide is a window member function. This is wrapper over ShowOrHideWindow() function.
*
*	@param wNotifyCode: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param wID: Its type is WORD which is a typedef for unsigned short. Not used.
*	@param hWndCtl: Its type is HWND which is a typede for pointer HWND__. Not used.
*	@param bHandled: Its type is BOOL which is a a typedef for int. Not used.
*	@returns 0: Its type is LRESULT which is LONG_PTR.
*
*/
LRESULT window::OnTrayMenuShowHide(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ShowOrHideWindow();
    return 0;
}

/**
*	ShowOrHideWindow is a window member function.
*	If window is visible hide it, and vice versa.
*
*	@return void.
*/
void window::ShowOrHideWindow()
{
    if (IsWindowOpen()) {
        SendMessage(WM_SYSCOMMAND, SC_CLOSE);
    }
    else {
        SendMessage(WM_SYSCOMMAND, SC_RESTORE);
    }
}

/**
*	IsWindowOpen is a window member function.
*	Checks if window is visible.
*
*	@return bool: This indicates if it is visible or not.
*/
bool window::IsWindowOpen()
{
    return this->IsWindowVisible() && !this->IsIconic();
}

/**
*	Quit is a window member function.
*	Quits the program. Use this instead of calling SendMessage(WM_QUIT) everywhere.
*
*/
LRESULT window::Quit()
{
    return SendMessage(WM_QUIT);
}

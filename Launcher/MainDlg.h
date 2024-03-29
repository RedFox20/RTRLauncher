// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <RTRCore/LauncherCore.h>
#include <atlmisc.h>
#include "DialogRegion.h"
#include "SetDpi.h"

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>, 
        public CMessageFilter, public CIdleHandler,
        public CDialogRegionT<CMainDlg>
{
public:
    CSetDPI DPI;
    core::LauncherCore Launcher;
    CButton StartBtn;
    CButton ChkShowErr;
    CButton ChkWindowed;
    CButton ChkLaunchStrat;
    CButton ChkDebugAttach;
    CButton ChkNoInject;
    CButton ChkCaLogs;
    CButton ChkBuildings;
    CButton ChkImages;
    CButton ChkValidateModels;
    CStatic TitleText;
    CComboBoxEx BoxTurnsPerYear;

    enum { IDD = IDD_MAINDLG };

    BOOL PreTranslateMessage(MSG* pMsg) override;
    BOOL OnIdle() override;

    BEGIN_UPDATE_UI_MAP(CMainDlg)
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(CMainDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        CHAIN_MSG_MAP(CDialogRegionT<CMainDlg>)
    END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    void StartWorker();

    /**
     * Loads dialog settings from loaded CoreSettings
     */
    void LoadDialogSettings();

    /**
     * Saves dialog settings into Launcher CoreSettings
     */
    void SaveDialogSettings();

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void CloseDialog(int nVal);
};

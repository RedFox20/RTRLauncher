// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

void CMainDlg::StartWorker()
{
	StartBtn.SetWindowTextA("Initializing");
	StartBtn.EnableWindow(FALSE);

	// Start RTR core asynchronously
	// on completion, enable the Start button
	Launcher.StartAsync().then([this]()
	{
		StartBtn.SetWindowTextA("Start RTR");
		StartBtn.EnableWindow(TRUE);
	});

}


LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DPI.Attach(_AtlBaseModule.GetResourceInstance(), m_hWnd, IDD, 96.0);
	SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)), TRUE);
	SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)), FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);
	UIAddChildWindowContainer(m_hWnd);

	// get control classes
	StartBtn	   = GetDlgItem(IDOK);
	ChkShowErr	   = GetDlgItem(IDC_FLAG_SHOWERR);
	ChkWindowed	   = GetDlgItem(IDC_FLAG_WINDOWED);
	ChkLaunchStrat = GetDlgItem(IDC_FLAG_LAUNCHSTRAT);
	ChkDebugAttach = GetDlgItem(IDC_FLAG_DEBUGATTACH);
	ChkNoInject	   = GetDlgItem(IDC_FLAG_NOINJECT);
	ChkCaLogs	   = GetDlgItem(IDC_FLAG_CALOG);
	ChkBuildings   = GetDlgItem(IDC_FLAG_CHECKBUILDINGS);
	ChkImages	   = GetDlgItem(IDC_FLAG_CHECKIMAGES);
	ChkValidateModels = GetDlgItem(IDC_FLAG_VALIDATEMODELS);
	BoxTurnsPerYear   = GetDlgItem(IDC_TPYBOX);
	TitleText         = GetDlgItem(IDC_TITLETEXT);

	// initialize controls data
	LoadDialogSettings();

	StartWorker();
	EnableWindow(TRUE);
	return TRUE;
}

static int ValidTPYValues[] = { 1, 2, 3, 4, 6, 8, 12 };

void CMainDlg::LoadDialogSettings()
{
	CoreSettings& settings = Launcher.Settings;
	ChkWindowed   .SetCheck(settings.Windowed);
	ChkShowErr    .SetCheck(settings.ShowErr);
	ChkLaunchStrat.SetCheck(settings.LaunchStrat);
	ChkDebugAttach.SetCheck(settings.DebugAttach);
	ChkNoInject      .SetCheck(!settings.Inject);
	ChkCaLogs        .SetCheck(settings.UseCaLog);
	ChkBuildings     .SetCheck(settings.CheckBuildings);
	ChkImages        .SetCheck(settings.CheckImages);
	ChkValidateModels.SetCheck(settings.ValidateModels);
	TitleText.SetWindowTextA(settings.Title.c_str());

	char buffer[20];
	BoxTurnsPerYear.ResetContent();
	for (int i = 0; i < sizeof(ValidTPYValues)/sizeof(int); ++i)
	{
		int value = ValidTPYValues[i];
		BoxTurnsPerYear.InsertItem(i, itoa(value, buffer, 10), 0, 0, 0);
		if (value == settings.TurnsPerYear)
			BoxTurnsPerYear.SetCurSel(i);
	}

	const char* background = settings.BkBitmap.c_str();
	if (SetBKBitmap(background)) 
	{
		SetDialogRegion();
	}
	else
	{
		concurrency::create_task([=]() {
			char msg[512];
			snprintf(msg, sizeof(msg), "Failed to find launcher background image '%s'", background);
			MessageBoxA(msg, "Error loading resource");
		});
	}
	CenterWindow();
}


void CMainDlg::SaveDialogSettings()
{
	CoreSettings& settings = Launcher.Settings;
	settings.TurnsPerYear   = ValidTPYValues[BoxTurnsPerYear.GetCurSel()];
	settings.Windowed	    = ChkWindowed.GetCheck()    == BST_CHECKED;
	settings.ShowErr	    = ChkShowErr.GetCheck()     == BST_CHECKED;
	settings.LaunchStrat    = ChkLaunchStrat.GetCheck() == BST_CHECKED;
	settings.DebugAttach    = ChkDebugAttach.GetCheck() == BST_CHECKED;
	settings.Inject		    = ChkNoInject.GetCheck()    != BST_CHECKED;
	settings.UseCaLog	    = ChkCaLogs.GetCheck()      == BST_CHECKED;
	settings.CheckBuildings = ChkBuildings.GetCheck()   == BST_CHECKED;
	settings.CheckImages    = ChkImages.GetCheck()      == BST_CHECKED;
	settings.ValidateModels = ChkValidateModels.GetCheck() == BST_CHECKED;

	Launcher.SaveSettings();
}


LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SaveDialogSettings();

	// start final foreground task... (
	Launcher.Start();
	if (Launcher.LaunchGame())
		CloseDialog(wID); // close the launcher on success

	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

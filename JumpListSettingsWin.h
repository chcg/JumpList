#pragma once

#ifdef _WIN32_WCE
#error "CDHtmlDialog is not supported for Windows CE."
#endif 

// JumpListSettingsWin dialog

class JumpListSettingsWin : public CDHtmlDialog
{
	DECLARE_DYNCREATE(JumpListSettingsWin)

public:
	JumpListSettingsWin(CWnd* pParent = NULL);   // standard constructor
	virtual ~JumpListSettingsWin();
// Overrides
	HRESULT OnButtonOK(IHTMLElement *pElement);
	HRESULT OnButtonCancel(IHTMLElement *pElement);

// Dialog Data
	enum { IDD = IDD_PROPPAGE_MEDIUM, IDH = IDR_HTML_JUMPLISTSETTINGSWIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
};

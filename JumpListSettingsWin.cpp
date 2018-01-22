// JumpListSettingsWin.cpp : implementation file
//

#include "stdafx.h"
#include "JumpListSettingsWin.h"


// JumpListSettingsWin dialog

IMPLEMENT_DYNCREATE(JumpListSettingsWin, CDHtmlDialog)

JumpListSettingsWin::JumpListSettingsWin(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(JumpListSettingsWin::IDD, JumpListSettingsWin::IDH, pParent)
{

}

JumpListSettingsWin::~JumpListSettingsWin()
{
}

void JumpListSettingsWin::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BOOL JumpListSettingsWin::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(JumpListSettingsWin, CDHtmlDialog)
END_MESSAGE_MAP()

BEGIN_DHTML_EVENT_MAP(JumpListSettingsWin)
	DHTML_EVENT_ONCLICK(_T("ButtonOK"), OnButtonOK)
	DHTML_EVENT_ONCLICK(_T("ButtonCancel"), OnButtonCancel)
END_DHTML_EVENT_MAP()



// JumpListSettingsWin message handlers

HRESULT JumpListSettingsWin::OnButtonOK(IHTMLElement* /*pElement*/)
{
	OnOK();
	return S_OK;
}

HRESULT JumpListSettingsWin::OnButtonCancel(IHTMLElement* /*pElement*/)
{
	OnCancel();
	return S_OK;
}

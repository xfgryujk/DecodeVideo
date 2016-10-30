
// RenderFilterGDIDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RenderFilterGDI.h"
#include "RenderFilterGDIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRenderFilterGDIDlg 对话框



CRenderFilterGDIDlg::CRenderFilterGDIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRenderFilterGDIDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRenderFilterGDIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRenderFilterGDIDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CRenderFilterGDIDlg 消息处理程序

BOOL CRenderFilterGDIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	HRESULT hr;
	m_decoder = new CDecoder(L"E:\\pump it.avi", &hr);
	m_decoder->SetOnPresent(std::bind(&CRenderFilterGDIDlg::OnPresent, this, std::placeholders::_1));

	m_decoder->GetVideoSize(m_videoSize);
	m_dc.Create(m_videoSize.cx, m_videoSize.cy, 32);

	m_decoder->RunVideo();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRenderFilterGDIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialog::OnPaint();

		CPaintDC dc(this);
		CRect rect;
		GetClientRect(&rect);
		m_dcLock.Lock();
		m_dc.StretchBlt(dc, rect);
		m_dcLock.Unlock();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRenderFilterGDIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRenderFilterGDIDlg::OnDestroy()
{
	CDialog::OnDestroy();

	delete m_decoder;

	CoUninitialize();
}

void CRenderFilterGDIDlg::OnPresent(IMediaSample* mediaSample)
{
	BYTE* sampleBuf = NULL;
	if (FAILED(mediaSample->GetPointer(&sampleBuf)))
		return;
	
	// 假设每行之间没有多余的字节
	ASSERT(mediaSample->GetActualDataLength() == m_videoSize.cx * m_videoSize.cy * 4);
	m_dcLock.Lock();
	// RGB位图都是从下到上储存的
	memcpy(m_dc.GetPixelAddress(0, m_videoSize.cy - 1), sampleBuf, m_videoSize.cx * m_videoSize.cy * 4);
	m_dcLock.Unlock();

	Invalidate(FALSE);
}

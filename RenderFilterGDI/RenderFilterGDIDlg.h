
// RenderFilterGDIDlg.h : 头文件
//

#pragma once
#include "Decoder.h"


// CRenderFilterGDIDlg 对话框
class CRenderFilterGDIDlg : public CDialog
{
// 构造
public:
	CRenderFilterGDIDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_RENDERFILTERGDI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();

	void OnPresent(IMediaSample* mediaSample);


	CDecoder* m_decoder = NULL;

	SIZE m_videoSize;
	CImage m_dc;
	CCriticalSection m_dcLock;
};

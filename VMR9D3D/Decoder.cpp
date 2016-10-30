#include "stdafx.h"
#include "Decoder.h"


CDecoder::CDecoder(LPCWSTR fileName, HWND d3dHwnd, IDirect3DDevice9* device) :
	m_d3dHwnd(d3dHwnd),
	m_device(device)
{
	HRESULT hr;
	hr = m_vmr9.CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER);
	hr = m_graph.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
	hr = m_graph.QueryInterface(&m_control);

	hr = m_graph->AddFilter(m_vmr9, L"VMR9");
	hr = m_graph->AddSourceFilter(fileName, L"Source", &m_source);
	
	// 设置呈现器
	CComPtr<IVMRFilterConfig9> vmrConfig;
	hr = m_vmr9.QueryInterface(&vmrConfig);
	hr = vmrConfig->SetRenderingMode(VMR9Mode_Renderless);
	hr = m_vmr9.QueryInterface(&m_vmrNotify);
	hr = m_vmrNotify->AdviseSurfaceAllocator(0, this);
	hr = AdviseNotify(m_vmrNotify);

	// 连接源和渲染器
	CComPtr<IEnumPins> enumPins;
	CComPtr<IPin> sourcePin, renderPin;
	PIN_DIRECTION pinDir;

	m_source->EnumPins(&enumPins);
	while (enumPins->Next(1, &sourcePin, NULL) == S_OK)
	{
		sourcePin->QueryDirection(&pinDir);
		if (pinDir == PINDIR_OUTPUT)
			break;
		sourcePin.Release();
	}
	enumPins.Release();

	m_vmr9->EnumPins(&enumPins);
	while (enumPins->Next(1, &renderPin, NULL) == S_OK)
	{
		renderPin->QueryDirection(&pinDir);
		if (pinDir == PINDIR_INPUT)
			break;
		renderPin.Release();
	}
	enumPins.Release();

	hr = m_graph->Connect(sourcePin, renderPin);
}

void CDecoder::Run()
{
	TRACE(_T("Run\n"));
	HRESULT hr = m_control->Run();
}

void CDecoder::Pause()
{
	TRACE(_T("Pause\n"));
	HRESULT hr = m_control->Pause();
}

void CDecoder::Stop()
{
	TRACE(_T("Stop\n"));
	HRESULT hr = m_control->Stop();
}

void CDecoder::GetVideoSize(SIZE& size)
{
	size = m_videoSize;
}

void CDecoder::SetOnPresent(std::function<void(VMR9PresentationInfo*)>&& onPresent)
{
	m_onPresent = std::move(onPresent);
}

// IUnknown

HRESULT STDMETHODCALLTYPE CDecoder::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (riid == IID_IUnknown)
	{
		*ppvObject = (IUnknown*)(IVMRSurfaceAllocator9*)this;
		return S_OK;
	}
	if (riid == IID_IVMRSurfaceAllocator9)
	{
		*ppvObject = (IVMRSurfaceAllocator9*)this;
		return S_OK;
	}
	if (riid == IID_IVMRImagePresenter9)
	{
		*ppvObject = (IVMRImagePresenter9*)this;
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CDecoder::AddRef(void)
{
	return 0;
}

ULONG STDMETHODCALLTYPE CDecoder::Release(void)
{
	return 0; // 没有严格实现COM组件，防止多次析构
}

// IVMRSurfaceAllocator9

HRESULT STDMETHODCALLTYPE CDecoder::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
	TRACE(_T("AdviseNotify\n"));
	return lpIVMRSurfAllocNotify->SetD3DDevice(m_device, MonitorFromWindow(m_d3dHwnd, MONITOR_DEFAULTTOPRIMARY));
}

HRESULT STDMETHODCALLTYPE CDecoder::InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers)
{
	TRACE(_T("InitializeDevice\n"));
	m_videoSize = lpAllocInfo->szNativeSize;

	m_surfaces.resize(*lpNumBuffers);
	lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface; // 直接创建纹理表面很可能失败，所以创建离屏表面再复制到纹理表面
	HRESULT hr = m_vmrNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_surfaces[0]);
	if (FAILED(hr))
		m_surfaces.clear();
	return hr;
}

HRESULT STDMETHODCALLTYPE CDecoder::TerminateDevice(DWORD_PTR dwID)
{
	TRACE(_T("TerminateDevice\n"));
	if (!m_surfaces.empty())
	{
		for (auto& i : m_surfaces)
			i->Release();
		m_surfaces.clear();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDecoder::GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface)
{
	TRACE(_T("GetSurface(%u)\n"), SurfaceIndex);
	if (SurfaceIndex < 0 || SurfaceIndex >= m_surfaces.size())
	{
		*lplpSurface = NULL;
		return E_FAIL;
	}
	*lplpSurface = m_surfaces[SurfaceIndex];
	(*lplpSurface)->AddRef();
	return S_OK;
}

// IVMRImagePresenter9

HRESULT STDMETHODCALLTYPE CDecoder::StartPresenting(DWORD_PTR dwUserID)
{
	TRACE(_T("StartPresenting\n"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDecoder::StopPresenting(DWORD_PTR dwUserID)
{
	TRACE(_T("StopPresenting\n"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDecoder::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo)
{
	//TRACE(_T("PresentImage(%I64d)\n"), lpPresInfo->rtStart);
	if (!m_onPresent._Empty())
		m_onPresent(lpPresInfo);
	return S_OK;
}

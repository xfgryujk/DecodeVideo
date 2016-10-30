#pragma once
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <vector>
#include <functional>


class CDecoder : private IVMRSurfaceAllocator9, private IVMRImagePresenter9
{
public:
	CDecoder(LPCWSTR fileName, HWND d3dHwnd, IDirect3DDevice9* device);
	virtual ~CDecoder() = default;

	virtual void Run();
	virtual void Pause();
	virtual void Stop();

	virtual void GetVideoSize(SIZE& size);

	// 设置需要呈现时的回调函数
	virtual void SetOnPresent(std::function<void(VMR9PresentationInfo*)>&& onPresent);

protected:
	CComPtr<IGraphBuilder> m_graph;
	CComPtr<IMediaControl> m_control;
	CComPtr<IBaseFilter> m_source;
	CComPtr<IBaseFilter> m_vmr9;

	SIZE m_videoSize;
	// 需要呈现时被调用
	std::function<void(VMR9PresentationInfo*)> m_onPresent;

	// IUnknown

private:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,  _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IVMRSurfaceAllocator9

private:
	HWND m_d3dHwnd;
	IDirect3DDevice9* m_device;

	CComPtr<IVMRSurfaceAllocatorNotify9> m_vmrNotify;

	std::vector<IDirect3DSurface9*> m_surfaces;

	virtual HRESULT STDMETHODCALLTYPE InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers);
	virtual HRESULT STDMETHODCALLTYPE TerminateDevice(DWORD_PTR dwID);
	virtual HRESULT STDMETHODCALLTYPE GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface);
	virtual HRESULT STDMETHODCALLTYPE AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

	// IVMRImagePresenter9

private:
	virtual HRESULT STDMETHODCALLTYPE StartPresenting(DWORD_PTR dwUserID);
	virtual HRESULT STDMETHODCALLTYPE StopPresenting(DWORD_PTR dwUserID);
	virtual HRESULT STDMETHODCALLTYPE PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo);
};

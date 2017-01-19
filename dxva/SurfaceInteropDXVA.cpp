#include "SurfaceInteropDXVA.h"
#include <QDebug>

#ifdef Q_OS_WIN

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#ifdef __MINGW32__
# include <_mingw.h>

# if !defined(__MINGW64_VERSION_MAJOR)
#  undef MS_GUID
#  define MS_GUID DEFINE_GUID /* dxva2api.h fails to declare those, redefine as static */
#  define DXVA2_E_NEW_VIDEO_DEVICE MAKE_HRESULT(1, 4, 4097)
# else
#  include <dxva.h>
# endif

#endif /* __MINGW32__ */

namespace QtAV
{
MS_GUID(IID_IDirect3DSurface9, 0xcfbaf3a, 0x9ff6, 0x429a, 0x99, 0xb3, 0xa2, 0x79, 0x6a, 0xf8, 0xb8, 0x9b);

    class EGLWrapper
    {
    public:
        EGLWrapper();

        __eglMustCastToProperFunctionPointerType getProcAddress(const char *procname);
        EGLSurface createPbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
        EGLBoolean destroySurface(EGLDisplay dpy, EGLSurface surface);
        EGLBoolean bindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
        EGLBoolean releaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

    private:
        typedef __eglMustCastToProperFunctionPointerType(EGLAPIENTRYP EglGetProcAddress)(const char *procname);
        typedef EGLSurface(EGLAPIENTRYP EglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
        typedef EGLBoolean(EGLAPIENTRYP EglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
        typedef EGLBoolean(EGLAPIENTRYP EglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
        typedef EGLBoolean(EGLAPIENTRYP EglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

        EglGetProcAddress m_eglGetProcAddress;
        EglCreatePbufferSurface m_eglCreatePbufferSurface;
        EglDestroySurface m_eglDestroySurface;
        EglBindTexImage m_eglBindTexImage;
        EglReleaseTexImage m_eglReleaseTexImage;
    };


    EGLWrapper::EGLWrapper()
    {
    #ifndef QT_OPENGL_ES_2_ANGLE_STATIC
        // Resolve the EGL functions we use. When configured for dynamic OpenGL, no
        // component in Qt will link to libEGL.lib and libGLESv2.lib. We know
        // however that libEGL is loaded for sure, since this is an ANGLE-only path.

    # ifdef QT_DEBUG
    	HMODULE eglHandle = GetModuleHandle(L"libEGLd.dll");
    # else
        HMODULE eglHandle = GetModuleHandle(L"libEGL.dll");
    # endif

        if (!eglHandle)
            qWarning("No EGL library loaded");

        m_eglGetProcAddress = (EglGetProcAddress)GetProcAddress(eglHandle, "eglGetProcAddress");
        m_eglCreatePbufferSurface = (EglCreatePbufferSurface)GetProcAddress(eglHandle, "eglCreatePbufferSurface");
        m_eglDestroySurface = (EglDestroySurface)GetProcAddress(eglHandle, "eglDestroySurface");
        m_eglBindTexImage = (EglBindTexImage)GetProcAddress(eglHandle, "eglBindTexImage");
        m_eglReleaseTexImage = (EglReleaseTexImage)GetProcAddress(eglHandle, "eglReleaseTexImage");
    #else
        // Static ANGLE-only build. There is no libEGL.dll in use.

        m_eglGetProcAddress = ::eglGetProcAddress;
        m_eglCreatePbufferSurface = ::eglCreatePbufferSurface;
        m_eglDestroySurface = ::eglDestroySurface;
        m_eglBindTexImage = ::eglBindTexImage;
        m_eglReleaseTexImage = ::eglReleaseTexImage;
    #endif
    }

    __eglMustCastToProperFunctionPointerType EGLWrapper::getProcAddress(const char *procname)
    {
        Q_ASSERT(m_eglGetProcAddress);
        return m_eglGetProcAddress(procname);
    }

    EGLSurface EGLWrapper::createPbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
    {
        Q_ASSERT(m_eglCreatePbufferSurface);
        return m_eglCreatePbufferSurface(dpy, config, attrib_list);
    }

    EGLBoolean EGLWrapper::destroySurface(EGLDisplay dpy, EGLSurface surface)
    {
        Q_ASSERT(m_eglDestroySurface);
        return m_eglDestroySurface(dpy, surface);
    }

    EGLBoolean EGLWrapper::bindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
    {
        Q_ASSERT(m_eglBindTexImage);
        return m_eglBindTexImage(dpy, surface, buffer);
    }

    EGLBoolean EGLWrapper::releaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
    {
        Q_ASSERT(m_eglReleaseTexImage);
        return m_eglReleaseTexImage(dpy, surface, buffer);
    }

    SurfaceInteropDXVA::SurfaceInteropDXVA(IDirect3DDevice9 * d3device, int32_t width, int32_t height):
        m_cropWidth(width),
        m_cropHeight(height)
    {
        _d3device = d3device;
        _egl = nullptr;
        _glTexture = 0;
        _dxSurface = nullptr;
        _dxTexture = nullptr;
        _dxQuery = nullptr;
        _dxvaSurface = nullptr;
    }

    SurfaceInteropDXVA::~SurfaceInteropDXVA()
    {
        QMutexLocker lock(&_dxSurfaceMutex);

        if (_egl) {
            if (_eglDisplay && _pboSurface)
                _egl->destroySurface(_eglDisplay, _pboSurface);

            _eglDisplay = nullptr;
            _pboSurface = nullptr;

            delete _egl;
        }
        _egl = nullptr;

        if (_dxQuery)
            _dxQuery->Release();
        _dxQuery = nullptr;

       if (_dxSurface)
           _dxSurface->Release();
       _dxSurface = nullptr;

       if (_dxTexture)
           _dxTexture->Release();
       _dxTexture = nullptr;

       if (_dxvaSurface)
           _dxvaSurface->Release();
       _dxvaSurface = nullptr;
    }

    void SurfaceInteropDXVA::setSurface(IDirect3DSurface9 * surface)
    {
        QMutexLocker lock(&_dxvaSurfaceMutex);

        if (_dxvaSurface)
        {
            _dxvaSurface->Release();
            _dxvaSurface = nullptr;
        }

        if (surface)
        {
            surface->QueryInterface(IID_IDirect3DSurface9, (void**)&_dxvaSurface);
        }
    }

    IDirect3DSurface9 * SurfaceInteropDXVA::getSurface()
    {
        return _dxvaSurface;
    }

    void* SurfaceInteropDXVA::map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane)
    {
        QMutexLocker lock(&_dxSurfaceMutex);

        if (!fmt.isRGB())
            return 0;

        if (!handle || !_dxvaSurface)
            return NULL;

        if (type == GLTextureSurface)
        {
            HRESULT hr = S_OK;

			if (!_glTexture)
			{
				_glTexture = *((GLint*)handle);

				int32_t width = 0;
				int32_t height = 0;

				D3DSURFACE_DESC dxvaDesc;
                hr = _dxvaSurface ? _dxvaSurface->GetDesc(&dxvaDesc) : S_FALSE;

                if (!SUCCEEDED(hr)) {
                    width = 0;
                    height = 0;
                } else {
                    width = m_cropWidth > 0 ? m_cropWidth : dxvaDesc.Width;
                    height = m_cropHeight > 0 ? m_cropHeight : dxvaDesc.Height;
                }

				m_width = width;
				m_height = height;

				QOpenGLContext *currentContext = QOpenGLContext::currentContext();
				if (!_egl)
					_egl = new EGLWrapper;

				HANDLE share_handle = NULL;
				QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
				_eglDisplay = static_cast<EGLDisplay*>(nativeInterface->nativeResourceForContext("eglDisplay", currentContext));
				_eglConfig = static_cast<EGLConfig*>(nativeInterface->nativeResourceForContext("eglConfig", currentContext));


				bool hasAlpha = currentContext->format().hasAlpha();

				EGLint attribs[] = {
					EGL_WIDTH, width,
					EGL_HEIGHT, height,
					EGL_TEXTURE_FORMAT, hasAlpha ? EGL_TEXTURE_RGBA : EGL_TEXTURE_RGB,
					EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
					EGL_NONE
				};


				_pboSurface = _egl->createPbufferSurface(	_eglDisplay,
															_eglConfig,
															attribs);

				if (_pboSurface != nullptr)
				{
					PFNEGLQUERYSURFACEPOINTERANGLEPROC eglQuerySurfacePointerANGLE = reinterpret_cast<PFNEGLQUERYSURFACEPOINTERANGLEPROC>(_egl->getProcAddress("eglQuerySurfacePointerANGLE"));
					Q_ASSERT(eglQuerySurfacePointerANGLE);
					int ret = eglQuerySurfacePointerANGLE(_eglDisplay,
						_pboSurface,
						EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, &share_handle);

					if (share_handle && ret == EGL_TRUE)
					{
						hr = _d3device->CreateTexture(width, height, 1,
							D3DUSAGE_RENDERTARGET,
							hasAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
							D3DPOOL_DEFAULT,
							&_dxTexture,
							&share_handle);

						if (SUCCEEDED(hr))
						{
							hr = _dxTexture->GetSurfaceLevel(0, &_dxSurface);
						}
					}
				}
				else
				{
					_glTexture = 0;
				}
            }

            if (_glTexture > 0)
            {
                QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, _glTexture);
                QOpenGLContext::currentContext()->functions()->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                QOpenGLContext::currentContext()->functions()->glPixelStorei(GL_PACK_ALIGNMENT, 1);

                if(m_width > 0 && m_height > 0 )
                {
                    RECT origin;
                    origin.left = 0;
                    origin.top = 0;
                    origin.right = m_width;
                    origin.bottom = m_height;


                    {
                        QMutexLocker lock(&_dxvaSurfaceMutex);

                        if(_dxvaSurface)
                            hr = _d3device->StretchRect(_dxvaSurface, &origin, _dxSurface, NULL, D3DTEXF_NONE);
                    }
                }
                else
                {
                    {
                        QMutexLocker lock(&_dxvaSurfaceMutex);

                        if(_dxvaSurface)
                            hr = _d3device->StretchRect(_dxvaSurface, NULL, _dxSurface, NULL, D3DTEXF_NONE);
                    }
                }

                if (SUCCEEDED(hr) && !_dxQuery && _dxSurface)
                {
                    IDirect3DDevice9 * d = nullptr;

                    if(SUCCEEDED(_dxSurface->GetDevice(&d)))
                    {
                        if(d && SUCCEEDED(d->CreateQuery(D3DQUERYTYPE_EVENT, &_dxQuery)))
                        {
                            qDebug() << "DX Query created on output surface device!";
                        }

                        d->Release();
                    }
                    else
                    {
                        qWarning() << "Failed to obtain device for output surface";
                    }
                }

                if (_dxQuery)
                {
                    _dxQuery->Issue(D3DISSUE_END);  // Flush the target device to render
                    int attempts = 0;
                    while ((_dxQuery->GetData(NULL, 0, D3DGETDATA_FLUSH) == FALSE) && attempts++ < 10)
                    {
                        qDebug() << "Called GetData and failed, waiting...";
                        Sleep(1);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    _egl->bindTexImage(_eglDisplay, _pboSurface, EGL_BACK_BUFFER);
                }
                else
                {
                    qWarning() << "Failed to copy dxsurface";
                }

                QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, 0);
            }

            return handle;
        }
        else {
            if (type == HostMemorySurface) {
            }
            else {
                return 0;
            }
        }

        return handle;
    }
    void SurfaceInteropDXVA::unmap(void *handle)
    {
        QMutexLocker lock(&_dxSurfaceMutex);

        _glTexture = 0;
         _egl->destroySurface(_eglDisplay, _pboSurface);

        if (_dxSurface)
        {
            _dxSurface->Release();
            _dxSurface = nullptr;
        }

        if (_dxTexture)
        {
            _dxTexture->Release();
            _dxTexture = nullptr;
        }
    }
    void* SurfaceInteropDXVA::createHandle(SurfaceType type, const VideoFormat& fmt, int plane)
    {
        return nullptr;
    }
}
#endif

#ifndef SURFACEINTEROPDXVA_H
#define SURFACEINTEROPDXVA_H

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "QtAV/SurfaceInterop.h"
#define EGL_EGLEXT_PROTOTYPES


#include <QtANGLE/EGL/egl.h>
#include <QtANGLE/EGL/eglext.h>
#include <qpa/qplatformnativeinterface.h>
#include <qtgui/qguiapplication.h>
#include <qtgui/qopenglcontext.h>
#include <QOpenGLContext>
#include <QtPlatformHeaders/QEGLNativeContext>
#include <qopenglfunctions.h>
#include <d3d9.h>
#include <dxva2api.h>

#include <QMutex>

namespace QtAV
{
    class EGLWrapper;
    class Q_AV_EXPORT SurfaceInteropDXVA : public VideoSurfaceInterop
    {
    public:
        SurfaceInteropDXVA(IDirect3DDevice9 * d3device, int32_t width = -1, int32_t height = -1);
        ~SurfaceInteropDXVA();
        void setSurface(IDirect3DSurface9 * surface);
        IDirect3DSurface9 * getSurface();
        virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane);
        virtual void unmap(void *handle);
        virtual void* createHandle(SurfaceType type, const VideoFormat& fmt, int plane = 0);

    private:
        IDirect3DSurface9 * _dxvaSurface;
        IDirect3DDevice9 * _d3device;
        EGLWrapper * _egl;
        GLint _glTexture;
        EGLSurface _pboSurface;
        IDirect3DQuery9 * _dxQuery;
        IDirect3DTexture9 * _dxTexture;
        IDirect3DSurface9 * _dxSurface;
        EGLDisplay _eglDisplay;
        EGLConfig _eglConfig;
        int32_t m_width;
        int32_t m_height;
        int32_t m_cropWidth;
        int32_t m_cropHeight;
        QMutex _dxSurfaceMutex;
        QMutex _dxvaSurfaceMutex;
    };
    typedef QSharedPointer<SurfaceInteropDXVA> SurfaceInteropDXVAPtr;
}
#endif
#endif // SURFACEINTEROPDXVA_H

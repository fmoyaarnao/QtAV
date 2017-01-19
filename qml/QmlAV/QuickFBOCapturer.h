#ifndef QUICKFBOCAPTURER_H
#define QUICKFBOCAPTURER_H

#include "QmlAV/QuickFBORenderer.h"

class QuickFBOCapturer: public QtAV::QuickFBORenderer
{
    Q_OBJECT
protected:
    virtual bool receiveFrame(const QtAV::VideoFrame &frame) Q_DECL_OVERRIDE;

signals:
    void frameCaptured(const QtAV::VideoFrame & frame);
};

#endif // QUICKFBOCAPTURER_H

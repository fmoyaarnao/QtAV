#ifndef QTAV_QUICKFBOCAPTURER_H
#define QTAV_QUICKFBOCAPTURER_H

#include <QmlAV/QuickFBORenderer.h>

namespace QtAV {
class QuickFBOCapturer: public QuickFBORenderer
{
    Q_OBJECT
public:
    explicit QuickFBOCapturer(QQuickItem *parent = 0);

Q_SIGNALS:
    void frameCaptured(const QtAV::VideoFrame & frame);

protected:
    virtual bool receiveFrame(const VideoFrame &frame) Q_DECL_OVERRIDE;
};
} // namespace QtAV
#endif // QTAV_QUICKFBOCAPTURER_H

#include "QmlAV/QuickFBOCapturer.h"

namespace QtAV {
QuickFBOCapturer::QuickFBOCapturer(QQuickItem *parent) : QuickFBORenderer(parent) {
}

bool QuickFBOCapturer::receiveFrame(const VideoFrame &frame)
{
    emit frameCaptured(frame);
    return QuickFBORenderer::receiveFrame(frame);
}
} // namespace QtAV

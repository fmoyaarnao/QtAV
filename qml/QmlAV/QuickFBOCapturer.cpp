#include "QmlAV/QuickFBOCapturer.h"

bool QuickFBOCapturer::receiveFrame(const QtAV::VideoFrame &frame)
{
    emit frameCaptured(frame);
    return QuickFBORenderer::receiveFrame(frame);
}

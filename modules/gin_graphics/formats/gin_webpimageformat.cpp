/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

extern "C"
{
#include "../3rdparty/webp/webp/decode.h"
}

juce::String WEBPImageFormat::getFormatName()
{
    return "WebP";
}

bool WEBPImageFormat::canUnderstand (juce::InputStream& input)
{
    return true;
}

bool WEBPImageFormat::usesFileExtension (const juce::File& possibleFile)
{
    return possibleFile.hasFileExtension ("webp");
}

juce::Image WEBPImageFormat::decodeImage (juce::InputStream& input)
{
    juce::MemoryBlock mb;
    input.readIntoMemoryBlock (mb);
    
    int w = 0, h = 0;
    if (! WebPGetInfo ((uint8_t*)mb.getData(), mb.getSize(), &w, &h))
        return {};

    return {};
}

bool WEBPImageFormat::writeImageToStream (const juce::Image& sourceImage, juce::OutputStream& dst)
{
    return false;
}

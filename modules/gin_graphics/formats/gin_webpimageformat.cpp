/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wzero-as-null-pointer-constant")

extern "C"
{
#include "../3rdparty/webp/webp/decode.h"
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

juce::String WEBPImageFormat::getFormatName()
{
    return "WebP";
}

bool WEBPImageFormat::canUnderstand (juce::InputStream& input)
{
    juce::MemoryBlock mb;
    input.setPosition (0);
    input.readIntoMemoryBlock (mb, 12);
    
    if (mb.getSize() < 12)
        return false;
    
    auto riff = std::memcmp (mb.begin(), "RIFF", 4) == 0;
    auto webp = std::memcmp (mb.begin() + 8, "WEBP", 4) == 0;

    return riff && webp;
}

bool WEBPImageFormat::usesFileExtension (const juce::File& possibleFile)
{
    return possibleFile.hasFileExtension ("webp");
}

juce::Image WEBPImageFormat::decodeImage (juce::InputStream& input)
{
    juce::MemoryBlock mb;
    input.setPosition (0);
    input.readIntoMemoryBlock (mb);
    
    int w = 0, h = 0;
    if (! WebPGetInfo ((uint8_t*)mb.begin(), mb.getSize(), &w, &h))
        return {};
    
    juce::Image img (juce::Image::ARGB, w, h, true);
    
    juce::Image::BitmapData data (img, juce::Image::BitmapData::readWrite);
    
    WebPDecodeBGRAInto ((uint8_t*)mb.begin(), mb.getSize(), data.data, data.size, data.lineStride);

    return img;
}

bool WEBPImageFormat::writeImageToStream (const juce::Image& sourceImage, juce::OutputStream& dst)
{
    juce::ignoreUnused (sourceImage, dst);
    return false;
}

/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== Notes.txt ==================
static const unsigned char temp_binary_data_0[] =
"\n"
"// fft :: 2^N sized fft -- 2^11 = 2048, ~23.4fps\n"
"// to maintain the same framerate (~23.4fps) i.e. update every 2048 samples\n"
"// -> we can use 50% overlap FFTN = 4096    -> copy i=2048~4095(end) to new buffer i=0~2047\n"
"// or we can use 75% overlap FFTN = 8192    -> copy i=2048~8191(end) to new buffer i=0~6143\n"
"\n"
"make FFTORDER an adjustable variable\n"
"re-initiate all instances related to using FFTORDER when switching to different overlap\n"
"\n"
"maintain 2048 sample - refresh rate (23.4fps)\n"
"\n"
" FA line 73   std::vector<float> getBuffer() const { return oBuffer; }    /// why does this create bad access when init new window 3 times? is it because of shared ptr in DWmixer?\n"
"\n"
"1. implement 2/4 overlap\n"
"\n"
"2. implement 3/4 overlap\n"
"\n"
"3. implement reset for choosing 2/4 and 3/4\n"
"\n"
"4. add menu selector in UI\n"
"\n"
"\n"
"\n"
"\n";

const char* Notes_txt = (const char*) temp_binary_data_0;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0xe2aa17d2:  numBytes = 793; return Notes_txt;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "Notes_txt"
};

const char* originalFilenames[] =
{
    "Notes.txt"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}

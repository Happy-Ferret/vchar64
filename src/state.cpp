#include "state.h"

#include <algorithm>
#include <QFile>
#include <QFileInfo>

#include "import.h"

static State *__instance = nullptr;

const int State::CHAR_BUFFER_SIZE;

State* State::getInstance()
{
    if (!__instance)
        __instance = new State();

    return __instance;
}

State::State()
    : _charIndex(0)
    , _totalChars(0)
    , _multiColor(false)
    , _colors{1,0,12,15}
    , _selectedColorIndex(1)
{
    loadCharSet(":/c64-chargen.bin");
}

State::~State()
{

}

bool State::loadCharSet(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    qint64 length=0;

    QFileInfo info(file);
    if (info.suffix() == "64c")
    {
        length = Import::load64C(file, this);
    }
    else if(info.suffix() == "ctm")
    {
        length = Import::loadCTM(file, this);
    }
    else
    {
        length = Import::loadRaw(file, this);
    }

    file.close();

    if(length<=0)
        return false;

    return true;
}

int State::getCharColor(int charIndex, int bitIndex) const
{
    Q_ASSERT(charIndex >=0 && charIndex < 256 && "Invalid charIndex. Valid range: 0,255");
    Q_ASSERT(bitIndex >=0 && bitIndex < 64 && "Invalid bit. Valid range: 0,63");

    char c = _chars[charIndex*8 + bitIndex/8];
    int b = bitIndex%8;
    int mask = 1 << (7-b);

   return (c & mask);
}

void State::setCharColor(int charIndex, int bitIndex, int colorIndex)
{
    Q_ASSERT(charIndex >=0 && charIndex < 256 && "Invalid charIndex. Valid range: 0,256");
    Q_ASSERT(bitIndex >=0 && bitIndex < 64 && "Invalid bit. Valid range: 0,64");
    Q_ASSERT(colorIndex >=0 && colorIndex < 4 && "Invalid colorIndex. range: 0,4");

//    if (_multiColor)
    int bits_to_mask = 1;
    int totalbits = 64;
    int modulus = 8;

    if (_multiColor) {
        bits_to_mask = 3;
        totalbits = 32;
        modulus = 4;

        // only care about even bits in multicolor
        bitIndex &= 0xfe;
    }


    // get the needed line ignoring whether it is multicolor or not
    char c = _chars[charIndex*8 + bitIndex/8];

    // for multicolor, we need to get the modulus
    // in different ways regarding it is multicolor or not

    int factor = 64/totalbits;
    int b = (bitIndex/factor) % modulus;
    int mask = bits_to_mask << ((modulus-1)-b) * factor;

    // turn off bits
    c &= ~mask;

    // and 'or' it with colorIndex
    c |= colorIndex << ((modulus-1)-b) * factor;

    _chars[charIndex*8 + bitIndex/8] = c;
}

char* State::getCharsBuffer()
{
    return _chars;
}

void State::resetCharsBuffer()
{
    memset(_chars, 0, sizeof(_chars));
}

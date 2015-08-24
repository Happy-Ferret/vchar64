/****************************************************************************
Copyright 2015 Ricardo Quesada

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/

#include "state.h"

#include <string.h>

#include <algorithm>
#include <QFile>
#include <QFileInfo>

#include "stateimport.h"
#include "stateexport.h"

static State *__instance = nullptr;

const int State::CHAR_BUFFER_SIZE;

State* State::getInstance()
{
    if (!__instance)
        __instance = new State();

    return __instance;
}

State::State()
    : _totalChars(0)
    , _multiColor(false)
    , _selectedColorIndex(3)
    , _colors{1,12,15,0}
    , _tileProperties{{1,1},1}
    , _loadedFilename("")
    , _savedFilename("")
    , _exportedFilename("")
    , _exportedAddress(-1)
    , _undoStack(nullptr)
{
    memset(_copyTile, 0, sizeof(_copyTile));
    _undoStack = new QUndoStack;
}

State::~State()
{
    delete _undoStack;
}

void State::reset()
{
    _totalChars = 0;
    _multiColor = false;
    _selectedColorIndex = 3;
    _colors[0] = 1;
    _colors[1] = 12;
    _colors[2] = 15;
    _colors[3] = 0;
    _tileProperties.size = {1,1};
    _tileProperties.interleaved = 1;
    _loadedFilename = "";
    _savedFilename = "";
    _exportedFilename = "";
    _exportedAddress = -1;

    memset(_chars, 0, sizeof(_chars));

    _undoStack->clear();

    emit fileLoaded();
    emit contentsChanged();
}

bool State::isModified() const
{
    return (!_undoStack->isClean());
}

bool State::export_()
{
    Q_ASSERT(_exportedFilename.length()>0 && "Invalid filename");

    if (_exportedAddress == -1)
        return exportRaw(_exportedFilename);
    else
        return exportPRG(_exportedFilename, (quint16)_exportedAddress);
}

bool State::exportRaw(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        return false;

    if (StateExport::saveRaw(this, file) > 0)
    {
        _exportedAddress = -1;
        _exportedFilename = filename;
        return true;
    }
    return false;
}

bool State::exportPRG(const QString& filename, quint16 address)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        return false;

    if (StateExport::savePRG(this, file, address) > 0)
    {
        _exportedAddress = address;
        _exportedFilename = filename;
        return true;
    }
    return false;
}

bool State::openFile(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    qint64 length=0;
    quint16 loadedAddress;

    enum {
        FILETYPE_VCHAR64,
        FILETYPE_PRG,
        FILETYPE_RAW,
        FILETYPE_CTM
    };
    int filetype = FILETYPE_RAW;


    QFileInfo info(file);
    if (info.suffix() == "vchar64proj")
    {
        length = StateImport::loadVChar64(this, file);
        filetype = FILETYPE_VCHAR64;
    }
    else if ((info.suffix() == "64c") || (info.suffix() == "prg"))
    {
        length = StateImport::loadPRG(this, file, &loadedAddress);
        filetype = FILETYPE_PRG;
    }
    else if(info.suffix() == "ctm")
    {
        length = StateImport::loadCTM(this, file);
        filetype = FILETYPE_CTM;
    }
    else
    {
        length = StateImport::loadRaw(this, file);
        filetype = FILETYPE_RAW;
    }

    file.close();

    if(length<=0)
        return false;

    // if a new file is loaded, then reset the exported and saved values
    _savedFilename = "";
    _exportedFilename = "";
    _exportedAddress = -1;

    // built-in resources are not saved
    if (filename[0] != ':')
    {
        _loadedFilename = filename;

        if (filetype == FILETYPE_VCHAR64)
        {
            _savedFilename = filename;
        }
        else if (filetype == FILETYPE_RAW || filetype == FILETYPE_PRG)
        {
            _exportedFilename = filename;
            if (filetype == FILETYPE_PRG) {
                _exportedAddress = loadedAddress;
            }
        }
    }

    _undoStack->clear();

    emit fileLoaded();
    emit contentsChanged();

    return true;
}

bool State::saveProject(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        return false;

    qint64 length=0;

    length = StateExport::saveVChar64(this, file);

    file.close();

    if(length<=0)
        return false;

    _savedFilename = filename;

    _undoStack->clear();

    emit contentsChanged();

    return true;
}

//
void State::setMultiColor(bool enabled)
{
    if (_multiColor != enabled)
    {
        _multiColor = enabled;

        emit colorPropertiesUpdated();
        emit contentsChanged();
    }
}

void State::setColorAtIndex(int colorIndex, int color)
{
    Q_ASSERT(colorIndex >=0 && colorIndex < 4);
    Q_ASSERT(color >=0 && color < 16);
    _colors[colorIndex] = color;

    emit colorPropertiesUpdated();
    emit contentsChanged();
}

int State::getTileColorAt(int tileIndex, const QPoint& position)
{
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    Q_ASSERT(position.x()<State::MAX_TILE_WIDTH*8 && position.y()<State::MAX_TILE_HEIGHT*8 && "Invalid position");

    int x = position.x();
    int y = position.y();
    int bitIndex = (x%8) + (y%8) * 8;
    int charIndex = getCharIndexFromTileIndex(tileIndex)
            + (x/8) * _tileProperties.interleaved
            + (y/8) * _tileProperties.interleaved * _tileProperties.size.width();

    char c = _chars[charIndex*8 + bitIndex/8];
    int b = bitIndex%8;
    int mask = 1 << (7-b);

   return (c & mask);
}

void State::tilePaint(int tileIndex, const QPoint& position, int colorIndex)
{
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    Q_ASSERT(position.x()<State::MAX_TILE_WIDTH*8 && position.y()<State::MAX_TILE_HEIGHT*8 && "Invalid position");
    Q_ASSERT(colorIndex >=0 && colorIndex < 4 && "Invalid colorIndex. range: 0,4");

    int x = position.x();
    int y = position.y();
    int bitIndex = (x%8) + (y%8) * 8;
    int charIndex = getCharIndexFromTileIndex(tileIndex)
            + (x/8) * _tileProperties.interleaved
            + (y/8) * _tileProperties.interleaved * _tileProperties.size.width();

    int byteIndex = charIndex*8 + bitIndex/8;

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
    quint8 c = _chars[byteIndex];

    // for multicolor, we need to get the modulus
    // in different ways regarding it is multicolor or not

    int factor = 64/totalbits;
    int b = (bitIndex/factor) % modulus;
    int mask = bits_to_mask << ((modulus-1)-b) * factor;

    // turn off bits
    c &= ~mask;

    // and 'or' it with colorIndex
    c |= colorIndex << ((modulus-1)-b) * factor;

    quint8 oldValue = _chars[byteIndex];
    if (oldValue != c) {
        _chars[byteIndex] = c;

        emit byteUpdated(byteIndex);
        emit contentsChanged();
    }
}

void State::setTileProperties(const TileProperties& properties)
{
    if (memcmp(&_tileProperties, &properties, sizeof(_tileProperties)) != 0) {
        _tileProperties = properties;

        emit tilePropertiesUpdated();
        emit contentsChanged();
    }
}

quint8* State::getCharsBuffer()
{
    return _chars;
}

void State::resetCharsBuffer()
{
    memset(_chars, 0, sizeof(_chars));
}

// buffer must be at least 8x8*8 bytes big
void State::copyCharFromIndex(int tileIndex, quint8* buffer, int bufferSize)
{
    int tileSize = _tileProperties.size.width() * _tileProperties.size.height() * 8;
    Q_ASSERT(bufferSize >= tileSize && "invalid bufferSize. Too small");
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    memcpy(buffer, &_chars[tileIndex*tileSize], tileSize);
}

// size-of-tile chars will be copied
void State::copyCharToIndex(int tileIndex, quint8* buffer, int bufferSize)
{
    int tileSize = _tileProperties.size.width() * _tileProperties.size.height() * 8;
    Q_ASSERT(bufferSize >= tileSize && "invalid bufferSize. Too small");
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    memcpy(&_chars[tileIndex*tileSize], buffer, tileSize);

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}


// helper functions
int State::getCharIndexFromTileIndex(int tileIndex) const
{
    int charIndex = tileIndex;
    if (_tileProperties.interleaved==1) {
        charIndex *= (_tileProperties.size.width() * _tileProperties.size.height());
    }

    return charIndex;
}

int State::getTileIndexFromCharIndex(int charIndex) const
{
    int tileIndex = charIndex;
    if (_tileProperties.interleaved==1) {
        tileIndex /= (_tileProperties.size.width() * _tileProperties.size.height());
    }

    return tileIndex;
}


// tile manipulation
void State::tileCopy(int tileIndex)
{
    int tileSize = _tileProperties.size.width() * _tileProperties.size.height() * 8;
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    memcpy(_copyTile, &_chars[tileIndex*tileSize], tileSize);
}

void State::tilePaste(int tileIndex)
{
    int tileSize = _tileProperties.size.width() * _tileProperties.size.height() * 8;
    Q_ASSERT(tileIndex>=0 && tileIndex<getTileIndexFromCharIndex(256) && "invalid index value");
    memcpy(&_chars[tileIndex*tileSize], _copyTile, tileSize);

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileInvert(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {
            for (int i=0; i<8; i++) {
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = ~charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];
            }
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileClear(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {
            for (int i=0; i<8; i++) {
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = 0;
            }
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileFlipHorizontally(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    // flip bits
    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {

            for (int i=0; i<8; i++) {
                char tmp = 0;
                for (int j=0; j<8; j++) {
                    if (charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] & (1<<j))
                        tmp |= 1 << (7-j);
                }
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = tmp;
            }
        }
    }

    // swap the chars
    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width()/2; x++) {
            for (int i=0; i<8; i++) {
                std::swap(charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved],
                        charPtr[i+(_tileProperties.size.width()-1-x+y*_tileProperties.size.width())*8*_tileProperties.interleaved]);
            }
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileFlipVertically(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    // flip bits
    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {

            for (int i=0; i<4; i++) {
                std::swap(charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved],
                        charPtr[7-i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved]);
            }
        }
    }

    // swap the chars
    for (int y=0; y<_tileProperties.size.height()/2; y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {
            for (int i=0; i<8; i++) {
                std::swap(charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved],
                        charPtr[i+(x+(_tileProperties.size.height()-1-y)*_tileProperties.size.width())*8*_tileProperties.interleaved]);
            }
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileRotate(int tileIndex)
{
    Q_ASSERT(_tileProperties.size.width() == _tileProperties.size.height() && "Only square tiles can be rotated");

    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);


    // rotate each char (its bits) individually
    for (int y=0; y<_tileProperties.size.height(); y++) {
        for (int x=0; x<_tileProperties.size.width(); x++) {

            Char tmpchr;
            memset(tmpchr._char8, 0, sizeof(tmpchr));

            for (int i=0; i<8; i++) {
                for (int j=0; j<8; j++) {
                    if (charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] & (1<<(7-j)))
                        tmpchr._char8[j] |= (1<<i);
                }
            }

            setCharForTile(tileIndex, x, y, tmpchr);
        }
    }


    // replace the chars in the correct order.
    if (_tileProperties.size.width()>1) {
        Char tmpchars[_tileProperties.size.width()*_tileProperties.size.height()];

        // place the rotated chars in a rotated tmp buffer
        for (int y=0; y<_tileProperties.size.height(); y++) {
            for (int x=0; x<_tileProperties.size.width(); x++) {
                // rotate them: tmpchars[w-y-1,x] = tile[x,y];
                tmpchars[(_tileProperties.size.width()-1-y) + x * _tileProperties.size.width()] = getCharFromTile(tileIndex, x, y);
            }
        }

        // place the rotated tmp buffer in the final position
        for (int y=0; y<_tileProperties.size.height(); y++) {
            for (int x=0; x<_tileProperties.size.width(); x++) {
                    setCharForTile(tileIndex, x, y, tmpchars[x + _tileProperties.size.width() * y]);
            }
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileShiftLeft(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);


    // top tile first
    for (int y=0; y<_tileProperties.size.height(); y++) {

        // top byte of
        for (int i=0; i<8; i++) {

            bool leftBit = false;
            bool prevLeftBit = false;

            for (int x=_tileProperties.size.width()-1; x>=0; x--) {
                leftBit = charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] & (1<<7);

                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] <<= 1;

                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] &= 254;
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] |= prevLeftBit;

                prevLeftBit = leftBit;
            }
            charPtr[i+(_tileProperties.size.width()-1+y*_tileProperties.size.width())*8*_tileProperties.interleaved] &= 254;
            charPtr[i+(_tileProperties.size.width()-1+y*_tileProperties.size.width())*8*_tileProperties.interleaved] |= leftBit;
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileShiftRight(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);


    // top tile first
    for (int y=0; y<_tileProperties.size.height(); y++) {

        // top byte of
        for (int i=0; i<8; i++) {

            bool rightBit = false;
            bool prevRightBit = false;

            for (int x=0; x<_tileProperties.size.width(); x++) {
                rightBit = charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] & (1<<0);

                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] >>= 1;

                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] &= 127;
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] |= (prevRightBit<<7);

                prevRightBit = rightBit;
            }
            charPtr[i+(0+y*_tileProperties.size.width())*8*_tileProperties.interleaved] &= 127;
            charPtr[i+(0+y*_tileProperties.size.width())*8*_tileProperties.interleaved] |= (rightBit<<7);
        }
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileShiftUp(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    for (int x=0; x<_tileProperties.size.width(); x++) {

        // bottom byte of bottom
        qint8 topByte, prevTopByte = 0;

        for (int y=_tileProperties.size.height()-1; y>=0; y--) {

            topByte = charPtr[0+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];

            for (int i=0; i<7; i++) {
                charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = charPtr[i+1+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];
            }

            charPtr[7+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = prevTopByte;
            prevTopByte = topByte;
        }
        // replace bottom byte (y=height-1) with top byte
        charPtr[7+(x+(_tileProperties.size.height()-1)*_tileProperties.size.width())*8*_tileProperties.interleaved] = prevTopByte;
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}

void State::tileShiftDown(int tileIndex)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);
    quint8* charPtr = getCharAtIndex(charIndex);

    for (int x=0; x<_tileProperties.size.width(); x++) {

        // bottom byte of bottom
        qint8 bottomByte, prevBottomByte = 0;

        for (int y=0; y<_tileProperties.size.height(); y++) {

            bottomByte = charPtr[7+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];

            for (int i=6; i>=0; i--) {
                charPtr[i+1+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = charPtr[i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];
            }

            charPtr[0+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = prevBottomByte;
            prevBottomByte = bottomByte;
        }
        // replace top byte (y=0) with bottom byte
        charPtr[x*8*_tileProperties.interleaved] = prevBottomByte;
    }

    emit tileUpdated(tileIndex);
    emit contentsChanged();
}


//
// Helpers
// They must not emit signals
//
State::Char State::getCharFromTile(int tileIndex, int x, int y) const
{
    Char ret;
    int charIndex = getCharIndexFromTileIndex(tileIndex);

    for (int i=0; i<8; i++) {
        ret._char8[i] = _chars[charIndex*8+i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved];
    }
    return ret;
}

void State::setCharForTile(int tileIndex, int x, int y, const Char& chr)
{
    int charIndex = getCharIndexFromTileIndex(tileIndex);

    for (int i=0; i<8; i++) {
        _chars[charIndex*8+i+(x+y*_tileProperties.size.width())*8*_tileProperties.interleaved] = chr._char8[i];
    }
}


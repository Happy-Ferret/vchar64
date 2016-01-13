/****************************************************************************
Copyright 2016 Ricardo Quesada

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

#include "importkoalaorigwidget.h"

#include <functional>

#include <QPainter>
#include <QPaintEvent>
#include <QFile>
#include <QDebug>

#include "palette.h"
#include "state.h"
#include "mainwindow.h"

static const int PIXEL_SIZE = 1;
static const int COLUMNS = 40;
static const int ROWS = 25;
static const int OFFSET = 0;

ImportKoalaOrigWidget::ImportKoalaOrigWidget(QWidget *parent)
    : QWidget(parent)
    , _offsetX(0)
    , _offsetY(0)
    , _displayGrid(false)
{
    memset(_framebuffer, 0, sizeof(_framebuffer));
    setFixedSize(PIXEL_SIZE * COLUMNS * 8 + OFFSET * 2,
                 PIXEL_SIZE * ROWS * 8 + OFFSET * 2);
}

//
// Overriden
//
void ImportKoalaOrigWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;

    painter.begin(this);
    painter.fillRect(event->rect(), QWidget::palette().color(QWidget::backgroundRole()));

    painter.setBrush(QColor(0,0,0));
    painter.setPen(Qt::NoPen);

    for (int y=0; y<200; y++)
    {
        for (int x=0; x<160; ++x)
        {
            painter.setBrush(Palette::getColor(_framebuffer[y * 160 + x]));
            painter.drawRect((x*2) * PIXEL_SIZE + OFFSET,
                             y * PIXEL_SIZE + OFFSET,
                             PIXEL_SIZE * 2,
                             PIXEL_SIZE);
        }
    }

    if (_displayGrid)
    {
        auto pen = painter.pen();
        pen.setColor(QColor(0,128,0));
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);

        for (int y=0; y<=200; y=y+8)
            painter.drawLine(QPointF(0,y), QPointF(320,y));

        for (int x=0; x<=320; x=x+8)
            painter.drawLine(QPointF(x,0), QPointF(x,200));
    }

    painter.end();
}

//
// public
//
void ImportKoalaOrigWidget::loadKoala(const QString& koalaFilepath)
{
    // call it before updating the _koalaBuffer
    resetOffset();
    resetColors();

    // in case the loaded file has less bytes than required, fill the buffer with zeroes
    memset(&_koalaCopy, 0, sizeof(_koalaCopy));

    QFile file(koalaFilepath);
    file.open(QIODevice::ReadOnly);
    file.read((char*)&_koalaCopy, sizeof(_koalaCopy));

    _koala = _koalaCopy;

    toFrameBuffer();
    findUniqueChars();
}

void ImportKoalaOrigWidget::enableGrid(bool enabled)
{
    if (_displayGrid != enabled)
    {
        _displayGrid = enabled;
        update();
    }
}

//
// protected
//
void ImportKoalaOrigWidget::resetColors()
{
    // reset state
    _colorsUsed.clear();
    for (int i=0; i<16; i++)
        _colorsUsed.push_back(std::make_pair(0,i));
    _uniqueChars.clear();

    for (int i=0; i<3; i++)
        _d02xColors[i] = -1;
}

void ImportKoalaOrigWidget::resetOffset()
{
    if (_offsetX != 0 || _offsetY != 0)
    {
        _koala = _koalaCopy;
        _offsetX = 0;
        _offsetY = 0;
    }
}

void ImportKoalaOrigWidget::setOffset(int offsetx, int offsety)
{
    Q_UNUSED(offsetx);
    Q_UNUSED(offsety);
}

void ImportKoalaOrigWidget::findUniqueChars()
{
    static const char hex[] = "0123456789ABCDEF";

    for (int y=0; y<25; ++y)
    {
        for (int x=0; x<40; ++x)
        {
            // 8 * 4
            char key[33];
            key[32] = 0;

            for (int i=0; i<8; ++i)
            {
                for (int j=0; j<4; ++j)
                {
                    auto colorIndex = _framebuffer[(y * 8 + i) * 160 + (x * 4 + j)];
                    key[i*4+j] = hex[colorIndex];
                    _colorsUsed[colorIndex].first++;
                }
            }
            std::string skey(key);

            if (_uniqueChars.find(skey) == _uniqueChars.end())
            {
                std::vector<std::pair<int,int>> v;
                v.push_back(std::make_pair(x,y));
                _uniqueChars[skey] = v;
            }
            else
            {
                _uniqueChars[skey].push_back(std::make_pair(x,y));
            }
        }
    }

    qDebug() << "Total unique chars:" << _uniqueChars.size();

    // FIXME: descending sort... just pass a "greater" function instead of reversing the result
    std::sort(std::begin(_colorsUsed), std::end(_colorsUsed));
    std::reverse(std::begin(_colorsUsed), std::end(_colorsUsed));

    auto deb = qDebug();
    for (int i=0; i<16; i++)
         deb << "Color:" << _colorsUsed[i].second << "=" << _colorsUsed[i].first << "\n";
}

void ImportKoalaOrigWidget::toFrameBuffer()
{
    // 25 rows
    for (int y=0; y<ROWS; ++y)
    {
        // 40 cols
        for (int x=0; x<COLUMNS; ++x)
        {
            // 8 pixels Y
            for (int i=0; i<8; ++i)
            {
                quint8 byte = _koala.bitmap[(y * COLUMNS + x) * 8 + i];

                static const quint8 masks[] = {192, 48, 12, 3};
                // 4 wide-pixels X
                for (int j=0; j<4; ++j)
                {
                    quint8 colorIndex = 0;
                    // get the two bits that reprent the color
                    quint8 color = byte & masks[j];
                    color >>= 6-j*2;

                    switch (color)
                    {
                    // bitmask 00: background ($d021)
                    case 0x0:
                        colorIndex = _koala.backgroundColor;
                        break;

                    // bitmask 01: #4-7 screen ram
                    case 0x1:
                        colorIndex = _koala.screenRAM[y * COLUMNS + x] >> 4;
                        break;

                    // bitmask 10: #0-3 screen ram
                    case 0x2:
                        colorIndex = _koala.screenRAM[y * COLUMNS + x] & 0xf;
                        break;

                    // bitmask 11: color ram
                    case 0x3:
                        colorIndex = _koala.colorRAM[y * COLUMNS + x] & 0xf;
                        break;
                    default:
                        qDebug() << "ImportKoalaWidget::paintEvent Invalid color: " << color << " at x,y=" << x << y;
                        break;
                    }

                    _framebuffer[(y * 8 + i) * 160 + (x * 4 + j)] = colorIndex;
                }
            }
        }
    }

    update();
}

void ImportKoalaOrigWidget::reportResults()
{
    int validChars = 0;
    int invalidChars = 0;
    int validUniqueChars = 0;
    int invalidUniqueChars = 0;

    for (auto it=_uniqueChars.begin(); it!=_uniqueChars.end(); ++it)
    {
        bool keyIsValid = true;
        auto key = it->first;
        // key is 4 * 32 bytes long. Each element of
        // the key, is a pixel
        for (int i=0; i<(int)key.size(); i++)
        {
            // convert Hex to int
            char c = key[i] - '0';
            if (c > 9)
                c -= 7;         // 'A' - '9'

            int color = c;

            // determine whether or not the char can be drawn with current selected colors

            // c in d021/d022/d023?
            if (std::find(std::begin(_d02xColors), std::end(_d02xColors), color) != std::end(_d02xColors))
                continue;

            if (c<8)
            {
                keyIsValid = false;
                break;
            }
        }

        if (keyIsValid)
        {
            // it->second is the vector<pair<int,int>>
            validChars += it->second.size();
            validUniqueChars++;
        }
        else
        {
            // it->second is the vector<pair<int,int>>
            invalidChars += it->second.size();
            invalidUniqueChars++;
        }
    }

    qDebug() << "Valid chars: " << validChars << " Valid Unique chars: " << validUniqueChars;
    qDebug() << "Invalid chars:" << invalidChars << " Invalid Unique chars: " << invalidUniqueChars;
    qDebug() << "$d021,22,23=" << _d02xColors[0] << _d02xColors[1] << _d02xColors[2];
}

void ImportKoalaOrigWidget::strategyD02xAbove8()
{
    // the most used colors whose values are >=8 are going to be used for d021, d022 and d023
    // if can't find 3 colors, list is completed with most used colors whose value is < 8

    // colors are already sorted: use most used colors whose values is >= 8
    // values < 8 are reserved screen color

    int found = 0;
    for (auto& color: _colorsUsed)
    {
        if (color.second >= 8 && color.first > 0) {
            _d02xColors[found++] = color.second;
            if (found == 3)
                break;
        }
    }

    // make sure that 3 colors where selected.
    // if not complete the list with most used colors where color < 8
    for (int j=0,i=0; i<3-found; ++i, ++j)
    {
        if (_colorsUsed[j].second < 8 && _colorsUsed[j].first > 0)
        {
            _d02xColors[found++] = _colorsUsed[j].second;
        }
    }
}

void ImportKoalaOrigWidget::strategyD02xAny()
{
    // three most used colors are the ones to be used for d021, d022 and d023
    for (int i=0; i<3; ++i)
        _d02xColors[i] = _colorsUsed[i].second;
}


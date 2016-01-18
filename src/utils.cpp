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

#include "utils.h"

#include <QPainter>
#include <QPoint>
#include <QSize>
#include <QDebug>

#include "state.h"
#include "palette.h"

void utilsDrawChar(State* state, QPainter* painter, const QSize& pixelSize, const QPoint& offset, int x, int y, quint8 c)
{
    auto charset = state->getCharsetBuffer();
    auto charsetAttribs = state->getCharAttribs();
    auto ismc = state->shouldBeDisplayedInMulticolor2(c);

    auto chardef = &charset[c * 8];

    static const quint8 mc_masks[] = {192, 48, 12, 3};
    static const quint8 hr_masks[] = {128, 64, 32, 16, 8, 4, 2, 1};

    for (int i=0; i<8; ++i)
    {
        auto byte = chardef[i];

        int char_width = 8;
        int bit_width = 1;      /* 8 = 8 * 1 */
        const quint8* masks = &hr_masks[0];

        if (ismc)
        {
            char_width = 4;
            bit_width = 2;    /* 8 = 4 * 2 */
            masks = mc_masks;
        }

        for (int j=0; j<char_width; ++j)
        {
            quint8 colorIndex = 0;
            // get the two bits that reprent the color
            quint8 color = byte & masks[j];
            color >>= (8 - bit_width) - j * bit_width;

            switch (color)
            {
            // bitmask 00: background ($d021)
            case 0x0:
                colorIndex = state->getColorForPen(State::PEN_BACKGROUND);
                break;

            // bitmask 01: multicolor #1 ($d022)
            case 0x1:
                if (ismc)
                    colorIndex = state->getColorForPen(State::PEN_MULTICOLOR1);
                else
                {
                    if (state->getCharColorMode() == State::CHAR_COLOR_GLOBAL)
                        colorIndex = state->getColorForPen(State::PEN_FOREGROUND);
                    else
                        colorIndex = charsetAttribs[c];
                }
                break;

            // bitmask 10: multicolor #2 ($d023)
            case 0x2:
                Q_ASSERT(ismc && "error in logic");
                colorIndex = state->getColorForPen(State::PEN_MULTICOLOR2);
                break;

            // bitmask 11: color RAM
            case 0x3:
                Q_ASSERT(ismc && "error in logic");
                if (state->getCharColorMode() == State::CHAR_COLOR_GLOBAL)
                    colorIndex = state->getColorForPen(State::PEN_FOREGROUND) - 8;
                else
                    colorIndex = charsetAttribs[c] - 8;
                break;
            default:
                qDebug() << "MapWidget::paintEvent Invalid color: " << color << " at x,y=" << x << y;
                break;
            }
            painter->setBrush(Palette::getColor(colorIndex));
            painter->drawRect( (x*8 + j*bit_width) * pixelSize.width() + offset.x(),
                             (y*8 + i) * pixelSize.height() + offset.y(),
                             pixelSize.width() * bit_width,
                             pixelSize.height());
        }
    }
}

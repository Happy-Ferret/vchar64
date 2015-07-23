/****************************************************************************
Copyright 2015 Henning Bekel

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

#pragma once

#include <QObject>
#include <QLibrary>

#include "state.h"

typedef bool (*xlink_ping_t)(void);
typedef bool (*xlink_load_t)(uchar, uchar, ushort, uchar*, int);
typedef bool (*xlink_peek_t)(uchar, uchar, ushort, uchar*);
typedef bool (*xlink_poke_t)(uchar, uchar, ushort, uchar);
typedef bool (*xlink_fill_t)(uchar, uchar, ushort, uchar, uint);

class Preview : public QObject
{
    Q_OBJECT

    void updateBackgroundColor();
    void updateForegroundColor();
    void updateMulticolor1();
    void updateMulticolor2();
    void updateColorMode();
    void updateCharset();
    bool updateScreen(const QString &filename);
public:
    static Preview* getInstance();

public slots:
    // file loaded, or new project
    void fileLoaded();

    // at least one pixel changes in the tile
    void tileUpdated(int tileIndex);

    // multi-color / hires or new colors
    void colorPropertiesUpdated();

    // a color was selected
    void colorSelected();

protected:
    Preview();
    void updateColorProperties();

    QLibrary *_xlink;
    xlink_ping_t xlink_ping;
    xlink_load_t xlink_load;
    xlink_peek_t xlink_peek;
    xlink_poke_t xlink_poke;
    xlink_fill_t xlink_fill;
};

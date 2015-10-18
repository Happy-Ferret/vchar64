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

#pragma once

#include <QWidget>

#include "state.h"


class TilesetWidget : public QWidget
{
    Q_OBJECT

public:
    TilesetWidget(QWidget *parent=nullptr);


public slots:
    void onTileIndexUpdated(int selectedTileIndex);
    void onTilePropertiesUpdated();

signals:
    /**
     * @brief tileSelected when a new tile is selected. Spinbox will consume this event.
     * @param tileIndex value between 0 and tileMax
     */
    void tileSelected(int tileIndex);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;


    void paintFocus(QPainter &painter);
    void paintPixel(QPainter &painter, int width, int height, quint8* charPtr);
    void paintSelectedTile(QPainter& painter);

    int _selectedTile;
    int _columns;
    int _rows;
};


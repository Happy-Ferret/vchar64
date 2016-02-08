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

class ImportVICECharsetWidget : public QWidget
{
    Q_OBJECT

public:
    ImportVICECharsetWidget(QWidget *parent=nullptr);
    /**
     * @brief setBuffer copies a buffer of 64k which belongs to the C64 RAM
     * @param buffer the buffer
     */
    void setBuffer(quint8* buffer);
    quint8* getBuffer();

public slots:
    void multicolorToggled(bool toggled);
    void addressChanged(int offset);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    int _memoryOffset;
    bool _multicolor;
    quint8 _buffer[65536];

};


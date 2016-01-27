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

#include "stateimport.h"

#include <algorithm>
#include <cstdlib>
#include <QDebug>
#include <QtEndian>

#include "state.h"
#include "mainwindow.h"

qint64 StateImport::loadRaw(State* state, QFile& file)
{
    auto size = file.size() - file.pos();
    if (size % 8 !=0)
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Warning: file is not multiple of 8. Characters might be incomplete"));
        qDebug() << "File size not multiple of 8 (" << size << "). Characters might be incomplete";
    }

    int toRead = std::min((int)size, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsetBuffer();

    auto total = file.read((char*)state->_charset, toRead);

    Q_ASSERT(total == toRead && "Failed to read file");

    return total;
}

qint64 StateImport::loadPRG(State *state, QFile& file, quint16* outAddress)
{
    auto size = file.size();
    if (size < 10) { // 2 + 8 (at least one char)
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: File size too small"));
        qDebug() << "Error: File Size too small.";
        return -1;
    }

    // ignore first 2 bytes
    quint16 address;
    file.read((char*)&address, 2);

    if (outAddress) {
        *outAddress = qFromLittleEndian(address);
    }

    return StateImport::loadRaw(state, file);
}

qint64 StateImport::loadCTM4(State *state, QFile& file, struct CTMHeader4* v4header)
{
    // only expanded files are supported
    if (!v4header->expanded)
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: CTM is not expanded"));
        qDebug() << "CTM is not expanded. Cannot load it";
        return -1;
    }

    // only 20 bytes were read, but v4 headers has 24 bytes.
    // but the 4 remaing bytes are not important.
    char ignore[4];
    file.read(ignore, sizeof(ignore));

    int num_chars = qFromLittleEndian(v4header->num_chars);
    int toRead = std::min(num_chars * 8, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsetBuffer();

    auto total = file.read((char*)state->_charset, toRead);

    for (int i=0; i<4; i++)
        state->setColorForPen(i, v4header->colors[i]);

    state->setMulticolorMode(v4header->vic_res);

    State::TileProperties tp;
    tp.interleaved = 1;
    tp.size.setWidth(v4header->tile_width);
    tp.size.setHeight(v4header->tile_height);
    state->setTileProperties(tp);

    return total;
}

qint64 StateImport::loadCTM5(State *state, QFile& file, struct CTMHeader5* v5header)
{
    // Must be expanded, or tile system disabled:
    // flags that we don't want: flags & 0b11 = 0b01
    if ((v5header->flags & 0b00000011) == 0b00000001)
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: CTM is not expanded"));
        qDebug() << "CTM is not expanded. Cannot load it";
        return -1;
    }

    int num_chars = qFromLittleEndian(v5header->num_chars) + 1;
    int num_tiles = qFromLittleEndian(v5header->num_tiles) + 1;
    QSize map_size = QSize(qFromLittleEndian(v5header->map_width), qFromLittleEndian(v5header->map_height));
    int toRead = std::min(num_chars * 8, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsetBuffer();

    auto total = file.read((char*)state->_charset, toRead);

    for (int i=0; i<4; i++)
        state->_setColorForPen(i, v5header->colors[i], -1);

    state->_setMulticolorMode(v5header->flags & 0b00000100);

    State::TileProperties tp;
    tp.interleaved = 1;
    // some files reports size == 0. Bug in CTMv5?
    tp.size.setWidth(qMax((int)v5header->tile_width,1));
    tp.size.setHeight(qMax((int)v5header->tile_height,1));
    state->_setTileProperties(tp);


    // if color_mode is per_char, convert it to per_tile
    state->_setForegroundColorMode((State::ForegroundColorMode)!!v5header->color_mode);
    state->_setMapSize(map_size);

    // color_mode == PER CHAR ?
    if (v5header->color_mode == 2)
    {
        // place char attribs in tile attribs
        file.read((char*)state->_tileAttribs, num_chars);
        // clean the upper nibble
        for (int i=0; i<num_chars; ++i)
            state->_tileAttribs[i] &= 0x0f;
    }
    else
    {
        // if colo
        file.seek(file.pos() + num_chars);

        // color_mode == PER TILE ? or GLOBAL?
        file.read((char*)state->_tileAttribs, num_tiles);
    }

    // since it is expanded, there are no tile_data

    int mapInBytes = map_size.width() * map_size.height();
    quint16* tmpBuffer = (quint16*) malloc(mapInBytes * 2);
    // read map
    file.read((char*)tmpBuffer, mapInBytes * 2);
    for (int i=0; i<mapInBytes; i++)
    {
        // FIXME: what happens with tiles bigger than 255?
        state->_map[i] = tmpBuffer[i] & 0xff;
    }
    free(tmpBuffer);

    return total;
}

qint64 StateImport::loadCTM(State *state, QFile& file)
{
    struct CTMHeader5 header;
    auto size = file.size();
    if ((std::size_t)size<sizeof(header))
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: CTM file too small"));
        qDebug() << "Error. File size too small to be CTM (" << size << ").";
        return -1;
    }

    size = file.read((char*)&header, sizeof(header));
    if ((std::size_t)size<sizeof(header))
        return -1;

    // check header
    if (header.id[0] != 'C' || header.id[1] != 'T' || header.id[2] != 'M')
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: invalid CTM file"));
        qDebug() << "Not a valid CTM file";
        return -1;
    }

    // check version
    if (header.version == 4) {
        return loadCTM4(state, file, (struct CTMHeader4*)&header);
    } else if (header.version == 5) {
        return loadCTM5(state, file, &header);
    }

    MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: CTM version not supported"));
    qDebug() << "Invalid CTM version: " << header.version;
    return -1;
}

qint64 StateImport::loadVChar64(State *state, QFile& file)
{
    struct VChar64Header header;
    auto size = file.size();
    if ((std::size_t)size<sizeof(header))
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: Invalid VChar file"));
        qDebug() << "Error. File size too small to be VChar64 (" << size << ").";
        return -1;
    }

    size = file.read((char*)&header, sizeof(header));
    if ((std::size_t)size<sizeof(header))
        return -1;

    // check header
    if (header.id[0] != 'V' || header.id[1] != 'C' || header.id[2] != 'h' || header.id[3] != 'a' || header.id[4] != 'r')
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: Invalid VChar file"));
        qDebug() << "Not a valid VChar64 file";
        return -1;
    }

    if (header.version > 2)
    {
        MainWindow::getInstance()->setErrorMessage(QObject::tr("Error: VChar version not supported"));
        qDebug() << "VChar version not supported";
        return -1;
    }

    // common for version 1 and version

    int num_chars = qFromLittleEndian((int)header.num_chars);
    int toRead = std::min(num_chars * 8, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsetBuffer();

    auto total = file.read((char*)state->_charset, toRead);

    for (int i=0; i<4; i++)
        state->_setColorForPen(i, header.colors[i], -1);

    state->_setMulticolorMode(header.vic_res);
    State::TileProperties properties;
    properties.size = {header.tile_width, header.tile_height};
    properties.interleaved = header.char_interleaved;
    state->_setTileProperties(properties);

    // version 2 only
    if (header.version == 2)
    {
        int color_mode = header.color_mode;
        state->_setForegroundColorMode((State::ForegroundColorMode)color_mode);

        int map_width = qFromLittleEndian((int)header.map_width);
        int map_height = qFromLittleEndian((int)header.map_height);
        state->_setMapSize(QSize(map_width, map_height));

        file.read((char*)state->_tileAttribs, State::TILE_ATTRIBS_BUFFER_SIZE);
        file.read((char*)state->_map, map_width * map_height);
    }
    return total;
}

qint64 StateImport::parseVICESnapshot(QFile& file, quint8* buffer64k)
{
    struct VICESnapshotHeader header;
    struct VICESnapshoptModule module;
    struct VICESnapshoptC64Mem c64mem;

    static const char VICE_MAGIC[] = "VICE Snapshot File\032";
    static const char VICE_C64MEM[] = "C64MEM";

    auto mainwindow = MainWindow::getInstance();

    if (!file.isOpen())
        file.open(QIODevice::ReadOnly);

    auto size = file.size();
    if (size < (qint64)sizeof(VICESnapshotHeader))
    {
        mainwindow->setErrorMessage(QObject::tr("Error: VICE file too small"));
        return -1;
    }

    file.seek(0);
    size = file.read((char*)&header, sizeof(header));
    if (size != sizeof(header))
    {
        mainwindow->setErrorMessage(QObject::tr("Error: VICE header too small"));
        return -1;
    }

    if (memcmp(header.id, VICE_MAGIC, sizeof(header.id)) != 0)
    {
        mainwindow->setErrorMessage(QObject::tr("Error: Invalid VICE header Id"));
        return -1;
    }

    int offset = file.pos();
    bool found = false;

    while (1) {
        size = file.read((char*)&module, sizeof(module));
        if (size != sizeof(module))
            break;

        /* Found?  */
        if (memcmp(module.moduleName, VICE_C64MEM, sizeof(VICE_C64MEM)) == 0 &&
                module.major == 0)
        {
            found = true;
            break;
        }
        offset += qFromLittleEndian(module.lenght);
        if (!file.seek(offset))
            break;
    }

    if (found)
    {
        size = file.read((char*)&c64mem, sizeof(c64mem));
        if (size != sizeof(c64mem))
        {
            mainwindow->setErrorMessage(QObject::tr("Error: Invalid VICE C64MEM segment"));
            return -1;
        }

        memcpy(buffer64k, c64mem.ram, sizeof(c64mem.ram));
    }
    else
    {
        mainwindow->setErrorMessage(QObject::tr("Error: VICE C64MEM segment not found"));
        return -1;
    }

    return 0;
}

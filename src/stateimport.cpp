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
#include <QDebug>
#include <QtEndian>

#include "state.h"

qint64 StateImport::loadRaw(State* state, QFile& file)
{
    auto size = file.size() - file.pos();
    if (size % 8 !=0) {
        qDebug() << "File size not multiple of 8 (" << size << "). Characters might be incomplete";
    }

    int toRead = std::min((int)size, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsBuffer();

    auto total = file.read((char*)state->getCharsBuffer(), toRead);

    Q_ASSERT(total == toRead && "Failed to read file");

    return total;
}

qint64 StateImport::loadPRG(State *state, QFile& file)
{
    auto size = file.size();
    if (size < 10) { // 2 + 8 (at least one char)
        qDebug() << "Error: File Size too small.";
        return -1;
    }

    // ignore first 2 bytes
    char buf[2];
    file.read(buf,2);

    return StateImport::loadRaw(state, file);
}

qint64 StateImport::loadCTM(State *state, QFile& file)
{
    struct CTMHeader header;
    auto size = file.size();
    if (size<sizeof(header)) {
        qDebug() << "Error. File size too small to be CTM (" << size << ").";
        return -1;
    }

    size = file.read((char*)&header, sizeof(header));
    if (size<sizeof(header))
        return -1;

    int num_chars = qFromLittleEndian((int)header.num_chars);
    int toRead = std::min(num_chars * 8, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsBuffer();

    auto total = file.read((char*)state->getCharsBuffer(), toRead);

    for (int i=0; i<4; i++)
        state->setColorAtIndex(i, header.colors[i]);

    state->setMultiColor(header.vic_res);

    return total;
}

qint64 StateImport::loadVChar64(State *state, QFile& file)
{
    struct VChar64Header header;
    auto size = file.size();
    if (size<sizeof(header)) {
        qDebug() << "Error. File size too small to be VChar64 (" << size << ").";
        return -1;
    }

    size = file.read((char*)&header, sizeof(header));
    if (size<sizeof(header))
        return -1;

    int num_chars = qFromLittleEndian((int)header.num_chars);
    int toRead = std::min(num_chars * 8, State::CHAR_BUFFER_SIZE);

    // clean previous memory in case not all the chars are loaded
    state->resetCharsBuffer();

    auto total = file.read((char*)state->getCharsBuffer(), toRead);

    for (int i=0; i<4; i++)
        state->setColorAtIndex(i, header.colors[i]);

    state->setMultiColor(header.vic_res);
    State::TileProperties properties;
    properties.size = {header.tile_width, header.tile_height};
    properties.interleaved = header.char_interleaved;
    state->setTileProperties(properties);

    return total;
}

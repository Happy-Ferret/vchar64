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

#include <QObject>
#include <QDebug>

#include "commands.h"

// Paint Tile
PaintTileCommand::PaintTileCommand(State *state, int tileIndex, const QPoint& position, int pen, bool mergeable, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    Q_ASSERT(position.x()<State::MAX_TILE_WIDTH*8 && position.y()<State::MAX_TILE_HEIGHT*8 && "Invalid position");

    _state = state;
    _tileIndex = tileIndex;
    _pen = pen;
    _mergeable = mergeable;
    _points.append(position);

    setText(QObject::tr("Paint #%1").arg(_tileIndex));
}

void PaintTileCommand::undo()
{
    _state->copyTileToIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
}

void PaintTileCommand::redo()
{
    _state->copyTileFromIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));

    for (int i = 0; i < _points.size(); ++i) {
        _state->_tileSetPen(_tileIndex, _points.at(i), _pen);
    }
}

bool PaintTileCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id())
        return false;

    auto p = static_cast<const PaintTileCommand*>(other);

    if (_pen != p->_pen || _tileIndex != p->_tileIndex || !p->_mergeable)
        return false;

    _points.append(p->_points);

    return true;
}


// ClearTileCommand

ClearTileCommand::ClearTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Clear #%1").arg(_tileIndex));
}

void ClearTileCommand::undo()
{
    _state->copyTileToIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
}

void ClearTileCommand::redo()
{
    _state->copyTileFromIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
    _state->_tileClear(_tileIndex);
}

// PasteCommand

PasteCommand::PasteCommand(State* state, int charIndex, const State::CopyRange* copyRange, const quint8* charsetBuffer, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _charIndex = charIndex;
    _state = state;
    _copyRange = *copyRange;
    memcpy(_copyBuffer, charsetBuffer, sizeof(_copyBuffer));

    setText(QObject::tr("Paste #%1").arg(_charIndex));
}

void PasteCommand::undo()
{
    State::CopyRange reversedCopyRange = _copyRange;

    if (reversedCopyRange.tileProperties.interleaved == 1)
        reversedCopyRange.offset = _charIndex / (reversedCopyRange.tileProperties.size.width() * reversedCopyRange.tileProperties.size.height());
    else
        reversedCopyRange.offset = _charIndex;

    _state->_paste(_charIndex, reversedCopyRange, _origBuffer);
}

void PasteCommand::redo()
{
    memcpy(_origBuffer, _state->getCharsetBuffer(), State::CHAR_BUFFER_SIZE);
    memcpy(_origBuffer + State::CHAR_BUFFER_SIZE, _state->getTileAttribs(), State::TILE_ATTRIBS_BUFFER_SIZE);
    _state->_paste(_charIndex, _copyRange, _copyBuffer);
}

// CutCommand

CutCommand::CutCommand(State *state, int charIndex, const State::CopyRange& copyRange, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _charIndex = charIndex;
    _state = state;
    _copyRange = copyRange;
    memset(_zeroBuffer, 0, sizeof(_zeroBuffer));

    setText(QObject::tr("Cut #%1").arg(_charIndex));
}

void CutCommand::undo()
{
    State::CopyRange reversedCopyRange;
    memcpy(&reversedCopyRange, &_copyRange, sizeof(State::CopyRange));
    reversedCopyRange.offset = _charIndex;

    _state->_paste(_charIndex, reversedCopyRange, _origBuffer);
}

void CutCommand::redo()
{
    memcpy(_origBuffer, _state->getCharsetBuffer(), sizeof(_origBuffer));
    _state->_paste(_charIndex, _copyRange, _zeroBuffer);
}
// FlipTileHCommand

FlipTileHCommand::FlipTileHCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Flip Horizontally #%1").arg(_tileIndex));
}

void FlipTileHCommand::undo()
{
    _state->_tileFlipHorizontally(_tileIndex);
}

void FlipTileHCommand::redo()
{
    _state->_tileFlipHorizontally(_tileIndex);
}

// FlipTileVCommand

FlipTileVCommand::FlipTileVCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Flip Vertically #%1").arg(_tileIndex));
}

void FlipTileVCommand::undo()
{
    _state->_tileFlipVertically(_tileIndex);
}

void FlipTileVCommand::redo()
{
    _state->_tileFlipVertically(_tileIndex);
}

// RotateTileCommand

RotateTileCommand::RotateTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Rotate #%1").arg(_tileIndex));
}

void RotateTileCommand::undo()
{
    _state->_tileRotate(_tileIndex);
    _state->_tileRotate(_tileIndex);
    _state->_tileRotate(_tileIndex);
}

void RotateTileCommand::redo()
{
    _state->_tileRotate(_tileIndex);
}

// InvertTile

InvertTileCommand::InvertTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Invert #%1").arg(_tileIndex));
}

void InvertTileCommand::undo()
{
    _state->_tileInvert(_tileIndex);
}

void InvertTileCommand::redo()
{
    _state->_tileInvert(_tileIndex);
}

// Shift left

ShiftLeftTileCommand::ShiftLeftTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Shift Left #%1").arg(_tileIndex));
}

void ShiftLeftTileCommand::undo()
{
    _state->_tileShiftRight(_tileIndex);
}

void ShiftLeftTileCommand::redo()
{
    _state->_tileShiftLeft(_tileIndex);
}

// Shift Right

ShiftRightTileCommand::ShiftRightTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Shift Right #%1").arg(_tileIndex));
}

void ShiftRightTileCommand::undo()
{
    _state->_tileShiftLeft(_tileIndex);
}

void ShiftRightTileCommand::redo()
{
    _state->_tileShiftRight(_tileIndex);
}

// Shift Up

ShiftUpTileCommand::ShiftUpTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Shift Up #%1").arg(_tileIndex));
}

void ShiftUpTileCommand::undo()
{
    _state->_tileShiftDown(_tileIndex);
}

void ShiftUpTileCommand::redo()
{
    _state->_tileShiftUp(_tileIndex);
}

// Shift Down

ShiftDownTileCommand::ShiftDownTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Shift Down #%1").arg(_tileIndex));
}

void ShiftDownTileCommand::undo()
{
    _state->_tileShiftUp(_tileIndex);
}

void ShiftDownTileCommand::redo()
{
    _state->_tileShiftDown(_tileIndex);
}

// SetTilePropertiesCommand

SetTilePropertiesCommand::SetTilePropertiesCommand(State *state, const State::TileProperties& properties, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _state = state;
    _new = properties;

    setText(QObject::tr("Tile Properties %1x%2 - %3")
            .arg(properties.size.width())
            .arg(properties.size.height())
            .arg(properties.interleaved)
            );
}

void SetTilePropertiesCommand::undo()
{
    _state->_setTileProperties(_old);
}

void SetTilePropertiesCommand::redo()
{
    _old = _state->getTileProperties();
    _state->_setTileProperties(_new);
}


// SetMulticolorModeCommand

SetMulticolorModeCommand::SetMulticolorModeCommand(State *state, bool multicolorEnabled, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _state = state;
    _new = multicolorEnabled;

    if (_new)
        setText(QObject::tr("Multicolor enabled"));
    else
        setText(QObject::tr("Multicolor disabled"));
}

void SetMulticolorModeCommand::undo()
{
    _state->_setMulticolorMode(_old);
}

void SetMulticolorModeCommand::redo()
{
    _old = _state->isMulticolorMode();
    _state->_setMulticolorMode(_new);
}

// SetColorCommand

SetColorCommand::SetColorCommand(State *state, int color, int pen, int tileIdx, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _state(state)
    , _pen(pen)
    , _new(color)
    , _tileIdx(tileIdx)
{
    setText(QObject::tr("Color[%1] = %2")
            .arg(pen)
            .arg(color)
            );

}

void SetColorCommand::undo()
{
    _state->_setColorForPen(_pen, _old, _tileIdx);
}

void SetColorCommand::redo()
{
    _old = _state->getColorForPen(_pen, _tileIdx);
    _state->_setColorForPen(_pen, _new, _tileIdx);
}

// SetForegroundColorMode

SetForegroundColorMode::SetForegroundColorMode(State *state, int mode, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _state(state)
    , _mode(mode)
{
    setText(QObject::tr("Foreground Mode = %1")
            .arg(mode)
            );
}

void SetForegroundColorMode::undo()
{
    _state->_setForegroundColorMode(_oldMode);
}

void SetForegroundColorMode::redo()
{
    _oldMode = _state->getForegroundColorMode();
    _state->_setForegroundColorMode(_mode);
}

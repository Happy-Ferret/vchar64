#include <QObject>
#include <QDebug>

#include "commands.h"

// Paint Tile
PaintTileCommand::PaintTileCommand(State *state, int tileIndex, const QPoint& position, int colorIndex, bool mergeable, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    Q_ASSERT(position.x()<State::MAX_TILE_WIDTH*8 && position.y()<State::MAX_TILE_HEIGHT*8 && "Invalid position");

    _state = state;
    _tileIndex = tileIndex;
    _colorIndex = colorIndex;
    _mergeable = mergeable;
    _points.append(position);

    setText(QObject::tr("Paint #%1").arg(_tileIndex));
}

void PaintTileCommand::undo()
{
    _state->copyCharToIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
}

void PaintTileCommand::redo()
{
    _state->copyCharFromIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));

    for (int i = 0; i < _points.size(); ++i) {
        _state->tilePaint(_tileIndex, _points.at(i), _colorIndex);
    }
}

bool PaintTileCommand::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id())
        return false;

    auto p = static_cast<const PaintTileCommand*>(other);

    if (_colorIndex!=p->_colorIndex || _tileIndex!=p->_tileIndex || !p->_mergeable)
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
    _state->copyCharToIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
}

void ClearTileCommand::redo()
{
    _state->copyCharFromIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
    _state->tileClear(_tileIndex);
}

// PasteTileCommand

PasteTileCommand::PasteTileCommand(State *state, int tileIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    _tileIndex = tileIndex;
    _state = state;

    setText(QObject::tr("Paste #%1").arg(_tileIndex));
}

void PasteTileCommand::undo()
{
    _state->copyCharToIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
}

void PasteTileCommand::redo()
{
    _state->copyCharFromIndex(_tileIndex, (quint8*)&_buffer, sizeof(_buffer));
    _state->tilePaste(_tileIndex);
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
    _state->tileFlipHorizontally(_tileIndex);
}

void FlipTileHCommand::redo()
{
    _state->tileFlipHorizontally(_tileIndex);
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
    _state->tileFlipVertically(_tileIndex);
}

void FlipTileVCommand::redo()
{
    _state->tileFlipVertically(_tileIndex);
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
    _state->tileRotate(_tileIndex);
    _state->tileRotate(_tileIndex);
    _state->tileRotate(_tileIndex);
}

void RotateTileCommand::redo()
{
    _state->tileRotate(_tileIndex);
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
    _state->tileInvert(_tileIndex);
}

void InvertTileCommand::redo()
{
    _state->tileInvert(_tileIndex);
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
    _state->tileShiftRight(_tileIndex);
}

void ShiftLeftTileCommand::redo()
{
    _state->tileShiftLeft(_tileIndex);
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
    _state->tileShiftLeft(_tileIndex);
}

void ShiftRightTileCommand::redo()
{
    _state->tileShiftRight(_tileIndex);
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
    _state->tileShiftDown(_tileIndex);
}

void ShiftUpTileCommand::redo()
{
    _state->tileShiftUp(_tileIndex);
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
    _state->tileShiftUp(_tileIndex);
}

void ShiftDownTileCommand::redo()
{
    _state->tileShiftDown(_tileIndex);
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
    _state->setTileProperties(_old);
}

void SetTilePropertiesCommand::redo()
{
    _old = _state->getTileProperties();
    _state->setTileProperties(_new);
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
    _state->setMultiColor(_old);
}

void SetMulticolorModeCommand::redo()
{
    _old = _state->isMultiColor();
    _state->setMultiColor(_new);
}

// SetColorCommand

SetColorCommand::SetColorCommand(State *state, int color, int colorIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(QObject::tr("Color[%1] = %2")
            .arg(colorIndex)
            .arg(color)
            );

    _state = state;
    _colorIndex = colorIndex;
    _new = color;
}

void SetColorCommand::undo()
{
    _state->setColorAtIndex(_colorIndex, _old);
}

void SetColorCommand::redo()
{
    _old = _state->getColorAtIndex(_colorIndex);
    _state->setColorAtIndex(_colorIndex, _new);
}

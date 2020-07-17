#pragma once
#include "CellCoord.hpp"
#include "GridContainer.hpp" // todo: shouldn't have to be included

#include "Position.h"

namespace Grids
{
    class GridMap;

    class GAME_API GridObjectBase
    {
        enum class MoveState : uint8
        {
            NONE, //not in move list
            ACTIVE, //in move list
        };

    public:

        bool IsInGrid() const { return _grid != nullptr; }

        void SetCurrentCell(Grids::CellCoord cell) { _currentCell = cell; }
        void SetNewCellPosition(float x, float y, float z, float o)
        {
            _moveState = MoveState::ACTIVE;
            _newPosition.Relocate(x, y, z, o);
        }

        void ResetGridState() { _moveState = MoveState::NONE; }

        CellCoord GetCurrentCell() const { return _currentCell; }
        void ApplyNewPosition(Position& pos) { pos.Relocate(_newPosition); }
        CellCoord GetNewCellCoord() const { return CellCoord::FromPosition(_newPosition.m_positionX, _newPosition.m_positionY); }
        bool IsGridActive() const { return _moveState == MoveState::ACTIVE; }

    protected:

        ~GridObjectBase() { AssertNotGrid(); ASSERT(_moveState != MoveState::ACTIVE); }

        GridMap* _grid = nullptr;

        void AssertNotGrid() const { ASSERT(!IsInGrid()); }

    private:

        MoveState _moveState = MoveState::NONE;
        Position _newPosition;
        CellCoord _currentCell;
    };

    template<class T>
    class GridObject : 
        public GridObjectBase
    {
    public:

        void AddToGrid(GridMap& m) { AssertNotGrid(); _grid = &m; }
        void RemoveFromGrid() { ASSERT(IsInGrid()); _grid->Erase(static_cast<T*>(this)); _grid = nullptr; }

    protected:

        ~GridObject() = default;
    };
}

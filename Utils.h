#pragma once

#include <BWAPI.h>

namespace Utils
{
    class Position
    {
        private:
            const static BWAPI::Position toTileCenter;
            BWAPI::Position srcPosition;
            int operator-(BWAPI::Unit unit);
            int operator-(BWAPI::Unitset unitset);
        public:
            Position();
            Position(BWAPI::Position position);
            static Position fromLocation(BWAPI::TilePosition location);
            static Position fromUnit(BWAPI::Unit unit);
            static Position fromUnitset(BWAPI::Unitset unitset);
            int operator-(BWAPI::Position pos);
            int operator-(BWAPI::TilePosition loc);
            BWAPI::Position get();
            std::function<bool (BWAPI::Position, BWAPI::Position)>
                comparePositions();
            std::function<bool (BWAPI::TilePosition, BWAPI::TilePosition)>
                compareLocations();
            std::function<bool (BWAPI::Unit, BWAPI::Unit)> compareUnits();
            std::function<bool (BWAPI::Unitset, BWAPI::Unitset)>
                compareUnitsets();
    };

    bool isIdle(BWAPI::Unit Facility);
}

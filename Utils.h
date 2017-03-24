#ifndef UTILS_H
#define UTILS_H

#include <BWAPI.h>

namespace Utils
{
    class compareDistanceFrom
    {
        private:
            BWAPI::Position sourcePosition = BWAPI::Positions::None;
        public:
            compareDistanceFrom(BWAPI::Position position)
                { sourcePosition = position; }
            compareDistanceFrom(BWAPI::TilePosition location)
                { sourcePosition = BWAPI::Position(location); }
            compareDistanceFrom(BWAPI::Unit unit)
                { sourcePosition = unit->getPosition(); }
            compareDistanceFrom(BWAPI::Unitset units)
                { sourcePosition = units.getPosition(); }
            bool operator()(BWAPI::Position Pos1, BWAPI::Position Pos2);
            bool operator()(
                BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2);
            bool operator()(BWAPI::Unit u1, BWAPI::Unit u2);
            bool operator()(BWAPI::Unitset u1, BWAPI::Unitset u2);
    };

    bool isIdle(BWAPI::Unit Facility);
}

#endif

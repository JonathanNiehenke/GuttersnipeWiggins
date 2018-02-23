#pragma once
#include <map>
#include <BWAPI.h>
#include "EnemyPositions.h"

class Cartographer {
    private:
        EnemyPositions enemyPositions;
        std::vector<BWAPI::Position> resourcePositions;
        int attackIdx = 0;
        static std::map<int, BWAPI::Unitset> getStarcraftMappedResources();
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
        static bool isTangible(const BWAPI::UnitType unitType);
    public:
        void discoverResourcePositions();
        std::vector<BWAPI::Position> getResourcePositions() const
            { return resourcePositions; }
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        static std::vector<BWAPI::Position> getStartingPositions();
        bool lacksEnemySighting();
        void addUnit(const BWAPI::Unit& unit);
        void removeUnit(const BWAPI::Unit& unit);
        void update();
        BWAPI::Position nextPosition(const BWAPI::Position& sourcePosition);
        std::function<bool(
                const PositionalType&, const PositionalType&)>
            closerPositionalType(const BWAPI::Position& srcPos) const;
        void drawStatus();
};

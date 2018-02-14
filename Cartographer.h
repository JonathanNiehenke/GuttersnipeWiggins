#pragma once
#include <map>
#include <BWAPI.h>
#include "EnemyTracker.h"

class Cartographer {
    private:
        EnemyTracker enemyTracker;
        std::vector<BWAPI::Position> resourcePositions;
        int attackIdx = 0;
        static std::map<int, BWAPI::Unitset> getStarcraftMappedResources();
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
    public:
        void discoverResourcePositions();
        std::vector<BWAPI::Position> getResourcePositions() const
            { return resourcePositions; }
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        static std::vector<BWAPI::Position> getStartingPositions();
        bool lacksEnemySighting() const;
        void addUnit(const BWAPI::Unit& unit);
        void removeUnit(const BWAPI::Unit& unit);
        void update();
        BWAPI::Position getNextPosition(const BWAPI::Position& sourcePosition);
        BWAPI::Position getClosestEnemyPosition(
            const BWAPI::Position& sourcePosition) const;
        void drawStatus();
};

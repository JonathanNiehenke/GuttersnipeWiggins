#pragma once
#include <BWAPI.h>
#include "Utils.h"

class Cartographer
{
    private:
        typedef std::pair<BWAPI::Position, BWAPI::UnitType> PositionalType;
        std::vector<BWAPI::Position> resourcePositions;
        std::map<BWAPI::Unit, PositionalType> currentPositions;
        std::map<BWAPI::Position, BWAPI::UnitType> foggyPositions;
        static std::map<int, BWAPI::Unitset> getStarcraftMappedResources();
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
        void updateCurrentPositions();
        void updateFoggyPositions();
        static bool isInFog(const BWAPI::Position& position);
    public:
        void discoverResourcePositions();
        std::vector<BWAPI::Position> getResourcePositions() const
            { return resourcePositions; }
        void addUnit(const BWAPI::Unit& unit);
        void removeUnit(const BWAPI::Unit& unit);
        void update();
        BWAPI::Position getClosestEnemyPosition(
            const BWAPI::Position& sourcePosition) const;
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        static std::vector<BWAPI::Position> getStartingPositions();
};

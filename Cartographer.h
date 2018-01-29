#pragma once
#include <BWAPI.h>
#include "Utils.h"

class Cartographer
{
    typedef std::set<BWAPI::TilePosition> locationSet;
    private:
        std::vector<BWAPI::Position> resourcePositions;
        std::map<BWAPI::Player, locationSet> enemyLocations;
        static void groupResources(const BWAPI::Unitset &Resources,
            std::map<int, BWAPI::Unitset> &groupedResources);
        static std::map<int, BWAPI::Unitset> getStarcraftMappedResources();
    public:
        void discoverResourcePositions();
        std::vector<BWAPI::Position> getResourcePositions() const
            { return resourcePositions; }
        void addBuildingLocation(
            const BWAPI::Player&, const BWAPI::TilePosition&);
        void removeBuildingLocation(
            const BWAPI::Player&, const BWAPI::TilePosition&);
        void removeGeyserLocation(
            const BWAPI::TilePosition& buildingLocation);
        void removePlayerLocations(const BWAPI::Player& deadPlayer);
        BWAPI::TilePosition getClosestEnemyLocation(
            const BWAPI::Position&) const;
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        locationSet getStartingLocations() const;
        void cleanEnemyLocations();
        void displayStatus(int &row) const;
};

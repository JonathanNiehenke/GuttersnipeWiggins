#ifndef CARTOGRAPHER_H
#define CARTOGRAPHER_H
#include <cassert>
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
        void discoverResources();
        std::vector<BWAPI::Position> getResourcePositions()
            { return resourcePositions; }
        void addBuildingLocation(BWAPI::Player, BWAPI::TilePosition);
        void removeBuildingLocation(BWAPI::Player, BWAPI::TilePosition);
        void removeBuildingLocation(BWAPI::TilePosition buildingLocation);
        void removePlayerLocations(BWAPI::Player deadPlayer);
        BWAPI::TilePosition getClosestEnemyLocation(BWAPI::Position);
        static std::vector<BWAPI::Position> getUnexploredStartingPositions();
        locationSet getStartingLocations();
        void cleanEnemyLocations();
        void displayStatus(int &row);
};

#endif


#pragma once
#include <BWAPI.h>
#include "Utils.h"

namespace {

    class EvasionTracker {
        private:
            typedef std::pair<BWAPI::Position, BWAPI::UnitType> PositionalType;
            std::map<BWAPI::Unit, PositionalType> currentPositions;
            std::map<BWAPI::Position, BWAPI::UnitType> foggyPositions;
            void updateCurrentPositions();
            void updateFoggyPositions();
            static bool isInFog(const BWAPI::Position& position);
            void moveToFoggyPositions(
                const std::pair<BWAPI::Unit, PositionalType>& pair);
            static void draw(const PositionalType& posType);
        public:
            bool empty() const;
            void addUnit(const BWAPI::Unit& unit);
            void removeUnit(const BWAPI::Unit& unit);
            void update();
            BWAPI::Position getClosestEnemyPosition(
                const BWAPI::Position& sourcePosition) const;
    };

}

class Cartographer {
    private:
        EvasionTracker evasionTracker;
        std::vector<BWAPI::Position> resourcePositions;
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
        BWAPI::Position getClosestEnemyPosition(
            const BWAPI::Position& sourcePosition) const;
};

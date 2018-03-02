#pragma once
#include <vector>
#include <BWAPI.h>
#include "Utils.h"

typedef std::pair<BWAPI::Position, BWAPI::UnitType> PositionalType;
typedef std::pair<BWAPI::Unit, PositionalType> UnitRecord;
typedef std::function<bool(const BWAPI::UnitType&)> TypePred;

class EnemyPositions {
    private:
        std::vector<PositionalType> enemyPositions;
        std::vector<UnitRecord> visibleUnits;
        void discardVisible();
        static bool isVisible(const PositionalType& posType);
        void updateVisible();
        void updatePosition(
            PositionalType& positionaType, const BWAPI::Position& position);
    public:
        void include(const BWAPI::Unit& unit);
        void discard(const BWAPI::Unit& unit);
        void morph(const BWAPI::Unit& unit);
        void update();
        bool lacking(TypePred typePred=nullptr) const;
        BWAPI::Position closestTo(
            const BWAPI::Position& srcPos, TypePred typePred=nullptr) const;
        void drawStatus() const;
};

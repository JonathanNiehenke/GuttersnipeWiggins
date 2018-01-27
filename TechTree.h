#pragma once
#include <BWAPI.h>

class TechTree {
    private:
        typedef std::vector<BWAPI::UnitType> UTypeSeries;
        std::map<BWAPI::UnitType, UTypeSeries> techMapping;
        std::vector<BWAPI::UnitType> builtTech;
        UTypeSeries getRequiredTree(const BWAPI::UnitType& unitType);
        UTypeSeries assembleTree(const BWAPI::UnitType& unitType);
        UTypeSeries assembleImmediateTree(const BWAPI::UnitType&) const;
        void joinSubTrees(UTypeSeries& immediateTree);
        void removeBuiltTechFromTree(UTypeSeries& requiresTree) const;
        void removeFromTree(UTypeSeries&, const BWAPI::UnitType&) const;
        void removeFromAllTrees(const BWAPI::UnitType& unitType);
    public:
        BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType& unitType);
        void addTech(const BWAPI::UnitType& unitType);
};


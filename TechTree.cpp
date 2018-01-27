#pragma once
#include "TechTree.h"

BWAPI::UnitType TechTree::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType)
{
    const UTypeSeries& requiresTree = getRequiredTree(unitType);
    if (requiresTree.size())
        return requiresTree.back();
    else
        throw std::runtime_error("Unit should be unlocked");
}

TechTree::UTypeSeries TechTree::getRequiredTree(
    const BWAPI::UnitType& unitType)
{
    try {
        return techMapping.at(unitType); }
    catch (std::out_of_range) {
        UTypeSeries& requiresTree = assembleTree(unitType);
        removeBuiltTechFromTree(requiresTree);
        techMapping[unitType] = requiresTree;
        return requiresTree;
    }
}

TechTree::UTypeSeries TechTree::assembleTree(const BWAPI::UnitType& unitType) {
    UTypeSeries requiresTree = assembleImmediateTree(unitType);
    joinSubTrees(requiresTree);
    return requiresTree;
}

TechTree::UTypeSeries TechTree::assembleImmediateTree(
    const BWAPI::UnitType& unitType) const
{
    UTypeSeries immediateTree;
    for (const auto& pair: unitType.requiredUnits()) {
        if (pair.first.isBuilding())
            immediateTree.push_back(pair.first);
    }
    return immediateTree;
}

void TechTree::joinSubTrees(UTypeSeries& immediateTree) {
    for (const BWAPI::UnitType& requiredType: immediateTree) {
        UTypeSeries subTree = getRequiredTree(requiredType);
        immediateTree.insert(
            immediateTree.end(), subTree.begin(), subTree.end());
    }
}

void TechTree::addTech(const BWAPI::UnitType& techType) {
    builtTech.push_back(techType);
    removeFromAllTrees(techType);
}

void TechTree::removeBuiltTechFromTree(UTypeSeries& requiresTree) const {
    for (const BWAPI::UnitType& builtType: builtTech)
        removeFromTree(requiresTree, builtType);
}

void TechTree::removeFromTree(
    UTypeSeries& requiresTree, const BWAPI::UnitType& techType) const
{
    requiresTree.erase(
        std::remove(requiresTree.begin(), requiresTree.end(), techType),
        requiresTree.end());
}

void TechTree::removeFromAllTrees(const BWAPI::UnitType& techType) {
    for (auto& pair: techMapping) {
        removeFromTree(pair.second, techType);
    }
}

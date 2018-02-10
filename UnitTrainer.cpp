#pragma once
#include "UnitTrainer.h"

using namespace BWAPI::Filter;

void UnitTrainer::includeFacility(const BWAPI::Unit& Facility) {
    productionFacilities.push_back(Facility);
}

void UnitTrainer::removeFacility(const BWAPI::Unit& Facility) {
    auto endIt = productionFacilities.end(),
         foundIt = find(productionFacilities.begin(), endIt, Facility);
    if (foundIt != endIt)
        productionFacilities.erase(foundIt);
}

int UnitTrainer::facilityCount() const {
    return productionFacilities.size();
}

bool UnitTrainer::readyToTrain(const BWAPI::UnitType& unitType) const {
    for (const BWAPI::Unit& Facility: productionFacilities) {
        if (Facility->canTrain(unitType) && isTrainingQueueEmpty(Facility))
            return true;
    }
    return false;
}

void UnitTrainer::trainUnits(const BWAPI::UnitType& unitType) {
    for (BWAPI::Unit& Facility: productionFacilities) {
        if (isTrainingQueueEmpty(Facility))
            Facility->train(unitType);
    }
}

bool UnitTrainer::isTrainingQueueEmpty(const BWAPI::Unit& Facility) const {
    // Because getLarva() returns Unitset and getTrainingQueue() deque
    if (Facility->getType().producesLarva())
        return !Facility->getLarva().empty();
    else
        return Facility->getTrainingQueue().empty();
}

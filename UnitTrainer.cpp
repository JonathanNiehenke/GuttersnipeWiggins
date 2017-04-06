#ifndef UNITTRAINER_CPP
#define UNITTRAINER_CPP
#include "UnitTrainer.h"

using namespace BWAPI::Filter;

bool UnitTrainer::isIdle(BWAPI::Unit Facility)
{
    // Zerg hatchery is always idle so use the count of lava. Because
    // Protoss and Terran never produce larva it negates the count.
    return (Facility->isIdle() && (!Facility->getType().producesLarva() ||
            !Facility->getLarva().empty()));
}

void UnitTrainer::onStart(
    BWAPI::UnitType unitType, CmdRescuer::Rescuer *cmdRescuer)
{
    this->unitType = unitType;
    this->cmdRescuer = cmdRescuer;
}

void UnitTrainer::removeFacility(BWAPI::Unit Facility)
{
    auto endIt = productionFacilities.end(),
         foundIt = find(productionFacilities.begin(), endIt, Facility);
    if (foundIt != endIt) {
        productionFacilities.erase(foundIt);
    }
}

bool UnitTrainer::canProduce()
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility)) return true;
    }
    return false;
}

void UnitTrainer::produceSingleUnit(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility)) {
            if (!Facility->train(unitType)) {
                cmdRescuer->append(
                    CmdRescuer::TrainCommand(Facility, unitType));
            }
            break;
        }
    }
}

void UnitTrainer::produceUnits(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility)) {
            if (!Facility->train(unitType)) {
                cmdRescuer->append(
                    CmdRescuer::TrainCommand(Facility, unitType));
            }
        }
    }
}

void UnitTrainer::displayStatus(int &row)
{
    row += 10;
    BWAPI::Broodwar->drawTextScreen(3, row,
        "%d Army Facilities", productionFacilities.size());
    // row += 10;
    // BWAPI::Broodwar->drawTextScreen(3, row, "%d Facilities Producing",
        // std::count_if(productionFacilities.begin(), productionFacilities.end(),
            // &UnitTrainer::isIdle));
    row += 5;
}

#endif

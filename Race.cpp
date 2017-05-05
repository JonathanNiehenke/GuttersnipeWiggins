#ifndef RACE_CPP
#define RACE_CPP
#include "Race.h"

// To reduce the parameters list of Race and derived constructors.
Core::Core(
    BuildingConstructer *buildingConstructer,
    Cartographer *cartographer,
    CmdRescuer::Rescuer *cmdRescuer,
    EcoBaseManager *ecoBaseManager,
    SquadCommander *squadCommander,
    UnitTrainer *unitTrainer)
{
    this->buildingConstructer = buildingConstructer;
    this->cartographer = cartographer;
    this->cmdRescuer = cmdRescuer;
    this->ecoBaseManager = ecoBaseManager;
    this->squadCommander = squadCommander;
    this->unitTrainer = unitTrainer;
}


Race::Race(
    BWAPI::UnitType centerType,
    BWAPI::UnitType workerType,
    BWAPI::UnitType supplyType,
    BWAPI::UnitType armyTechType,
    BWAPI::UnitType armyUnitType,
    Core &core,
    float Ratio) : expandRatio(Ratio)
{
    this->centerType = centerType;
    this->workerType = workerType;
    this->supplyType = supplyType;
    this->armyTechType = armyTechType;
    this->armyUnitType = armyUnitType;
    this->buildingConstructer = core.buildingConstructer;
    this->cartographer = core.cartographer;
    this->cmdRescuer = core.cmdRescuer;
    this->ecoBaseManager = core.ecoBaseManager;
    this->squadCommander = core.squadCommander;
    this->unitTrainer = core.unitTrainer;
    this->Self = BWAPI::Broodwar->self();
    for (ResourceLocation resourceGroup: cartographer->getResourceGroups()) {
        drawCenterSearch(resourceGroup.getPosition());
    }
}

void Race::drawCenterSearch(BWAPI::Position resourceLocation)
{
    // Live debugging info.
    BWAPI::Broodwar->registerEvent(
        [resourceLocation](BWAPI::Game*){
            BWAPI::Broodwar->drawCircleMap(resourceLocation, 8,
                BWAPI::Color(255, 255, 255), true);  // White
            BWAPI::Broodwar->drawCircleMap(resourceLocation, 380,
                BWAPI::Color(255, 255, 255), false);  // White
        },  nullptr, -1);
}

void Race::onCenterComplete(BWAPI::Unit Unit)
{
    Utils::compareDistanceFrom centerPos(Unit);
    for (ResourceLocation resourceGroup: cartographer->getResourceGroups()) {
        if (centerPos.getDifference(resourceGroup.getPosition()) < 380) {
            ecoBaseManager->addBase(Unit, resourceGroup.getMinerals());
            return;
        }
    }
}

void Race::addWorker(BWAPI::Unit Unit)
{
    try {
        ecoBaseManager->addWorker(Unit);
    }
    catch (char* err) {
        if (ecoBaseManager->getBaseAmount()) {
            BWAPI::Broodwar->sendText(err);
        }
        else {
            onCompleteWorkaround(Unit);
        }
    }
}

int Race::getAvailableSupply()
{
    const int suppProvided = supplyType.supplyProvided(),
              centProvided = centerType.supplyProvided();
    int creatingSupply = Self->incompleteUnitCount(supplyType),
        constructingCenter = Self->incompleteUnitCount(centerType),
        supplyToBeProvided = (
            creatingSupply * suppProvided + constructingCenter * centProvided);
    return Self->supplyTotal() + supplyToBeProvided - Self->supplyUsed();
}

int Race::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Prevents repetitive conversion into a float.
    const float supplyBuildTime = float(supplyType.buildTime());
    int unitsPerSupplyBuild = int(std::ceil(
            supplyBuildTime / unitType.buildTime())),
        facilityAmount = (unitType == workerType
            ? ecoBaseManager->getBaseAmount()
            : unitTrainer->facilityCount()),
        unitSupplyUsed = unitType.supplyRequired();
    return unitsPerSupplyBuild * facilityAmount * unitSupplyUsed;
}

bool Race::needsSupply()
{
    int workerBuffer = getUnitBuffer(workerType),
        armyBuffer = getUnitBuffer(armyUnitType);
    return getAvailableSupply() <= workerBuffer + armyBuffer;
}

void Race::manageProduction()
{
    // ToDo: Max supply condition relative to game mode (Melee).
    if (needsSupply() && Self->supplyTotal() != 400)
    {
        createSupply();
    }
    else {
        try {
            createWorkers();
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
        createWarriors();
    }
}

bool Race::readyToExpand()
{
    float Ratio = (float(unitTrainer->facilityCount() + 1) /
        ecoBaseManager->getBaseAmount());
    return (!Self->incompleteUnitCount(centerType) &&
            (Ratio > expandRatio || ecoBaseManager->isAtCapacity()));
}

bool Race::readyForArmyTech()
{
    const int facilityPrice = armyTechType.mineralPrice();
    int armyBuffer = 50 * Self->completedUnitCount(armyTechType),
        mineralsToBuild = facilityPrice + armyBuffer;
    return (Self->minerals() > mineralsToBuild || !unitTrainer->isAvailable());
}

void Race::manageStructures()
{
    if (readyToExpand()) {
        createCenter();
    }
    else if (readyForArmyTech()) {
        createFacility();
    }
}

bool Race::isUnderAttack()
{
    using namespace BWAPI::Filter;
    for (BWAPI::Position Pos: cartographer->getFacilityPositions()) {
        // To keep it fair trigger when undetectable unit attacks.
        if (BWAPI::Broodwar->getClosestUnit(
                Pos, IsEnemy && (IsDetected || IsAttacking), 250))
        {
            return true;
        }
    }
    return false;
}

void Race::manageAttackGroups()
{
    // ToDo: Reduce assembleSquads when at max. Its causing lag.
    if (Self->supplyUsed() == 400 || isUnderAttack()) {
        assembleSquads();
    }
    squadCommander->uniteSquads();
    squadCommander->removeEmptySquads();
}

// Workaround cause initial workers may complete before the centerType.
// To associate a worker to an EcoBase it must first exist.
void Race::onCompleteWorkaround(BWAPI::Unit Unit)
{
    // C3480 requires the capture variable to originate within scope
    EcoBaseManager *lambdaTemp = ecoBaseManager;
    BWAPI::Broodwar->registerEvent(
        [Unit, lambdaTemp](BWAPI::Game*) {
            assert(lambdaTemp->getBaseAmount());
            lambdaTemp->addWorker(Unit);
            },
        nullptr, 2);
}

void Race::scout(std::set<BWAPI::TilePosition> scoutLocations)
{
    using namespace BWAPI::Filter;
    // ToDo: Scout with fewer workers.
    // Bug: Protoss scout consistency.
    BWAPI::Unit baseCenter = ecoBaseManager->getFirstCenter();
    BWAPI::Unitset workerUnits = baseCenter->getUnitsInRadius(900,
        IsWorker && IsOwned && (CurrentOrder != BWAPI::Orders::Move ||
        CurrentOrder != BWAPI::Orders::PlaceBuilding));
    auto workerIt = workerUnits.begin(),
         workerEndIt = workerUnits.end();
    auto scoutIt = scoutLocations.begin(),
         scoutEndIt = scoutLocations.end();
    for (; workerIt != workerEndIt && scoutIt != scoutEndIt;
           ++workerIt, ++scoutIt)
    {
        // ToDo: Gather the previous mineral instead.
        BWAPI::Unit Scout = *workerIt,
                    closestMineral = Scout->getClosestUnit(IsMineralField);
        BWAPI::Position scoutPos = BWAPI::Position((*scoutIt));
        if (!Scout->move(scoutPos)) {
            cmdRescuer->append(CmdRescuer::MoveCommand(Scout, scoutPos, true));
            cmdRescuer->append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
        else if (!Scout->gather(closestMineral, true)) {
            cmdRescuer->append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
    }
    BWAPI::Broodwar->sendTextEx(true,
        "Found %d workers for scouting", workerUnits.size());
    BWAPI::Broodwar->sendTextEx(true, "Found %d locations", scoutLocations.size());
}

void Race::displayStatus()
{
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "availableSupply: %d, Buffer: %d, pendingSupply %d ",
        getAvailableSupply(), 
        getUnitBuffer(workerType) + getUnitBuffer(armyUnitType),
        Self->incompleteUnitCount(supplyType));
    int row = 30;
    ecoBaseManager->displayStatus(row);
    unitTrainer->displayStatus(row);
    buildingConstructer->displayStatus(row);
    squadCommander->displayStatus(row);
    cartographer->displayStatus(row);
}

#endif

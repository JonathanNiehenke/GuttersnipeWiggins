#ifndef GUTTERSNIPEWIGGINS_CPP
#define GUTTERSNIPEWIGGINS_CPP
#include "GuttersnipeWiggins.h"

// ToDo: UnitTrainer.

using namespace BWAPI::Filter;

typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
typedef std::set<BWAPI::TilePosition> locationSet;

void GW::assignFields()
{
    Self = BWAPI::Broodwar->self();
    BWAPI::Race myRace = Self->getRace();
    centerType = myRace.getCenter();
    supplyType = myRace.getSupplyProvider();
    workerType = myRace.getWorker();
    baseCenter = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(Self->getStartLocation()), IsResourceDepot);
    // Used only by Zerg. Zerg no longer works.
    // TRAINING[supplyType].includeFacility(baseCenter);
    switch (myRace) {
        case BWAPI::Races::Enum::Protoss: // Enum for constant value.
            armyEnablingTechType = BWAPI::UnitTypes::Protoss_Gateway;
            armyUnitType = BWAPI::UnitTypes::Protoss_Zealot;
            break;
        case BWAPI::Races::Enum::Terran:
            armyEnablingTechType = BWAPI::UnitTypes::Terran_Barracks;
            armyUnitType = BWAPI::UnitTypes::Terran_Marine;
            break;
        case BWAPI::Races::Enum::Zerg:
            armyEnablingTechType = BWAPI::UnitTypes::Zerg_Spawning_Pool;
            armyUnitType = BWAPI::UnitTypes::Zerg_Zergling;
            break;
    }
}

void GW::onStart()
{
    BWAPI::Broodwar->enableFlag(1);
    assignFields();
    cartographer.discoverResources(BWAPI::Position(Self->getStartLocation()));
    // Workaround cause initial workers may complete before the baseCenter.
    for (BWAPI::Unitset mineralCluster: cartographer.getMinerals()) {
        BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
            mineralCluster.getPosition(), IsResourceDepot, 300);
        if (baseCenter == baseCenter) {
            ecoBaseManager.addBase(baseCenter, mineralCluster);
            break;
        }
    }
    workerBuffer = GW::getUnitBuffer(workerType);
    squadCommander.onStart(baseCenter, armyUnitType, Self, &cartographer);
    buildingConstructer.onStart(Self, baseCenter, &cmdRescuer, &cartographer);
    unitTrainer.onStart(&cmdRescuer);
    Core core(&buildingConstructer, &cartographer, &cmdRescuer,
              &ecoBaseManager, &squadCommander, &unitTrainer);
    switch (Self->getRace()) {
        case BWAPI::Races::Enum::Protoss:
            race = new ProtossRace(core);
            break;
        case BWAPI::Races::Enum::Terran:
            race = new TerranRace(core);
            break;
        case BWAPI::Races::Enum::Zerg:
            race = new ZergRace(core);
            break;
    }
}

void GW::onFrame()
{
    const int actionFrames = std::max(5, BWAPI::Broodwar->getLatency());
    GW::displayStatus(); // For debugging.
    switch(BWAPI::Broodwar->getFrameCount() % actionFrames) {
        case 0: GW::manageProduction();
            break;
        case 1: GW::manageBases();
            break;
        case 2: GW::manageAttackGroups();
            break;
        case 3: squadCommander.combatMicro();
            break;
        case 4:
            cmdRescuer.rescue();
            cartographer.cleanEnemyLocations();
            if (Self->supplyUsed() == 400) {
                squadCommander.assembleSquad();
            }
            break;
        default: break;
    }
}

void GW::onBuildingCreate(BWAPI::Unit Unit)
{
    buildingConstructer.addProduct(Unit);
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
            squadCommander.assembleSquad();
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            if (!unitTrainer.isAvailable()) {
                scout(cartographer.getStartingLocations());
            }
            unitTrainer.includeFacility(Unit);
            armyBuffer = getUnitBuffer(armyUnitType);
        // Because we expect it catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
            break;
        default: 
            BWAPI::Broodwar << "Unexpected Building created: "
                            << Unit->getType().c_str() << std::endl;
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != Self) return;  // Ignoring non-owned units
    race->onUnitCreate(Unit);
}


void GW::onBuildingMorph(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            unitTrainer.includeFacility(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            unitTrainer.includeFacility(baseCenter);
            scout(cartographer.getStartingLocations());
            break;
        default: 
                BWAPI::Broodwar << "Unexpected Building created: "
                                << Unit->getType().c_str() << std::endl;
    }
    buildingConstructer.addProduct(Unit);
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    // Zerg workers become buildings, so recalculate.
    availableSupply = GW::getAvailableSupply();
    armyBuffer = getUnitBuffer(armyUnitType);
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        race->onUnitMorph(Unit);
    }
    // Geyser structures morph back rather than getting destroyed.
    else if (Unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
            cartographer.removeBuildingLocation(Unit->getTilePosition());
    }
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        race->onUnitComplete(Unit);
    }
    // Previously: Incremented pending type.
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        race->onUnitDestroy(Unit);
    }
    else if (Unit->getType().isMineralField()) {
        try {
            ecoBaseManager.removeMineral(Unit);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
    }
    else if (Unit->getType().isBuilding()) {
        cartographer.removeBuildingLocation(
            Unit->getPlayer(), Unit->getTilePosition());
    }
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    if (Self->isEnemy(owningPlayer) && Unit->getType().isBuilding() &&
        !Unit->isFlying())
    {
        cartographer.addBuildingLocation(
            owningPlayer, Unit->getTilePosition());
    }
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
    // Perhaps I will learn something.
    BWAPI::Broodwar->sendTextEx(true, "%s is Renegade: %s.",
        Unit->getPlayer()->getName().c_str(), Unit->getType().c_str());
    if (Unit->getPlayer() != Self) return;  // Ignoring non-owned units
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
    if (text.substr(0, 7) == "getUnit") {
        int unitId = atoi(text.substr(8, 4).c_str());
        BWAPI::Unit unit = BWAPI::Broodwar->getUnit(unitId);
        if (unit) {
            BWAPI::Broodwar << "Found Unit: %d " << unitId << std::endl;
            BWAPI::Broodwar->setScreenPosition(unit->getPosition());
        }
        return;
    }
    BWAPI::Unitset selectedUnits = BWAPI::Broodwar->getSelectedUnits();
    if (text == "isStuck") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Broodwar->sendTextEx(true, "%d: %s",
                unit->getID(), unit->isStuck() ? "True" : "False");
            }
    }
    else if (text == "getPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position Pos = unit->getPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getLocation") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::TilePosition Pos = unit->getTilePosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getTargetPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), TP.x, TP.y);
        }
    }
    else if (text == "canAttack") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true,
                unit->getType().groundWeapon() != BWAPI::WeaponTypes::None
                    ? "true" : "false");
        }
    }
    else {
        BWAPI::Broodwar->sendTextEx(true, "'%s'", text.c_str());
    }
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onPlayerLeft(BWAPI::Player Player)
{
    cartographer.removePlayerLocations(Player);
}

void GW::onEnd(bool IsWinner)
{
    delete race;
}

void GW::manageProduction()
{
    if (availableSupply <= workerBuffer + armyBuffer &&
        Self->supplyTotal() != 400)
    {
        if (supplyType.isBuilding()) {
            // Constructs a pylon or supply depot.
            buildingConstructer.constructUnit(supplyType);
        }
        else {
            // Trains a overloard.
            // TRAINING[supplyType].produceSingleUnit(supplyType);
        }
    }
    else {
        try {
            ecoBaseManager.produceUnits(workerType);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
        unitTrainer.produceUnits(armyUnitType);
    }
}

void GW::manageBases()
{
    const int centerPrice = centerType.mineralPrice(),
              armyFacilityPrice = armyEnablingTechType.mineralPrice();
    if (!PENDING_UNIT_TYPE_COUNT[centerType] &&
        unitTrainer.facilityCount() >= 2 &&
        ecoBaseManager.isAtCapacity())
    {
        buildingConstructer.constructExpansion(centerType);
    }
    else if (supplyCount && (Self->minerals() > armyFacilityPrice * 1.5 ||
            !unitTrainer.isAvailable()))
    {
        // Construct multiple Gateways and Barracks.
        if (armyEnablingTechType.canProduce())
        {
            buildingConstructer.constructUnit(armyEnablingTechType);
        }
        // Instead of multiple spawning pools build hatcharies.
        else if (unitTrainer.isAvailable() ||
                PENDING_UNIT_TYPE_COUNT[armyEnablingTechType])
        {
            buildingConstructer.constructUnit(BWAPI::UnitTypes::Zerg_Hatchery);
        }
        // This is where we build the spawning pool.
        else
        {
            buildingConstructer.constructUnit(armyEnablingTechType);
        }
    }
}

void GW::manageAttackGroups()
{
    // ToDo: Change defense to around given positons.
    if (baseCenter->getClosestUnit(IsEnemy, 900))
        squadCommander.assembleSquad();  // Enemy is inside base assemble defenders.
    squadCommander.uniteSquads();
    squadCommander.removeEmptySquads();
}

int GW::getAvailableSupply()
{
    const int supply = supplyType.supplyProvided(),
              centerSupply = centerType.supplyProvided();
    int supplyConstructing = (PENDING_UNIT_TYPE_COUNT[supplyType] * supply +
        PENDING_UNIT_TYPE_COUNT[centerType] * centerSupply);
    return Self->supplyTotal() + supplyConstructing - Self->supplyUsed();
}

int GW::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Preventing repetitive conversion into a float by using a float.
    const float supplyBuildTime = supplyType.buildTime();
    int unitSupply = unitType.supplyRequired(),
        unitBuildTime = unitType.buildTime(),
        facilityAmount = (unitType == workerType
            ? ecoBaseManager.getBaseAmount()
            : unitTrainer.facilityCount()),
        unitsDuringBuild = facilityAmount * std::ceil(
            supplyBuildTime / unitBuildTime);
    return unitsDuringBuild * unitSupply;
}

void GW::scout(std::set<BWAPI::TilePosition> scoutLocations)
{
    // ToDo: Scout with fewer workers.
    // Bug: Won't scout usually Protoss.
    BWAPI::Unitset workerUnits = baseCenter->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    auto workerIt = workerUnits.begin();
    for (BWAPI::TilePosition Location: scoutLocations) {
        // ToDo: Gather the previous mineral instead.
        BWAPI::Unit Scout = *workerIt++,
                    closestMineral = Scout->getClosestUnit(IsMineralField);
        if (!Scout->move(BWAPI::Position(Location))) {
            cmdRescuer.append(CmdRescuer::MoveCommand(
                Scout, BWAPI::Position(Location), true));
        }
        else if (!Scout->gather(closestMineral, true)) {
            cmdRescuer.append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
    }
}

void GW::displayUnitInfo()
{
    int row = 15;
    for (BWAPI::Unit unit: BWAPI::Broodwar->getSelectedUnits()) {
        int sinceCommandFrame = (BWAPI::Broodwar->getFrameCount() -
            unit->getLastCommandFrame());
        BWAPI::UnitCommand lastCmd = unit->getLastCommand();
        BWAPI::Broodwar->drawTextScreen(440, row,
            "%s: %d - %s", unit->getType().c_str(), unit->getID(),
            unit->getOrder().c_str());
        BWAPI::Broodwar->drawTextScreen(440, row + 10,
            "    %d - %s", sinceCommandFrame,
            lastCmd.getType().c_str());
        BWAPI::Unit targetedUnit = unit->getTarget();
        if (targetedUnit) {
            BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                targetedUnit->getPosition(), BWAPI::Color(0, 255, 0));
        }
        if (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit) {
            BWAPI::Broodwar->registerEvent(
                [unit, lastCmd](BWAPI::Game*)
                    {
                        BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                            lastCmd.getTarget()->getPosition(),
                            BWAPI::Color(255, 0, 0));
                    },
                nullptr, 1);
        }
        row += 25;
    }
}

void GW::displayStatus()
{
    // May perhaps reveal logic errors and bugs.
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "availableSupply: %d, Buffer: %d, pendingSupply %d ", availableSupply,
        workerBuffer + armyBuffer, PENDING_UNIT_TYPE_COUNT[supplyType]);
    int row = 30;
    ecoBaseManager.displayStatus(row);
    unitTrainer.displayStatus(row);
    buildingConstructer.displayStatus(row);
    squadCommander.displayStatus(row);
    cartographer.displayStatus(row);
    displayUnitInfo();
}

#endif

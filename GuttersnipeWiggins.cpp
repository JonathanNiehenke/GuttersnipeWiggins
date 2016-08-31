/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::vector<BWAPI::TilePosition> locationVector;
typedef std::set<BWAPI::TilePosition> locationSet;

class compareDistanceFrom
{
    private:
        BWAPI::Position sourcePosition = BWAPI::Positions::None;
    public:
        compareDistanceFrom(BWAPI::Position position)
            { sourcePosition = position; }
        compareDistanceFrom(BWAPI::TilePosition location)
            { sourcePosition = BWAPI::Position(location); }
        compareDistanceFrom(BWAPI::Unit unit)
            { sourcePosition = unit->getPosition(); }
        bool operator()(BWAPI::Position Pos1, BWAPI::Position Pos2);
        bool operator()(BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2);
        bool operator()(BWAPI::Unit unitV1, BWAPI::Unit unitV2);
};

bool compareDistanceFrom::operator()(BWAPI::Position Pos1, BWAPI::Position Pos2)
{
    return (sourcePosition.getApproxDistance(Pos1) <
            sourcePosition.getApproxDistance(Pos2));
}

bool compareDistanceFrom::operator()(
        BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2)
{
    return (sourcePosition.getApproxDistance(BWAPI::Position(tPos1)) <
            sourcePosition.getApproxDistance(BWAPI::Position(tPos2)));
}

bool compareDistanceFrom::operator()(BWAPI::Unit unit1, BWAPI::Unit unit2)
{
    return (sourcePosition.getApproxDistance(unit1->getPosition()) <
            sourcePosition.getApproxDistance(unit2->getPosition()));
}


class UnitTraining
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
    public:
        void includeFacility(BWAPI::Unit Facility)
            {productionFacilities.push_back(Facility); }
        void removeFacility(BWAPI::Unit Facility);
        bool isAvailable()
            { return !productionFacilities.empty(); }
        int facilityCount()
            { return productionFacilities.size(); }
        bool isIdle(BWAPI::Unit Facility);
        bool canProduce();
        void produceSingleUnit(BWAPI::UnitType unitType);
        void produceUnits(BWAPI::UnitType unitType);
};

void UnitTraining::removeFacility(BWAPI::Unit Facility)
{
    productionFacilities.erase(find(productionFacilities.begin(),
                                    productionFacilities.end(), Facility));
}

bool UnitTraining::isIdle(BWAPI::Unit Facility)
{
    // Zerg hatchery is always idle so determine with larva.
    // Negate larva for Protoss and Terran with producesLarva.
    return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
            Facility->getLarva().empty()));
}

bool UnitTraining::canProduce()
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility))
            return true;
    }
    return false;
}

void UnitTraining::produceSingleUnit(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility)) {
            Facility->train(unitType);
            break;
        }
    }
}

void UnitTraining::produceUnits(BWAPI::UnitType unitType)
{
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isIdle(Facility))
            Facility->train(unitType);
    }
}


int AVAILABLE_SUPPLY = 0, WORKER_BUFFER = 0, ARMY_BUFFER = 0;
BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER,  // The primary/initial base building.
            SUPPLY_UNIT = nullptr; // Required for Protoss construction.
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE, SUPPLY_TYPE, ARMY_ENABLING_TECH_TYPE,
                ARMY_UNIT_TYPE;
locationVector MINERAL_LOCATIONS;
// Indicates number already in construction/training for UnitType.
std::map<BWAPI::UnitType, short> PENDING_UNIT_TYPE_COUNT;
locationSet SCOUT_LOCATIONS;
locationVector CLUSTER_LOCATIONS;
std::map<BWAPI::Player, locationSet> ENEMY_LOCATIONS;
std::map<BWAPI::UnitType, UnitTraining> TRAINING;

int getNumQueued(BWAPI::Unit trainingFacility)
{
    const bool isZerg = SELF->getRace() == BWAPI::Races::Zerg;
    // Zerg has parallel queue of 3, as opposed to serial queue of 5.
    return isZerg ? 3 - trainingFacility->getLarva().size()
                  : trainingFacility->getTrainingQueue().size();
}

void GW::onStart()
{
    using namespace BWAPI;
    SELF = Broodwar->self();
    TilePosition startLocation = SELF->getStartLocation();
    BASE_CENTER = Broodwar->getClosestUnit(
        Position(startLocation), IsResourceDepot);
    Race myRace = SELF->getRace();
    CENTER_TYPE = myRace.getCenter();
    SUPPLY_TYPE = myRace.getSupplyProvider();
    WORKER_TYPE = myRace.getWorker();
    TRAINING[WORKER_TYPE].includeFacility(BASE_CENTER);
    // Used only by Zerg.
    TRAINING[SUPPLY_TYPE].includeFacility(BASE_CENTER);
    WORKER_BUFFER = GW::getUnitBuffer(WORKER_TYPE);
    switch (myRace) {
        case Races::Enum::Protoss: // Enum for constant value.
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Protoss_Gateway;
            ARMY_UNIT_TYPE = UnitTypes::Protoss_Zealot;
            break;
        case Races::Enum::Terran:
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Terran_Barracks;
            ARMY_UNIT_TYPE = UnitTypes::Terran_Marine;
            break;
        case Races::Enum::Zerg:
            ARMY_ENABLING_TECH_TYPE = UnitTypes::Zerg_Spawning_Pool;
            ARMY_UNIT_TYPE = UnitTypes::Zerg_Zergling;
            break;
    }
    SCOUT_LOCATIONS = GW::collectScoutingLocations();
    MINERAL_LOCATIONS = GW::getMineralClusterLocations();
    std::sort(MINERAL_LOCATIONS.begin(), MINERAL_LOCATIONS.end(),
        compareDistanceFrom(startLocation));
}

void GW::onFrame()
{
    const int latency = BWAPI::Broodwar->getLatency();
    const int centerPrice = CENTER_TYPE.mineralPrice(),
              armyFacilityPrice = ARMY_ENABLING_TECH_TYPE.mineralPrice();
    GW::displayState(); // For debugging.
    if (BWAPI::Broodwar->getFrameCount() % latency)
        return;
    if (AVAILABLE_SUPPLY <= WORKER_BUFFER + ARMY_BUFFER) {
        // Pylon and supply depot are buildings, unlike overlords.
        if (SUPPLY_TYPE.isBuilding()) 
            GW::constructUnit(SUPPLY_TYPE);
        else
            TRAINING[SUPPLY_TYPE].produceSingleUnit(SUPPLY_TYPE);
    }
    else if (TRAINING[WORKER_TYPE].canProduce()) {
        TRAINING[WORKER_TYPE].produceUnits(WORKER_TYPE);
    }
    else if (TRAINING[ARMY_UNIT_TYPE].canProduce()) {
        TRAINING[ARMY_UNIT_TYPE].produceUnits(ARMY_UNIT_TYPE);
    }
    // else if (SELF->minerals() >= centerPrice - 48) {
        // GW::constructExpansion();
    // }
    else if (SUPPLY_UNIT && (SELF->minerals() > armyFacilityPrice * 2 ||
            !TRAINING[ARMY_UNIT_TYPE].isAvailable())) {
        // Allows multiple Gateways and Barracks, unlike spawning pool.
        if (ARMY_ENABLING_TECH_TYPE.canProduce())
            GW::constructUnit(ARMY_ENABLING_TECH_TYPE);
        // Build hatcharies in-place of multiple spawning pools.
        else if (TRAINING[ARMY_UNIT_TYPE].isAvailable() ||
                PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE])
            GW::constructUnit(BWAPI::UnitTypes::Zerg_Hatchery);
        // Build a single spawning pool after all the filtering.
        else
            GW::constructUnit(ARMY_ENABLING_TECH_TYPE);
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == SUPPLY_TYPE && TRAINING[ARMY_UNIT_TYPE].isAvailable()) {
        GW::attack_from(BASE_CENTER->getPosition());
    }
    else if (unitType == ARMY_ENABLING_TECH_TYPE) {
        if (!TRAINING[ARMY_UNIT_TYPE].isAvailable())
            GW::scout();
        TRAINING[ARMY_UNIT_TYPE].includeFacility(Unit);
        ARMY_BUFFER = getUnitBuffer(ARMY_UNIT_TYPE);
    }
    // BWAPI::Broodwar->sendTextEx(true, "%s: %d created.",
        // Unit->getType().c_str(), Unit->getID());
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    // Always after change to pending count.
    AVAILABLE_SUPPLY = GW::getAvailableSupply();
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF) {
        // Because geyer structures are never destroyed.
        if (Unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
            BWAPI::Broodwar->sendTextEx(true, "Removing (%d, %d)",
                Unit->getTilePosition().x, Unit->getTilePosition().y);
            GW::removeLocation(Unit->getTilePosition());
            GW::attack_from(Unit->getPosition());
        }
        return;  // Ignoring non-owned units.
    }
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == BWAPI::UnitTypes::Zerg_Egg) {
        BWAPI::UnitType insideEggType = Unit->getBuildType();
        if (insideEggType == SUPPLY_TYPE &&
                TRAINING[ARMY_UNIT_TYPE].isAvailable())
            GW::attack_from(BASE_CENTER->getPosition());
        // BWAPI::Broodwar->sendTextEx(true, "Morphing: %s.",
            // insideEggType.c_str());
        PENDING_UNIT_TYPE_COUNT[insideEggType]++;
        // Always after change to pending count.
        AVAILABLE_SUPPLY = GW::getAvailableSupply();
    }
    else if (unitType.isBuilding()) {
        PENDING_UNIT_TYPE_COUNT[unitType]++;
        // Always after change to pending count.
        AVAILABLE_SUPPLY = GW::getAvailableSupply();
        if (unitType == BWAPI::UnitTypes::Zerg_Hatchery) {
            TRAINING[SUPPLY_TYPE].includeFacility(Unit);
            TRAINING[ARMY_UNIT_TYPE].includeFacility(Unit);
        }
        else if (unitType == BWAPI::UnitTypes::Zerg_Spawning_Pool) {
            TRAINING[ARMY_UNIT_TYPE].includeFacility(BASE_CENTER);
            GW::scout();
        }
        ARMY_BUFFER = getUnitBuffer(ARMY_UNIT_TYPE);
        // BWAPI::Broodwar->sendTextEx(true, "Morphing building: %s.",
            // unitType.c_str());
    }
    else {
        // BWAPI::Broodwar->sendTextEx(true, "Exceptional morph: %s.",
            // unitType.c_str());
    }
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE) {
        Unit->gather(Unit->getClosestUnit(IsMineralField));
    }
    else if (unitType == SUPPLY_TYPE) {
        SUPPLY_UNIT = Unit;
    }
    // else {
        // BWAPI::Broodwar->sendTextEx(true, "Exceptional completion: %s.",
            // unitType.c_str());
    // }
    PENDING_UNIT_TYPE_COUNT[unitType]--;
    // Always after change to pending count.
    AVAILABLE_SUPPLY = GW::getAvailableSupply();
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    BWAPI::UnitType unitType = Unit->getType();
    if (owningPlayer == SELF) {
        if (unitType == BWAPI::UnitTypes::Zerg_Spawning_Pool ||
                Unit == BASE_CENTER) {
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
        }
        else if (unitType == ARMY_ENABLING_TECH_TYPE) {
            TRAINING[ARMY_UNIT_TYPE].removeFacility(Unit);
        }
        else if (unitType.producesLarva()){
            TRAINING[ARMY_UNIT_TYPE].removeFacility(Unit);
        }
    }
    else if (unitType.isBuilding()) {
        GW::removeLocation(owningPlayer, Unit->getTilePosition());
        GW::attack_from(Unit->getPosition());
        // BWAPI::Broodwar->sendTextEx(true, "Enemy %s destroyed at (%d, %d).",
            // Unit->getType().c_str());
    }
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    if (owningPlayer == SELF)
        return;  // Otherwise imprecisely repeats Create/Morph/Complete.
    if (Unit->getType().isBuilding() &&
            ENEMY_LOCATIONS.find(owningPlayer) != ENEMY_LOCATIONS.end())
    {
        ENEMY_LOCATIONS[owningPlayer].insert(Unit->getTilePosition());
        // BWAPI::Broodwar->sendTextEx(true, "Enemy %s discovered.",
            // Unit->getType().c_str());
    }
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;  // Ignoring non-owned units.
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onPlayerLeft(BWAPI::Player Player)
{
    if (ENEMY_LOCATIONS.find(Player) != ENEMY_LOCATIONS.end()) {
        ENEMY_LOCATIONS.erase(Player);
    }
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onEnd(bool IsWinner)
{
}

locationSet GW::collectScoutingLocations()
{
    typedef BWAPI::TilePosition tilePos;
    tilePos myStart = SELF->getStartLocation();
    locationSet Locations;
    for (BWAPI::Player Enemy:BWAPI::Broodwar->enemies()) {
        ENEMY_LOCATIONS[Enemy];
        tilePos enemyPosition = Enemy->getStartLocation();
        if (enemyPosition != BWAPI::TilePositions::None &&
                enemyPosition != BWAPI::TilePositions::Unknown)
            Locations.insert(enemyPosition);
    }
    if (Locations.empty()) {
        // ? How to convert stl types to existing container?
        for (tilePos Start:BWAPI::Broodwar->getStartLocations()) {
            if (Start != myStart)
                Locations.insert(Start);
        }
    }
    return Locations;
}

locationVector GW::getMineralClusterLocations()
{
    // Group static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    // Collect the location of each of the minerals groups.
    locationVector clusterLocations;
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters used as destructible terrain.
        if (mineralCluster.size() > 4) {
            clusterLocations.push_back(
                BWAPI::TilePosition(mineralCluster.getPosition()));
        }
    }
    return clusterLocations;
}

int GW::getAvailableSupply()
{
    const int supply = SUPPLY_TYPE.supplyProvided();
    int supplyConstructing = PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE] * supply;
    return SELF->supplyTotal() + supplyConstructing - SELF->supplyUsed();
}

int GW::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Preventing repedative convertion into a float by using a float.
    const float supplyBuildTime = SUPPLY_TYPE.buildTime();
    int unitSupply = unitType.supplyRequired(),
        unitBuildTime = unitType.buildTime(),
        unitsDuringBuild = TRAINING[unitType].facilityCount() * std::ceil(
            supplyBuildTime / unitBuildTime);
    return unitsDuringBuild * unitSupply;
}

void GW::scout()
{
    BWAPI::Unitset workerUnits = BASE_CENTER->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    for (BWAPI::TilePosition scoutLocation: SCOUT_LOCATIONS) {
        BWAPI::Unit Scout = *workerUnits.begin();
        Scout->move(BWAPI::Position(scoutLocation));
        Scout->gather(Scout->getClosestUnit(IsMineralField), true);
        // BWAPI::Broodwar->sendTextEx(true, "%d SCOUTING.", Scout->getID());
        // workerUnits.erase(Scout);
    }
}

void GW::attack_from(BWAPI::Position Position)
{
    BWAPI::Unitset Attackers = BWAPI::Broodwar->getUnitsInRadius(
        Position, 900, GetType == ARMY_UNIT_TYPE);
    for (auto mapPair: ENEMY_LOCATIONS) {
        locationSet enemyLocations = mapPair.second;
        if (!enemyLocations.empty() && !Attackers.empty()) {
            BWAPI::TilePosition attackLocation = *enemyLocations.begin();
            BWAPI::Broodwar->sendTextEx(true, "attacking location %d, %d.",
                attackLocation.x, attackLocation.y);
            Attackers.attack(BWAPI::Position(attackLocation));
            break;
        }
    }
}

void GW::removeLocation(BWAPI::TilePosition Location)
{
    for (auto &playerLocations: ENEMY_LOCATIONS)
        playerLocations.second.erase(Location);
}

void GW::removeLocation(BWAPI::Player Player, BWAPI::TilePosition Location)
{
    if (ENEMY_LOCATIONS.find(Player) != ENEMY_LOCATIONS.end())
        ENEMY_LOCATIONS[Player].erase(Location);
}

int GW::getContractorTask(BWAPI::Unit contractorUnit)
{
    enum {Other, Position, Build};
    int Task;
    BWAPI::Order currentOrder = (contractorUnit
        ? contractorUnit->getOrder() : BWAPI::Orders::None);
    switch (currentOrder) {
        case BWAPI::Orders::Enum::PlaceBuilding:
        case BWAPI::Orders::Enum::ResetCollision:
            Task = Build;
            break;
        case BWAPI::Orders::Enum::Move:
        case BWAPI::Orders::Enum::Guard:  // A short wait.
        case BWAPI::Orders::Enum::PlayerGuard:  // A long wait.
            Task = Position;
            break;
        default:
            BWAPI::Broodwar->sendTextEx(true, "Current order %s, Minerals %d.",
                currentOrder.c_str(), SELF->minerals());
            Task = Other;
    }
    return Task;
}

void GW::constructUnit(BWAPI::UnitType constructableType)
{
    const BWAPI::Order
        Position = BWAPI::Orders::Move,
        Construct = BWAPI::Orders::PlaceBuilding,
        // When unit completed previous order.
        shortWait = BWAPI::Orders::Guard,
        longWait = BWAPI::Orders::PlayerGuard,
        // When worker occupies the same space as another unit.
        Ignored = BWAPI::Orders::ResetCollision;
    static BWAPI::Unit lastContractorUnit = nullptr;
    static BWAPI::TilePosition lastBuildLocation = BWAPI::TilePositions::None;
    BWAPI::Order recentOrder = lastContractorUnit
        ? lastContractorUnit->getOrder() : BWAPI::Orders::None;
    // Prevents reissuing the order to construct if already commanded
    // or current minerals below building's preparation range.
    if (!(recentOrder == Construct || recentOrder == Ignored) &&
            SELF->minerals() >= constructableType.mineralPrice() - 16) {
        BWAPI::Unit contractorUnit = nullptr;
        BWAPI::TilePosition constructionLocation = BWAPI::TilePositions::None;
        if (recentOrder == Position || recentOrder == shortWait ||
                recentOrder == longWait) {
            // lastContractorUnit was recently ordered into position.
            contractorUnit = lastContractorUnit;
            constructionLocation = lastBuildLocation;
        }
        else {
            contractorUnit = BASE_CENTER->getClosestUnit(
                IsWorker && !IsCarryingMinerals && IsOwned && !IsConstructing);
            constructionLocation = BWAPI::Broodwar->getBuildLocation(
                constructableType, contractorUnit->getTilePosition());
        }
        if (contractorUnit && contractorUnit->canBuild(
                constructableType, constructionLocation)) {
            contractorUnit->build(constructableType, constructionLocation);
            // Queues command to return to minerals. !Working for Zerg.
            contractorUnit->gather(BWAPI::Broodwar->getClosestUnit(
                BWAPI::Position(constructionLocation), IsMineralField), true);
            lastContractorUnit = contractorUnit;
        }
        // Prevent reissuing unit to reposition.
        else if (contractorUnit && (recentOrder != Position ||
                recentOrder != shortWait || recentOrder != longWait))
        {
            contractorUnit->move(BWAPI::Position(constructionLocation));
            lastContractorUnit = contractorUnit;
            lastBuildLocation = constructionLocation;
        }
    }
}

void GW::constructUnit(BWAPI::UnitType constructableType,
    BWAPI::TilePosition constructionLocation, BWAPI::Unit contractorUnit)
{
    enum {Position = 1, Build};
    switch (GW::getContractorTask(contractorUnit)) {
        case Position:
            if (contractorUnit->canBuild(
                    constructableType, constructionLocation))
            {
                contractorUnit->build(constructableType, constructionLocation);
                // Queues command to return to minerals. !Working for Zerg.
                contractorUnit->gather(
                    BWAPI::Broodwar->getClosestUnit(
                        BWAPI::Position(constructionLocation), IsMineralField),
                    true);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}

BWAPI::TilePosition GW::getExpansionLocation(BWAPI::Unit centerContractor)
{
    BWAPI::TilePosition expansionLocation = BWAPI::TilePositions::Invalid;
    for (auto baseLocation: MINERAL_LOCATIONS) {
        BWAPI::Position basePosition = BWAPI::Position(baseLocation);
        if (centerContractor && centerContractor->hasPath(basePosition) &&
            !BWAPI::Broodwar->isExplored(baseLocation))
        {
            expansionLocation = BWAPI::Broodwar->getBuildLocation(
                CENTER_TYPE, baseLocation);
            break;
        }
    }
    return expansionLocation;
}

void GW::constructExpansion()
{
    enum {Position = 1, Build};
    static BWAPI::Unit centerContractor = nullptr;
    static BWAPI::TilePosition expansionLocation = (
        BWAPI::TilePositions::Invalid);
    switch (GW::getContractorTask(centerContractor)) {
        case Position:
            bool isVisible;  // Prevents error C2360.
            isVisible = BWAPI::Broodwar->isVisible(expansionLocation);
            if (isVisible && BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(expansionLocation), IsResourceDepot, 300))
            {
                expansionLocation = GW::getExpansionLocation(centerContractor);
                centerContractor->move(BWAPI::Position(expansionLocation));
            }
            else if (isVisible) {
                GW::constructUnit(
                    CENTER_TYPE, expansionLocation, centerContractor);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            centerContractor = BASE_CENTER->getClosestUnit(IsWorker);
            expansionLocation = GW::getExpansionLocation(centerContractor);
            if (expansionLocation != BWAPI::TilePositions::Invalid)
                centerContractor->move(BWAPI::Position(expansionLocation));
    }
}

void GW::displayState()
{
    // May perhaps reveal logic errors and bugs.
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    BWAPI::Broodwar->drawTextScreen(3, 13,
        "AVAILABLE_SUPPLY: %d, BUFFER: %d, pendingSupply %d ", AVAILABLE_SUPPLY,
        WORKER_BUFFER + ARMY_BUFFER, PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 23,
        "armyFacilites %d, pendingArmyBuild: %d",
        TRAINING[ARMY_UNIT_TYPE].facilityCount(),
        PENDING_UNIT_TYPE_COUNT[ARMY_ENABLING_TECH_TYPE]);
    BWAPI::Broodwar->drawTextScreen(3, 33,
        "workerFacilities %d, BASE_CENTER Queue: %d.",
        TRAINING[WORKER_TYPE].facilityCount(), getNumQueued(BASE_CENTER));
    BWAPI::Broodwar->drawTextScreen(3, 43, "SCOUT_LOCATIONS: %d",
        SCOUT_LOCATIONS.size());
    int screenPosition = 63;
    for (auto playerLocations: ENEMY_LOCATIONS) {
        BWAPI::Player Player = playerLocations.first;
        locationSet Locations = playerLocations.second;
        BWAPI::Broodwar->drawTextScreen(3, screenPosition,
            "%s: %s, locationCount %d", Player->getName().c_str(),
            Player->getRace().c_str(), Locations.size());
        for (auto Location: Locations) {
            screenPosition += 10;
            BWAPI::Broodwar->drawTextScreen(3, screenPosition,
                "(%d, %d)", Location.x, Location.y);
        }
        screenPosition += 15;
    }
}


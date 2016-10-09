/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
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
        compareDistanceFrom(BWAPI::Unitset units)
            { sourcePosition = units.getPosition(); }
        bool operator()(BWAPI::Position Pos1, BWAPI::Position Pos2);
        bool operator()(BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2);
        bool operator()(BWAPI::Unit u1, BWAPI::Unit u2);
        bool operator()(BWAPI::Unitset u1, BWAPI::Unitset u2);
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

bool compareDistanceFrom::operator()(BWAPI::Unit u1, BWAPI::Unit u2)
{
    return (sourcePosition.getApproxDistance(u1->getPosition()) <
            sourcePosition.getApproxDistance(u2->getPosition()));
}

bool compareDistanceFrom::operator()(BWAPI::Unitset u1, BWAPI::Unitset u2)
{
    return (sourcePosition.getApproxDistance(u1.getPosition()) <
            sourcePosition.getApproxDistance(u2.getPosition()));
}


class EcoBase
{
    typedef std::vector<BWAPI::Unit> UnitVector;
    private:
        BWAPI::Unit Center = nullptr;
        UnitVector Minerals, Miners;
        int mineralIndex = 0;
        void removeUnit(BWAPI::Unit unit, UnitVector &unitGroup);
        int EcoBase::countAbandonedMinerals();
        int EcoBase::countPatientMiners();
    public:
        EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster);
        BWAPI::Unit getCenter()
            { return Center; }
        int getMineralCount()
            { return Minerals.size(); }
        int getMinerCount()
            { return Miners.size(); }
        void assignMiner(BWAPI::Unit minerUnit);
        void releaseMiner(BWAPI::Unit minerUnit);
        void removeMineral(BWAPI::Unit Mineral);
        bool EcoBase::isLackingMiners();
};

EcoBase::EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster)
{
    Center = center;
    Minerals.assign(mineralCluster.begin(), mineralCluster.end());
    std::sort(Minerals.begin(), Minerals.end(), compareDistanceFrom(center));
}

void EcoBase::removeUnit(BWAPI::Unit unit, UnitVector &unitGroup)
{
    auto itEnd = unitGroup.end(),
         itFound = find(unitGroup.begin(), itEnd, unit);
    if (itFound != itEnd)
        unitGroup.erase(itFound);
}

int EcoBase::countAbandonedMinerals()
{
    return std::count_if(Minerals.begin(), Minerals.end(),
        [](BWAPI::Unit Mineral){ return !Mineral->isBeingGathered(); });
}

int EcoBase::countPatientMiners()
{
    const auto patience = BWAPI::Orders::WaitForMinerals;
    return std::count_if(Miners.begin(), Miners.end(),
        [patience](BWAPI::Unit Miner){ return Miner->getOrder() == patience; });
}

void EcoBase::assignMiner(BWAPI::Unit minerUnit)
{
    Miners.push_back(minerUnit);
    minerUnit->gather(Minerals[mineralIndex++ % Minerals.size()]);
}

void EcoBase::releaseMiner(BWAPI::Unit minerUnit)
{
    removeUnit(minerUnit, Miners);
    minerUnit->stop();
}

void EcoBase::removeMineral(BWAPI::Unit mineralUnit)
{
    removeUnit(mineralUnit, Minerals);
}

bool EcoBase::isLackingMiners()
{
    int abandonedMinerals = countAbandonedMinerals();
    // Cause the recently completed worker is not counted.
    return abandonedMinerals && abandonedMinerals > countPatientMiners() + 1;
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


namespace Production
{
    
    bool isIdle(BWAPI::Unit Facility)
    {
        // Zerg hatchery is always idle so determine with larva.
        // Negate larva for Protoss and Terran with producesLarva.
        return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
                Facility->getLarva().empty()));
    }

    bool canProduce(std::vector<EcoBase*> EcoBases)
    {
        for (EcoBase *Base: EcoBases) {
            if (Base->isLackingMiners() && isIdle(Base->getCenter()))
                return true;
        }
        return false;
    }

    // For overlords.
    void produceSingleUnit(std::vector<EcoBase*> EcoBases,
        BWAPI::UnitType unitType)
    {
        for (EcoBase *Base: EcoBases) {
            BWAPI::Unit Center = Base->getCenter();
            if (isIdle(Center)) {
                Center->train(unitType);
                break;
            }
        }
    }

    void produceUnits(std::vector<EcoBase*> EcoBases, BWAPI::UnitType unitType)
    {
        for (EcoBase *Base: EcoBases) {
            BWAPI::Unit Center = Base->getCenter();
            if (Base->isLackingMiners() && isIdle(Center))
                Center->train(unitType);
        }
    }

}


int AVAILABLE_SUPPLY = 0, WORKER_BUFFER = 0, ARMY_BUFFER = 0;
BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER,  // The primary/initial base building.
            SUPPLY_UNIT = nullptr; // Required for Protoss construction.
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE, SUPPLY_TYPE, ARMY_ENABLING_TECH_TYPE,
                ARMY_UNIT_TYPE;
std::vector<PositionedUnits> MAP_MINERALS;
// Indicates number already in construction/training for UnitType.
std::map<BWAPI::UnitType, short> PENDING_UNIT_TYPE_COUNT;
std::unordered_map<BWAPI::Unit, BWAPI::Unit> UNIT_CREATOR;
std::vector<EcoBase*> MY_BASES;
locationSet SCOUT_LOCATIONS;
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
    // Used only by Zerg.
    TRAINING[SUPPLY_TYPE].includeFacility(BASE_CENTER);
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
    MAP_MINERALS = GW::getMapMinerals();
    auto startPosition = BWAPI::Position(startLocation);
    std::sort(MAP_MINERALS.begin(), MAP_MINERALS.end(),
        [startPosition](PositionedUnits pu1, PositionedUnits pu2)
        {
            return (startPosition.getApproxDistance(pu1.first) <
                    startPosition.getApproxDistance(pu2.first));
        });
    SCOUT_LOCATIONS = GW::collectScoutingLocations();
    // Workaround cause initial workers may complete before the BASE_CENTER.
    MY_BASES.push_back(new EcoBase(BASE_CENTER, MAP_MINERALS.front().second));
    WORKER_BUFFER = GW::getUnitBuffer(WORKER_TYPE);
}

void GW::onFrame()
{
    const int latency = BWAPI::Broodwar->getLatency(),
              centerPrice = CENTER_TYPE.mineralPrice(),
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
    else if (Production::canProduce(MY_BASES)) {
        Production::produceUnits(MY_BASES, WORKER_TYPE);
    }
    else if (TRAINING[ARMY_UNIT_TYPE].canProduce()) {
        TRAINING[ARMY_UNIT_TYPE].produceUnits(ARMY_UNIT_TYPE);
    }
    else if (!PENDING_UNIT_TYPE_COUNT[CENTER_TYPE] &&
        std::all_of(MY_BASES.begin(), MY_BASES.end(),
            [](EcoBase *Base) { return !Base->isLackingMiners(); }))
    {
        GW::constructExpansion();
    }
    else if (SUPPLY_UNIT && (SELF->minerals() > armyFacilityPrice * 1.5 ||
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
    else if (Unit->getType() == WORKER_TYPE) {
        UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 0);
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
        if (insideEggType == WORKER_TYPE) {
            UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 64);
        }
        else if (insideEggType == SUPPLY_TYPE &&
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
        BWAPI::Unit cUnit = UNIT_CREATOR[Unit];
        auto foundIt = std::find_if(MY_BASES.begin(), MY_BASES.end(),
            [cUnit](EcoBase *Base){ return Base->getCenter() == cUnit; });
        if (foundIt != MY_BASES.end())
            (*foundIt)->assignMiner(Unit);
        else // Should only be initial units.
            MY_BASES.front()->assignMiner(Unit);
    }
    else if (unitType == SUPPLY_TYPE) {
        SUPPLY_UNIT = Unit;
    }
    // Prevent Hatcheries not meant to be a Base;
    // Don't duplicated BASE_CENTER EcoBase, done at onStart.
    else if (unitType == CENTER_TYPE &&
        !Unit->getClosestUnit(IsResourceDepot, 300) && Unit != BASE_CENTER)
    {
        // Append to MY_BASES after finding nearest mineralCluster.
        for (PositionedUnits mineralCluster: MAP_MINERALS) {
            BWAPI::Unit center = BWAPI::Broodwar->getClosestUnit(
                mineralCluster.first, IsResourceDepot, 300);
            if (center == Unit) {
                MY_BASES.push_back(new EcoBase(Unit, mineralCluster.second));
                break;
            }
        }
        WORKER_BUFFER = GW::getUnitBuffer(WORKER_TYPE);
    }
    // else {
        // BWAPI::Broodwar->sendTextEx(true, "Exceptional completion: %s.",
            // unitType.c_str());
    // }
    UNIT_CREATOR.erase(Unit);
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
        else if (unitType.isResourceDepot()) {
            // Includes potential Lair or Hive with isResourceDepot.
            auto foundIt = std::find_if(MY_BASES.begin(), MY_BASES.end(),
                [Unit](EcoBase *Base){ return Base->getCenter() == Unit; });
            if (foundIt != MY_BASES.end()) {
                delete *foundIt;
                MY_BASES.erase(foundIt);
            }
        }
    }
    else if (unitType.isMineralField()) {
        BWAPI::Broodwar->sendText("Mineral field was destroyed.");
        for (EcoBase *Base: MY_BASES)
            Base->removeMineral(Unit);
    }
    else if (unitType.isBuilding()) {
        GW::removeLocation(owningPlayer, Unit->getTilePosition());
        GW::attack_from(Unit->getPosition());
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
    if (Unit->getPlayer() == SELF)
        return;  // Ignoring owned units.
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == SELF)
        return;  // Ignoring owned units.
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == SELF)
        return;  // Ignoring owned units.
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
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
    for (EcoBase *Base: MY_BASES) {
        delete Base;
        Base = nullptr;
    }
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

std::vector<PositionedUnits> GW::getMapMinerals()
{
    // Group static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    std::vector<PositionedUnits> mapMinerals;
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters possibly used as terrain.
        if (mineralCluster.size() > 4)
            mapMinerals.push_back(std::make_pair(
                mineralCluster.getPosition(), mineralCluster));
    }
    return mapMinerals;
}

int GW::getAvailableSupply()
{
    const int supply = SUPPLY_TYPE.supplyProvided(),
              centerSupply = CENTER_TYPE.supplyProvided();
    int supplyConstructing = (PENDING_UNIT_TYPE_COUNT[SUPPLY_TYPE] * supply +
        PENDING_UNIT_TYPE_COUNT[CENTER_TYPE] * centerSupply);
    return SELF->supplyTotal() + supplyConstructing - SELF->supplyUsed();
}

int GW::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Preventing repedative convertion into a float by using a float.
    const float supplyBuildTime = SUPPLY_TYPE.buildTime();
    int unitSupply = unitType.supplyRequired(),
        unitBuildTime = unitType.buildTime(),
        facilityAmount = (unitType == WORKER_TYPE ? MY_BASES.size()
            : TRAINING[unitType].facilityCount()),
        unitsDuringBuild = facilityAmount * std::ceil(
            supplyBuildTime / unitBuildTime);
    return unitsDuringBuild * unitSupply;
}

void GW::scout()
{
    BWAPI::Unitset workerUnits = BASE_CENTER->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    auto workerIt = workerUnits.begin();
    for (BWAPI::TilePosition scoutLocation: SCOUT_LOCATIONS) {
        BWAPI::Unit Scout = *workerIt++;
        Scout->move(BWAPI::Position(scoutLocation));
        Scout->gather(Scout->getClosestUnit(IsMineralField), true);
        // BWAPI::Broodwar->sendTextEx(true, "%d SCOUTING.", Scout->getID());
        // workerUnits.erase(Scout);
    }
}

void GW::scoutLocations(std::vector<PositionedUnits> mapMinerals)
{
    BWAPI::Unit mineralScout = BASE_CENTER->getClosestUnit(
        IsWorker && !IsCarryingMinerals && IsOwned && !IsConstructing);
    mineralScout->stop();  // Because the following orders are queued.
    for (auto pair: mapMinerals)
        mineralScout->move(pair.first, true);
    BWAPI::Broodwar->sendTextEx(true, "%d SCOUTING.",
        mineralScout->getID());
}

void GW::attackLocations(
        BWAPI::Unitset unitGroup, std::vector<PositionedUnits> mapMinerals)
{
    BWAPI::Position attackerPosition = unitGroup.getPosition();
    std::sort(MAP_MINERALS.begin(), MAP_MINERALS.end(),
        [attackerPosition](PositionedUnits pu1, PositionedUnits pu2)
        {
            return (attackerPosition.getApproxDistance(pu1.first) <
                    attackerPosition.getApproxDistance(pu2.first));
        });
    for (auto pair: mapMinerals) {
        BWAPI::Position targetPos = pair.first;
        if (!BWAPI::Broodwar->isVisible(BWAPI::TilePosition(targetPos)))
            unitGroup.attack(targetPos, true);
    }
}

void GW::attack_from(BWAPI::Position Position)
{
    BWAPI::Unitset Attackers = BWAPI::Broodwar->getUnitsInRadius(
        Position, 900, GetType == ARMY_UNIT_TYPE);
    bool attackersAvailable = !Attackers.empty();
    for (auto mapPair: ENEMY_LOCATIONS) {
        locationSet enemyLocations = mapPair.second;
        if (attackersAvailable && !enemyLocations.empty()) {
            BWAPI::TilePosition attackLocation = *enemyLocations.begin();
            BWAPI::Broodwar->sendTextEx(true, "attacking location %d, %d.",
                attackLocation.x, attackLocation.y);
            Attackers.attack(BWAPI::Position(attackLocation));
            return;
        }
    }
    if (attackersAvailable)
        attackLocations(Attackers, MAP_MINERALS);
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
            Task = Other;
    }
    return Task;
}

BWAPI::TilePosition GW::getExpansionLocation(BWAPI::Unit centerContractor)
{
    //! Function may cause performance lag.
    const int indexMax = MAP_MINERALS.size();
    static int expandIndex = 0;
    auto expansionLocation = BWAPI::TilePositions::Invalid;
    if (!centerContractor)
        return expansionLocation;
    // Wrap around iteration of MAP_MINERALS positons.
    int indexCycle = expandIndex + indexMax;
    BWAPI::Position avgMineralPosition;
    BWAPI::TilePosition avgMineralLocation;
    do {
        avgMineralPosition = MAP_MINERALS[expandIndex % indexMax].first;
        avgMineralLocation = BWAPI::TilePosition(avgMineralPosition);
        ++expandIndex;
    }
    while (expandIndex < indexCycle && (
           BWAPI::Broodwar->isVisible(avgMineralLocation) ||
           !centerContractor->hasPath(avgMineralPosition)));
    // If all avgMineralPositions are unacceptable.
    if (expandIndex != indexCycle) {
        expansionLocation = BWAPI::Broodwar->getBuildLocation(
            CENTER_TYPE, avgMineralLocation, 24);
    }
    ++expandIndex;  // Prevent immediate duplication of return.
    return expansionLocation;
}

void GW::constructUnit(BWAPI::UnitType constructableType,
    BWAPI::TilePosition constructionLocation, BWAPI::Unit contractorUnit,
    int Task = -1)
{
    enum {Unexceptable = -1, Position = 1, Build};
    if (Task == Unexceptable)
        Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
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

void GW::constructUnit(BWAPI::UnitType constructableType)
{
    enum {Position = 1, Build};
    static BWAPI::Unit contractorUnit = nullptr;
    static BWAPI::TilePosition constructionLocation = BWAPI::TilePositions::None;
    int Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
        case Position:
            GW::constructUnit(
                constructableType, constructionLocation, contractorUnit, Task);
        case Build:
            break;  // Do not reissue command;
        default:
            contractorUnit = BASE_CENTER->getClosestUnit(IsWorker);
            constructionLocation = BWAPI::Broodwar->getBuildLocation(
                constructableType, contractorUnit->getTilePosition());
            if (constructionLocation != BWAPI::TilePositions::Invalid)
                contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}
    
void GW::constructExpansion()
{
    enum {Position = 1, Build};
    static BWAPI::Unit centerContractor = nullptr;
    static BWAPI::TilePosition expansionLocation = (
        BWAPI::TilePositions::Invalid);
    int Task = GW::getContractorTask(centerContractor);
    switch (Task) {
        case Position:
            bool isVisible;  // Prevents error C2360 by not initalizing.
            isVisible = BWAPI::Broodwar->isVisible(expansionLocation);
            if (isVisible && BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(expansionLocation), IsResourceDepot, 300))
            {
                expansionLocation = GW::getExpansionLocation(centerContractor);
                if (expansionLocation != BWAPI::TilePositions::Invalid)
                    centerContractor->move(BWAPI::Position(expansionLocation));
            }
            else if (isVisible) {
                GW::constructUnit(
                    CENTER_TYPE, expansionLocation, centerContractor, Task);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            centerContractor = BASE_CENTER->getClosestUnit(IsWorker);
            expansionLocation = GW::getExpansionLocation(centerContractor);
            if (expansionLocation != BWAPI::TilePositions::Invalid) {
                MY_BASES.front()->releaseMiner(centerContractor);
                centerContractor->move(BWAPI::Position(expansionLocation));
            }
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
    BWAPI::Broodwar->drawTextScreen(3, 40, "MY_BASES: %d",
        MY_BASES.size());
    int row = 50;
    for (EcoBase *Base: MY_BASES) {
        BWAPI::Broodwar->drawTextScreen(3, row,
            "  Miners: %d, Minerals: %d, LackingMiners %s.",
            Base->getMinerCount(), Base->getMineralCount(),
            Base->isLackingMiners() ? "true" : "false");
        row += 10;
    }
    row += 5;
    BWAPI::Broodwar->drawTextScreen(3, row, "SCOUT_LOCATIONS: %d",
        SCOUT_LOCATIONS.size());
    row += 10;
    for (auto playerLocations: ENEMY_LOCATIONS) {
        BWAPI::Player Player = playerLocations.first;
        locationSet Locations = playerLocations.second;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "%s: %s, locationCount %d", Player->getName().c_str(),
            Player->getRace().c_str(), Locations.size());
        for (auto Location: Locations) {
            row += 10;
            BWAPI::Broodwar->drawTextScreen(3, row,
                "(%d, %d)", Location.x, Location.y);
        }
        row += 15;
    }
}

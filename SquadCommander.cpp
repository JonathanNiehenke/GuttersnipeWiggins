#ifndef SQUADCOMMANDER_CPP
#define SQUADCOMMANDER_CPP
#include "SquadCommander.h"

using namespace BWAPI::Filter;

int compareEnemyTargets::getDamage(BWAPI::UnitType unitType)
{
    // Zealots attack in two volleys so damage factor is included.
    // Hydralisks have explosive damage so damage type is included.
    BWAPI::WeaponType unitWeapon = unitType.groundWeapon();
    return (unitWeapon.damageAmount() * unitWeapon.damageFactor() *
        (unitWeapon.damageType() == BWAPI::DamageTypes::Explosive ? 0.5 : 1));
}

int compareEnemyTargets::getDurability(BWAPI::Unit unit)
{
    return  (unit->getShields() + unit->getHitPoints() *
        (unit->getType().armor() + 1));
}

bool compareEnemyTargets::isJunk(BWAPI::Unit unit, BWAPI::UnitType unitType)
{
    return unitType.isBuilding() && (!unit->isPowered() || unitType.isAddon());
}

BWAPI::Unit compareEnemyTargets::operator()(BWAPI::Unit u1, BWAPI::Unit u2)
{
    // Targeting to reduce incoming damage. Focus on those who deal
    // greater damage, are attacking, are about to die, or are closer.
    // Assuming most unequal unit types have different damage output.
    BWAPI::UnitType u1Type = u1->getType(), u2Type = u2->getType();
    if (u1Type != u2Type) {
        // Causes harmful triggering of needToGroup during battle.
        // Prioritizing high templar, medics, ghosts and lurkers.
        // if (u1Type.isSpellcaster() || u1Type.isBurrowable() !=
            // u2Type.isSpellcaster() || u2Type.isBurrowable())
            // return u1Type.isSpellcaster() || u1Type.isBurrowable()? u1 : u2;
        int u1Damage = getDamage(u1Type), u2Damage = getDamage(u2Type);
        if (u1Damage != u2Damage)
            return u1Damage > u2Damage ? u1 : u2;
        // To reach this far I assume nearlly all are non-weaponized.
        // Avoiding eggs (200hp, 10armor) then larva (25hp, 10armor).
        if ((u1Type == BWAPI::UnitTypes::Zerg_Egg) !=
            (u2Type == BWAPI::UnitTypes::Zerg_Egg))
            return u1Type != BWAPI::UnitTypes::Zerg_Egg ? u1 : u2;
        if ((u1Type == BWAPI::UnitTypes::Zerg_Larva) !=
            (u2Type == BWAPI::UnitTypes::Zerg_Larva))
            return u1Type != BWAPI::UnitTypes::Zerg_Larva ? u1 : u2;
        // Avoiding powerless or addon buildings.
        if (isJunk(u1, u1Type) != isJunk(u2, u2Type))
            return !isJunk(u1, u1Type) ? u1 : u2;
    }
    // Prioritizing zerglings over drones who have the same damage.
    // ToDo: Change condition, isAttacking is true during annimation.
    if (u1->isAttacking() != u2->isAttacking())
        return u1->isAttacking() ? u1 : u2;
    // To reach this far I assume units are of the same unit type.
    // Prioritizing the one closer to death. Dead units lack damage!
    int u1Durability = getDurability(u1), u2Durability = getDurability(u2);
    if (u1Durability != u2Durability)
        return u1Durability < u2Durability ? u1 : u2;
    // Lastly, prioritizing closer targets.
    if (sourcePosition.getApproxDistance(u1->getPosition()) <
        sourcePosition.getApproxDistance(u2->getPosition()))
        return u1;
    else
        return u2;
}


SquadCommander::SquadCommander()
{
    this->self = nullptr;
    this->cartographer = nullptr;
}

void SquadCommander::onStart(Cartographer *cartographer)
{
    this->self = BWAPI::Broodwar->self();
    this->cartographer = cartographer;
}

void SquadCommander::drawSquadGather(BWAPI::Position Pos, int Range)
{
    // Live debugging info.
    BWAPI::Broodwar->registerEvent(
        [Pos, Range](BWAPI::Game*){
            BWAPI::Broodwar->drawCircleMap(Pos, Range,
                BWAPI::Color(255, 127, 0), false);  // Orange range.
        },  nullptr, 24);
}

void SquadCommander::manageAttackGroups() {
    uniteSquads();
    removeEmptySquads();
}

void SquadCommander::assembleSquads(BWAPI::Unit warrior) {
    BWAPI::Unitset Squad = BWAPI::Broodwar->getUnitsInRadius(
        warrior->getPosition(), 50, GetType == warrior->getType() && IsOwned);
    drawSquadGather(warrior->getPosition(), 50);
    if (Squad.size() > 2)
        armySquads.push_back(Squad);
}

void SquadCommander::removeWarrior(BWAPI::Unit deadWarrior)
{
    for (BWAPI::Unitset &Squad: armySquads) {
        Squad.erase(deadWarrior);
    }
}

void SquadCommander::uniteSquads()
{
    // Joining nearby squads to increase strength and coordination.
    int armySquadLength = armySquads.size();
    if (armySquadLength < 2) return;  // Prevent out of range error.
    for (int i = 0; i < armySquadLength - 1; ++ i) {
        BWAPI::Unitset &Squad = armySquads[i];
        Utils::Position fromSquadPos(Squad.getPosition());
        for (int j = i + 1; j < armySquadLength; ++j) {
            BWAPI::Unitset &otherSquad = armySquads[j];
            if (fromSquadPos - otherSquad.getPosition() < 250)
            {
                Squad.insert(otherSquad.begin(), otherSquad.end());
                otherSquad.clear();
            }
        }
    }
}

void SquadCommander::removeEmptySquads()
{
    armySquads.erase(
        std::remove_if(armySquads.begin(), armySquads.end(), 
            //! C2064 Doesn't evaluate to a function taking 1 arguments
            // &SquadCommander::isEmptySquad),
            [](BWAPI::Unitset Squad) { return Squad.empty(); }),
        armySquads.end());
}

void SquadCommander::drawSqaudTargetRange(BWAPI::Position squadPos)
{
    // Live debugging info.
    BWAPI::Broodwar->registerEvent(
        [squadPos](BWAPI::Game*){
            BWAPI::Broodwar->drawCircleMap(squadPos, 8,
                BWAPI::Color(0, 255, 0), true);  // Green squadPos.
            BWAPI::Broodwar->drawCircleMap(squadPos, 600,
                BWAPI::Color(0, 0, 255), false);  // Blue range.
        },  nullptr, 5);
}

void SquadCommander::drawBullsEye(BWAPI::Unit targetUnit)
{
    // Red with inset yellow circle for targetUnit.
    BWAPI::Broodwar->registerEvent(
        [targetUnit](BWAPI::Game*) {
            BWAPI::Broodwar->drawCircleMap(
                targetUnit->getPosition(), 6,
                BWAPI::Color(255, 0, 0), true);
            BWAPI::Broodwar->drawCircleMap(
                targetUnit->getPosition(), 3,
                BWAPI::Color(255, 255, 0), true);
        }, nullptr, 5);
}

bool SquadCommander::needToGroup(BWAPI::Unitset Squad, BWAPI::Position squadPos)
{
    bool notGrouped = false;
    int groupSize = 23 * std::sqrt(Squad.size() * 5);
    for (BWAPI::Unit Warrior: Squad) {
        BWAPI::Unit currentTarget = Warrior->getTarget();
        if (currentTarget && Warrior->isInWeaponRange(currentTarget))
            return false;
        notGrouped = notGrouped ? true :
            squadPos.getApproxDistance(Warrior->getPosition()) > groupSize;
    }
    return notGrouped;
}

void SquadCommander::attackUnit(BWAPI::Unitset Squad, BWAPI::Unit targetUnit)
{
    for (BWAPI::Unit Warrior: Squad) {
        // ?What is the difference between isAttacking and isAttackFrame.
        if (Warrior->isAttackFrame()) return;  // Continue annimation.
        BWAPI::UnitCommand lastCmd = Warrior->getLastCommand();
        BWAPI::Unit attackedUnit = lastCmd.getTarget();
        BWAPI::Position targetPosition = targetUnit->getPosition();
        if (Warrior->isInWeaponRange(targetUnit) && attackedUnit != targetUnit)
        {
            Warrior->attack(targetUnit);
        }
        else if (!Warrior->canAttackUnit(attackedUnit)
                 && lastCmd.getTargetPosition() != targetPosition)
        {
            Warrior->attack(targetPosition);
        }
    }
}

void SquadCommander::attackLocations(
    BWAPI::Unitset Squad, std::vector<BWAPI::Position> resourcePositions)
{
    // Target closer resource positions first.
    BWAPI::Position squadPos = Squad.getPosition();
    std::sort(resourcePositions.begin(), resourcePositions.end(),
        Utils::Position(squadPos).comparePositions());
    for (BWAPI::Position targetPos: resourcePositions) {
        // The locations we see are likely ours and don't bother
        // locations we can't reach.
        if (!BWAPI::Broodwar->isVisible(BWAPI::TilePosition(targetPos)) &&
            BWAPI::Broodwar->hasPath(squadPos, targetPos))
        {
            Squad.attack(targetPos, true);
        }
    }
}

void SquadCommander::attackPositon(BWAPI::Unitset Squad)
{
    BWAPI::TilePosition attackLocation = cartographer->getClosestEnemyLocation(
        Squad.getPosition());
    if (attackLocation == BWAPI::TilePositions::Unknown)
    {
        // ToDo: Redirect all squads when a building is found.
        // No enemy locations so attack every resource location.
        attackLocations(Squad, cartographer->getResourcePositions());
    }
    else {
        Squad.attack(BWAPI::Position(attackLocation));
    }
}

void SquadCommander::combatMicro()
{
    BWAPI::Unit targetUnit;
    for (BWAPI::Unitset Squad: armySquads) {
        BWAPI::Position squadPos = Squad.getPosition();
        drawSqaudTargetRange(squadPos);  // Live debug info.
        targetUnit = BWAPI::Broodwar->getBestUnit(
            compareEnemyTargets(squadPos),  // Determines best after
            IsEnemy && IsDetected && !IsFlying,  // these conditions
            squadPos,  // are applied to units around this positon
            600);  // at this range.
        if (targetUnit) {
            drawBullsEye(targetUnit);  // Live debug info.
            bool canAttack = targetUnit->getType().groundWeapon() != noWeapon;
            BWAPI::Unit beatUnit = targetUnit->getTarget();
            beatUnit = beatUnit ? beatUnit : targetUnit->getOrderTarget();
            bool isAttacking = beatUnit && beatUnit->getPlayer() == self;
            if (canAttack && !isAttacking && needToGroup(Squad, squadPos)) {
                Squad.move(squadPos);  // Move away
                Squad.attack(targetUnit->getPosition(), true);
            }
            else {
                attackUnit(Squad, targetUnit);
            }
        }
        else if (std::any_of(Squad.begin(), Squad.end(),
            [](BWAPI::Unit unit)
                { return (unit->getOrder() == BWAPI::Orders::PlayerGuard); }))
        {
            attackPositon(Squad);
        }
    }
}

void SquadCommander::displayStatus(int &row)
{
    int armySquadlength = armySquads.size();
    for (int i = 0; i < armySquadlength; ++i) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(
            3, row, "Sqaud #%d: %d Warriors" , i, armySquads[i].size());
    }
    row += 5;
}

#endif

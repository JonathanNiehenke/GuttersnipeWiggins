#pragma once
#include <BWAPI.h>
#include "Utils.h"

namespace {

    class TargetPrioritizer {
        private:
            std::vector<BWAPI::Unit> enemyUnits;
            BWAPI::Position avgPosition;
            static bool greaterPriority(
                const BWAPI::Unit&, const BWAPI::Unit&);
            static int byType(const BWAPI::UnitType& unitType);
            static bool hasWeapon(const BWAPI::UnitType& unitType);
            static int byDamage(const BWAPI::UnitType& unitType);
            static int byDurability(const BWAPI::Unit& unit);
        public:
            BWAPI::Position getAvgPosition() const { return avgPosition; }
            void setTargets(const BWAPI::Unitset& targets);
            std::vector<BWAPI::Unit>::const_iterator begin() const
                { return enemyUnits.cbegin(); }
            std::vector<BWAPI::Unit>::const_iterator end() const
                { return enemyUnits.cend(); }
            bool empty() const { return enemyUnits.empty(); }
    };

    class Squad {
        private:
            BWAPI::Unitset members;
            TargetPrioritizer targets;
            bool isUnderAttack() const;
            void aquireAggresiveTargets();
            void aquireNormalTargets();
            void attackPosition() const;
            bool isAttackingPosition(const BWAPI::Unit& squadMember) const;
            void attackTargets() const;
            void moveToClosestTarget(const BWAPI::Unit& squadMember) const;
            BWAPI::Unit getClosestTarget(const BWAPI::Unit& squadMember) const;
            bool isAttackingTarget(const BWAPI::Unit& squadMember) const;
            bool memberIsTargeting(
                const BWAPI::Unit&, const BWAPI::Unit&) const;
        public:
            Squad(const BWAPI::Unit&, const BWAPI::Position&);
            BWAPI::Position aggresivePosition;
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            int size() const { return members.size(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            bool isJoinable(const Squad& otherSquad) const;
            void join(Squad& otherSquad);
            // void split(const int& newSquadAmount);
            void aquireTargets();
            void attack() const;
            bool completedAttack() const;
    };
}

class SquadCommander {
    private:
        std::vector<Squad> deployedForces;
        void uniteNearBySquads();
        void removeEmptySquads();
    public:
        void commandSquadsTo(const BWAPI::Position& attackPosition);
        void sendUnitToAttack(const BWAPI::Unit&, const BWAPI::Position&);
        void removeFromDuty(const BWAPI::Unit& armyUnit);
        void updateGrouping();
        void updateTargeting();
        void updateAttacking();
        std::vector<BWAPI::Position*> completed();
        void completeMissions();
        void drawStatus(int& row) const;
};

#pragma once
#include <BWAPI.h>
#include "Combat.h"
#include "Utils.h"

namespace {

    class ReservedForces {
        private:
            typedef std::function<bool(BWAPI::Unitset)> DeploymentPred;
            DeploymentPred deploymentPred;
            std::vector<BWAPI::Unitset> forces;
            void includeForce(const BWAPI::Unit& unit);
        public:
            ReservedForces(DeploymentPred deploymentPred)
                : deploymentPred(deploymentPred) {}
            void include(const BWAPI::Unit& unit);
            void discard(const BWAPI::Unit& unit);
            BWAPI::Unitset release();
            void drawStatus(int row) const;
    };

    class Squad {
        private:
            BWAPI::Unitset members;
            Combat combat;
            bool isJoinable(const Squad& otherSquad) const;
            Squad(const BWAPI::Unit& unit, const BWAPI::Position& position);
        public:
            Squad(const BWAPI::Unitset& units)
                : members(units), combat(units.getPosition()) {}
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            void join(Squad& otherSquad);
            std::vector<Squad> spreadTo(std::vector<BWAPI::Position> sPos);
            bool combatComplete();
            BWAPI::Position combatPosition();
            void combatPosition(const BWAPI::Position& position);
            void prepareCombat();
            void engageCombat() const;
    };

    class DeployedForces {
        private:
            typedef std::vector<BWAPI::Position> PosSeries;
            typedef std::function<BWAPI::Position(BWAPI::Position)> PosToPos;
            PosToPos nextPosition;
            std::vector<Squad> forces;
            void funnel();
            void terminate();
            void assignCombatPosition(Squad& squad) const;
            void prepare();
        public:
            DeployedForces(PosToPos nextPosition) : nextPosition(nextPosition) {}
            void incorporate(const BWAPI::Unitset& units);
            void deactivate(const BWAPI::Unit& armyUnit);
            void search(PosSeries searchPositions);
            void charge();
            void execute() const;
            void focus(const BWAPI::Position& attackPosition);
            void drawStatus(int row) const;
    };

}

class SquadCommander {
    private:
        typedef std::function<bool(BWAPI::Unitset)> DeploymentPred;
        typedef std::function<BWAPI::Position(BWAPI::Position)> PosToPos;
        ReservedForces reservedForces;
        DeployedForces deployedForces;
        void deploy();
    public:
        SquadCommander(DeploymentPred deploymentPred, PosToPos nextPosition)
            : reservedForces(deploymentPred), deployedForces(nextPosition) {}
        void include(const BWAPI::Unit& unit);
        void deactivate(const BWAPI::Unit& armyUnit);
        void search(std::vector<BWAPI::Position> searchPositions);
        void charge();
        void execute() const;
        void focus(const BWAPI::Position& attackPosition);
        void drawStatus(int row) const;
};

#include "BattlePhaseManager.h"
#include "BattleState.h" // BattlePhase enum定義のため
#include "../entities/Player.h"
#include "../entities/Enemy.h"

BattlePhaseManager::BattlePhaseManager(BattleLogic* battleLogic, Player* player, Enemy* enemy)
    : battleLogic(battleLogic), player(player), enemy(enemy) {
}

BattlePhaseManager::PhaseTransitionResult BattlePhaseManager::updatePhase(
    const PhaseUpdateContext& context, float deltaTime) {
    
    PhaseTransitionResult result;
    result.shouldTransition = false;
    result.resetTimer = false;
    result.nextPhase = context.currentPhase;
    
    switch (context.currentPhase) {
        case BattlePhase::INTRO:
            if (context.phaseTimer > BattleConstants::INTRO_DURATION) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::COMMAND_SELECT;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::COMMAND_SELECT:
            if (context.currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::JUDGE;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::JUDGE:
            break;
            
        case BattlePhase::JUDGE_RESULT:
            if (context.phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::EXECUTE;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::EXECUTE:
            if (context.currentExecutingTurn >= static_cast<int>(context.pendingDamagesSize)) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::END;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::DESPERATE_COMMAND_SELECT:
            if (context.currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::DESPERATE_JUDGE;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::DESPERATE_JUDGE:
            break;
            
        case BattlePhase::DESPERATE_JUDGE_RESULT:
            if (context.phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::DESPERATE_EXECUTE;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::DESPERATE_EXECUTE:
            if (context.currentExecutingTurn >= static_cast<int>(context.pendingDamagesSize)) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::END;
                result.resetTimer = true;
            }
            break;
            
        default:
            break;
    }
    
    return result;
}

bool BattlePhaseManager::shouldTransitionToNextPhase(
    BattlePhase currentPhase, const PhaseUpdateContext& context) const {
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            return context.phaseTimer > BattleConstants::INTRO_DURATION;
            
        case BattlePhase::COMMAND_SELECT:
            return context.currentSelectingTurn >= battleLogic->getCommandTurnCount();
            
        case BattlePhase::JUDGE_RESULT:
            return context.phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION;
            
        case BattlePhase::EXECUTE:
            return context.currentExecutingTurn >= static_cast<int>(context.pendingDamagesSize);
            
        case BattlePhase::DESPERATE_COMMAND_SELECT:
            return context.currentSelectingTurn >= battleLogic->getCommandTurnCount();
            
        case BattlePhase::DESPERATE_JUDGE_RESULT:
            return context.phaseTimer > BattleConstants::JUDGE_RESULT_ANNOUNCEMENT_DURATION;
            
        case BattlePhase::DESPERATE_EXECUTE:
            return context.currentExecutingTurn >= static_cast<int>(context.pendingDamagesSize);
            
        default:
            return false;
    }
}

BattlePhase BattlePhaseManager::getNextPhase(
    BattlePhase currentPhase, const PhaseUpdateContext& context) const {
    
    switch (currentPhase) {
        case BattlePhase::INTRO:
            return BattlePhase::COMMAND_SELECT;
            
        case BattlePhase::COMMAND_SELECT:
            return BattlePhase::JUDGE;
            
        case BattlePhase::JUDGE_RESULT:
            return BattlePhase::EXECUTE;
            
        case BattlePhase::EXECUTE:
            return BattlePhase::END;
            
        case BattlePhase::DESPERATE_COMMAND_SELECT:
            return BattlePhase::DESPERATE_JUDGE;
            
        case BattlePhase::DESPERATE_JUDGE_RESULT:
            return BattlePhase::DESPERATE_EXECUTE;
            
        case BattlePhase::DESPERATE_EXECUTE:
            return BattlePhase::END;
            
        default:
            return currentPhase;
    }
}


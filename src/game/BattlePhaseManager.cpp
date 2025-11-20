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
            // JUDGE_RESULTフェーズ内で全処理を完結するため、自動遷移は行わない
            // ダメージ適用完了後、BattleState側で直接COMMAND_SELECTまたはENDへ遷移
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
            // DESPERATE_JUDGE_RESULTフェーズ内で全処理を完結するため、自動遷移は行わない
            // ダメージ適用完了後、BattleState側で直接COMMAND_SELECTまたはENDへ遷移
            break;
            
        case BattlePhase::LAST_CHANCE_COMMAND_SELECT:
            if (context.currentSelectingTurn >= battleLogic->getCommandTurnCount()) {
                result.shouldTransition = true;
                result.nextPhase = BattlePhase::LAST_CHANCE_JUDGE;
                result.resetTimer = true;
            }
            break;
            
        case BattlePhase::LAST_CHANCE_JUDGE:
            break;
            
        case BattlePhase::LAST_CHANCE_JUDGE_RESULT:
            // LAST_CHANCE_JUDGE_RESULTフェーズ内で全処理を完結するため、自動遷移は行わない
            // ダメージ適用完了後、BattleState側で直接ENDへ遷移
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
            
        case B
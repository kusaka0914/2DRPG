#include "PlayerTrust.h"

PlayerTrust::PlayerTrust(int initialTrustLevel, bool initialIsEvil)
    : trustLevel(initialTrustLevel), isEvil(initialIsEvil), evilActions(0), goodActions(0) {
    if (trustLevel > 100) trustLevel = 100;
    if (trustLevel < 0) trustLevel = 0;
}

void PlayerTrust::changeTrustLevel(int amount) {
    trustLevel += amount;
    if (trustLevel > 100) trustLevel = 100;
    if (trustLevel < 0) trustLevel = 0;
}

void PlayerTrust::recordEvilAction() {
    evilActions++;
    changeTrustLevel(-5); // 悪行で信頼度が下がる
}

void PlayerTrust::recordGoodAction() {
    goodActions++;
    changeTrustLevel(3); // 善行で信頼度が上がる
}


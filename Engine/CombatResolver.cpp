#include "pch.h"
#include "CombatResolver.h"
#include "DiceSystem.h"
#include "LogSystem.h"

#include <algorithm>

CombatRollResult CombatResolver::ResolveAttack(const AttackProfile& attack, 
                                               const DefenseProfile& defense, 
                                               DiceSystem& diceSystem,
                                               LogSystem* logger) const
{
    CombatRollResult result{};

    const CombatStatRoll combatStat = diceSystem.RollCombatStat(RandomDomain::Combat);
    result.roll  = combatStat.d20;
    result.total = result.roll + attack.attackModifier;

    const bool critical = attack.allowCritical && result.roll == 20;
    const bool autoFail = attack.autoFailOnOne && result.roll == 1;

    if (autoFail)
        result.hit = HitResult::Miss;
    else if (critical || result.total >= defense.defense)
    {
        result.hit = critical ? HitResult::Critical : HitResult::Hit;
        const int maxDamage = std::max(attack.minDamage, attack.maxDamage);
        DiceConfig config{ 1, maxDamage, 0 };
        const int rolledDamage = diceSystem.RollTotal(config, RandomDomain::Combat);
        result.damage = rolledDamage + attack.attackModifier;       // 장착 무기 + 힘 or 민첩 수정치

        if (critical)
        {
            result.damage *= 2;
        }
    }

    if (logger)
    {
		LogContext context{};
		context.actor = attack.attackerName.empty() ? "Actor" : attack.attackerName;
		context.target = attack.targetName;
		context.item = attack.itemName;

		if (attack.isThrow)
		{
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Throw, context);
		}

		const LogMessageType type = (result.hit == HitResult::Miss) ? LogMessageType::Miss : LogMessageType::Attack;
		logger->AddLocalized(LogChannel::Combat, type, context);

		if (result.hit != HitResult::Miss && result.damage > 0)
		{
			LogContext damageContext = context;
			damageContext.value = result.damage;
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Damage, damageContext);
		}

		if (result.hit != HitResult::Miss && attack.targetDied)
		{
			LogContext deathContext{};
			deathContext.actor = attack.targetName;
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Death, deathContext);
		}
	}
    
    return result;
}

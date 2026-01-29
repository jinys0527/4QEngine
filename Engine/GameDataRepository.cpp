#include "pch.h"
#include "GameDataRepository.h"

#include "CsvParser.h"

ItemCategory ParseCategory(const std::string& value)
{
	if (value == "Currency")  return ItemCategory::Currency;
	if (value == "Healing")   return ItemCategory::Healing;
	if (value == "Equipment") return ItemCategory::Equipment;
	if (value == "Throwable") return ItemCategory::Throwable;

	return ItemCategory::Currency;
}

bool GameDataRepository::LoadFromFiles(const DataSheetPaths& path, std::string* errorOut)
{
	if (!LoadItemsFromFile(path.itemsPath, errorOut))
	{
		return false;
	}

	if (!LoadEnemiesFromFile(path.enemiesPath, errorOut))
	{
		return false;
	}

	if (!LoadDropTablesFromFile(path.dropTablesPath, errorOut))
	{
		return false;
	}

	return true;
}

const ItemDefinition* GameDataRepository::GetItem(int index) const
{
	auto it = m_Items.find(index);
	
	if (it == m_Items.end())
	{
		return nullptr;
	}

	return &it->second;
}

const EnemyDefinition* GameDataRepository::GetEnemy(int id) const
{
	auto it = m_Enemies.find(id);

	if (it == m_Enemies.end())
	{
		return nullptr;
	}

	return &it->second;
}

const DropTableDefinition* GameDataRepository::GetDropTable(int difficultyGroup) const
{
	auto it = m_DropTables.find(difficultyGroup);

	if (it == m_DropTables.end())
	{
		return nullptr;
	}

	return &it->second;
}

std::vector<const ItemDefinition*> GameDataRepository::GetItemsByCategory(ItemCategory category, int maxDifficulty) const
{
	std::vector<const ItemDefinition*> result;
	for (const auto& [index, item] : m_Items)
	{
		if (item.category != category)
		{
			continue;
		}

		if (item.difficultyGroup > maxDifficulty)
		{
			continue;
		}

		result.push_back(&item);
	}

	return result;
}

bool GameDataRepository::LoadItemsFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Items Sheet path is empty";
		}

		return false;
	}

	m_Items.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		ItemDefinition item{};
		item.index            = ParseInt(GetField(row, header, "index"), 0);
		item.category         = ParseCategory(GetField(row, header, "category"));
		item.name			  = GetField(row, header, "name");
		item.description	  = GetField(row, header, "description");
		item.iconPath		  = GetField(row, header, "iconPath");
		item.meshPath		  = GetField(row, header, "meshPath");
		item.basePrice		  = ParseInt(GetField(row, header, "basePrice"), 0);
		item.difficultyGroup  = ParseInt(GetField(row, header, "difficultyGroup"), 1);

		item.minHealPercent   = ParseInt(GetField(row, header, "minHealPercent"), 0);
		item.maxHealPercent   = ParseInt(GetField(row, header, "maxHealPercent"), 0);
		item.minDamage        = ParseInt(GetField(row, header, "minDamage"), 0);
		item.maxDamage        = ParseInt(GetField(row, header, "maxDamage"), 0);

		item.healthModifier   = ParseInt(GetField(row, header, "healthModifier"), 0);
		item.strengthModifier = ParseInt(GetField(row, header, "strengthModifier"), 0);
		item.agilityModifier  = ParseInt(GetField(row, header, "agilityModifier"), 0);
		item.senseModifier	  = ParseInt(GetField(row, header, "senseModifier"), 0);
		item.skillModifier	  = ParseInt(GetField(row, header, "skillModifier"), 0);
		item.defenseBonus	  = ParseInt(GetField(row, header, "defenseBonus"), 0);

		item.throwRange       = ParseInt(GetField(row, header, "throwRange"), 0);
		item.range            = ParseInt(GetField(row, header, "range"), 0);

		if (item.index != 0)
		{
			m_Items[item.index] = std::move(item);
		}
	}

	return true;
}

bool GameDataRepository::LoadEnemiesFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Enemies sheet path is empty.";
		}
		return false;
	}

	m_Enemies.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		EnemyDefinition enemy{};
		enemy.id				 = ParseInt(GetField(row, header, "id"), 0);
		enemy.name				 = GetField(row, header, "name");
		enemy.health			 = ParseInt(GetField(row, header, "health"), 0);
		enemy.initiativeModifier = ParseInt(GetField(row, header, "initiativeModifier"), 0);
		enemy.defense			 = ParseInt(GetField(row, header, "defense"), 0);
		enemy.accuracyModifier   = ParseInt(GetField(row, header, "accuracyModifier"), 0);
		enemy.damageDiceCount    = ParseInt(GetField(row, header, "damageDiceCount"), 0);
		enemy.damageDiceValue    = ParseInt(GetField(row, header, "damageDiceValue"), 0);
		enemy.damageBonus        = ParseInt(GetField(row, header, "damageBonus"), 0);
		enemy.range				 = ParseInt(GetField(row, header, "range"), 0);
		enemy.moveDistance		 = ParseInt(GetField(row, header, "moveDistance"), 0);
		enemy.sightDistance		 = ParseFloat(GetField(row, header, "sightDistance"), 0.0f);
		enemy.sightAngle		 = ParseFloat(GetField(row, header, "sightAngle"), 0.0f);
		enemy.difficultyGroup	 = ParseInt(GetField(row, header, "difficultyGroup"), 1);
		enemy.dropTableGroup	 = ParseInt(GetField(row, header, "dropTableGroup"), enemy.difficultyGroup);

		if (enemy.id != 0)
		{
			m_Enemies[enemy.id] = std::move(enemy);
		}
	}

	return true;
}

bool GameDataRepository::LoadDropTablesFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Drop tables sheet path is empty.";
		}
		return false;
	}

	m_DropTables.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		const int difficultyGroup = ParseInt(GetField(row, header, "difficultyGroup"), 1);
		DropEntry drop{};
		drop.itemIndex			  = ParseInt(GetField(row, header, "itemIndex"), 0);
		drop.weight				  = ParseFloat(GetField(row, header, "weight"), 0.0f);

		auto& table				  = m_DropTables[difficultyGroup];
		table.difficultyGroup	  = difficultyGroup;
		table.entries.push_back(drop);
	}

	return true;
}

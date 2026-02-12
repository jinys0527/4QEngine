#pragma once
#include "FSMComponent.h"
#include <vector>

class ItemSpawnerComponent;

class PlayerShopFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerShopFSMComponent";
	const char* GetTypeName() const override;

	PlayerShopFSMComponent();
	virtual ~PlayerShopFSMComponent() override = default;

	void Start() override;
	void ConfigureVendingOffer(int fixedPrice, const std::vector<int>& itemIds, ItemSpawnerComponent* spawner);
	void ClearVendingOffer();
private:
	int  ResolveActivePrice() const;
	int  ResolvePurchaseItemId() const;
	void DispatchCurrentMoneyState();
	void DispatchMoneyStateEvent(bool hasMoney);

	int m_SelectedPrice = 0;
	int m_SelectedItemId = -1;

	bool m_UseVendingOffer = false;
	int m_VendingFixedPrice = 10;
	std::vector<int> m_VendingItemIds;
	ItemSpawnerComponent* m_VendingSpawner = nullptr;
};
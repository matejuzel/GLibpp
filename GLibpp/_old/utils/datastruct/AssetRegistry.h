#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

using asset_handle_t = uint32_t;

template<typename T>
struct Slot {
	std::unique_ptr<T> asset;
	uint32_t generation = 0; // zatim nepouzivame - pripraveno na pouziti "index generation"
};

template<typename T>
class AssetRegistry
{
public:

	static constexpr asset_handle_t handle_invalid = std::numeric_limits<asset_handle_t>::max();

	asset_handle_t add(std::unique_ptr<T> asset)
	{
		std::string name = "stdname_" + std::to_string(cntr++);
		return add(std::move(asset), name);
	}

	asset_handle_t add(std::unique_ptr<T> asset, const std::string& name)
	{
		if (name_to_handle.find(name) != name_to_handle.end())
		{
			return handle_invalid;
		}

		slots.emplace_back(Slot<T>{ std::move(asset), 0 });

		auto handle = static_cast<asset_handle_t>(slots.size() - 1);
		name_to_handle[name] = handle;
		return handle;
	}

	T* get(asset_handle_t handle)
	{
		if (handle >= slots.size())
			return nullptr;

		return slots[handle].asset.get();
	}

	T* get(const std::string& name)
	{
		auto it = name_to_handle.find(name);
		return it != name_to_handle.end() ? get(it->second) : nullptr;
	}

	size_t size() const
	{
		return slots.size();
	}

	template<typename F>
	void iterate(F&& callback)
	{
		for (auto& slot : slots) 
		{
			if (slot.asset) {
				callback(*slot.asset);
			}
		}
	}


	//void remove(asset_handle_t handle) // zatim to nepotrebuji, dodelame treba pozdeji

private:

	size_t cntr = 0;
	std::vector<Slot<T>> slots;

	// chtelo by do budoucna
	// 1: zavest "free-list" - evidovat smazane indexy pro znovupouziti
	// 2: zavest "index generation" moznost pouziti uvolnenych indexu
	std::unordered_map<std::string, asset_handle_t> name_to_handle;
};


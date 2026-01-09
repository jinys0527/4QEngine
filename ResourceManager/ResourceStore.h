#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ResourceHandle.h"

// 실제 리소스 데이터
// id -> 실제 데이터 맵핑

template <typename T, typename HandleType = ResourceHandle>
class ResourceStore
{
public:
	// Loader : 실제로 데이터 로드하는 함수 객체
	using Loader = std::function<std::unique_ptr<T>()>;

	HandleType Load(const std::string& key, Loader loader)
	{
		auto it = m_KeyToHandle.find(key);
		if (it != m_KeyToHandle.end())
		{
			HandleType existing = it->second;
			//로딩되어 있을 경우(캐싱)
			if (IsAlive(existing))
			{
				return existing;
			}
		}

		HandleType handle = AllocateHandle();
		Entry& entry = m_Entries[handle.id];
		entry.key = key;
		entry.resource = loader ? loader() : nullptr;
		entry.alive = true;
		entry.generation = handle.generation;

		m_KeyToHandle[key] = handle;
		return handle;
	}

	T* Get(HandleType handle)
	{
		if (!IsAlive(handle))
		{
			return nullptr;
		}

		return m_Entries[handle.id].resource.get();
	}

	const T* Get(HandleType handle) const
	{
		if (!IsAlive(handle))
		{
			return nullptr;
		}

		return m_Entries[handle.id].resource.get();
	}

	void Unload(HandleType handle)
	{
		if (!IsAlive(handle))
		{
			return;
		}

		Entry& entry = m_Entries[handle.id];
		m_KeyToHandle.erase(entry.key);
		entry.resource.reset();
		entry.alive = false;
		++entry.generation;
		m_FreeIds.push_back(handle.id);
	}

	void Clear()
	{
		m_KeyToHandle.clear();
		m_Entries.clear();
		m_FreeIds.clear();
		m_Entries.resize(1); // index 0 reserved for invalid handle
	}

	bool IsAlive(HandleType handle) const
	{
		if (handle.id == 0 || handle.id >= m_Entries.size())
		{
			return false;
		}

		const Entry& entry = m_Entries[handle.id];
		return entry.alive && entry.generation == handle.generation;
	}

#if defined(_DEBUG)
	void DebugDump(std::ostream& os) const
	{
		os << "ResourceStore<" << typeid(T).name() << "> entries=" << (m_Entries.size() > 0 ? m_Entries.size() - 1 : 0)
			<< " freeIds=" << m_FreeIds.size() << "\n";

		for (size_t i = 1; i < m_Entries.size(); ++i)
		{
			const Entry& entry = m_Entries[i];
			os << "  [" << i << "] key=\"" << entry.key << "\" alive=" << (entry.alive ? "true" : "false")
				<< " gen=" << entry.generation << "\n";
		}
	}
#endif

	const std::unordered_map<std::string, HandleType>& GetKeyToHandle() const
	{
		return m_KeyToHandle;
	}

private:
	struct Entry
	{
		std::string        key;
		std::unique_ptr<T> resource;
		UINT32             generation = 1;
		bool               alive = false;
	};

	HandleType AllocateHandle()
	{
		UINT32 id = 0;
		if (!m_FreeIds.empty())
		{
			id = m_FreeIds.back();
			m_FreeIds.pop_back();
		}
		else
		{
			id = static_cast<UINT32>(m_Entries.size());
			m_Entries.emplace_back();
		}
		 
		HandleType handle;
		handle.id = id;
		handle.generation = m_Entries[id].generation;
		return handle;
	}

	std::unordered_map<std::string, HandleType> m_KeyToHandle;
	std::vector<Entry>    m_Entries = std::vector<Entry>(1);
	std::vector<UINT32>   m_FreeIds;
};

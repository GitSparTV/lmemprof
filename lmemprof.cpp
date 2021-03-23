#include <unordered_map>
#include <string>

#include <lua.hpp>

namespace memprof {
lua_Alloc original_allocator = nullptr;

class MemProf {
public:
	inline bool IsEnabled() const {
		return enabled_;
	}

	inline std::string_view GetZone() const {
		return current_zone_;
	}

	inline size_t GetCounter() const noexcept {
		return profiler_zones_.count(current_zone_) ? profiler_zones_.at(current_zone_) : 0;
	}

	inline size_t GetCounter(const std::string_view name) const noexcept {
		return profiler_zones_.count(name) ? profiler_zones_.at(name) : 0;
	}

public:
	inline void Enable() {
		enabled_ = true;
	}

	inline void Disable() {
		enabled_ = false;
	}

	inline void SetZone(const std::string_view name) {
		current_zone_ = name;
	}

	inline void ClearZone() {
		current_zone_ = "";
	}

	inline void IncrementCounter(size_t number) noexcept {
		profiler_zones_[current_zone_] += number;
	}

	inline void DecrementCounter(size_t number) noexcept {
		profiler_zones_[current_zone_] -= number;
	}

	inline void ResetCounter(std::string_view name) noexcept {
		profiler_zones_.insert_or_assign(name, size_t{0});
	}

	inline void ResetCounter() noexcept {
		ResetCounter(current_zone_);
	}

private:
	bool enabled_ = false;
	std::unordered_map<std::string_view, size_t> profiler_zones_;
	std::string_view current_zone_;
};

MemProf prof;

namespace lua {
void* Hook(void* ud, void* ptr, size_t osize, size_t nsize) {
	if (prof.IsEnabled()) {
		if (osize < nsize) {
			prof.IncrementCounter(nsize - osize);
		}
		else {
			prof.DecrementCounter(osize - nsize);
		}
	}
	
	return original_allocator(ud, ptr, osize, nsize);
}

#ifdef FOR_LUA
int IsEnabled(lua_State* L) {
	lua_pushboolean(L, prof.IsEnabled());

	return 1;
}

int GetZone(lua_State* L) {
	const std::string_view name = prof.GetZone();

	lua_pushlstring(L, name.data(), name.size());

	return 1;
}

int Enable([[maybe_unused]] lua_State* L) {
	prof.Enable();

	return 0;
}

int Disable([[maybe_unused]] lua_State* L) {
	prof.Disable();

	return 0;
}

int SetZone(lua_State* L) {
	prof.SetZone(luaL_checkstring(L, 1));

	return 0;
}

int ClearZone([[maybe_unused]] lua_State* L) {
	prof.ClearZone();

	return 0;
}

int IncrementCounter(lua_State* L) noexcept {
	prof.IncrementCounter(static_cast<size_t>(luaL_checknumber(L, 1)));

	return 0;
}

int DecrementCounter(lua_State* L) noexcept {
	prof.DecrementCounter(static_cast<size_t>(luaL_checknumber(L, 1)));

	return 0;
}

int ResetCounter([[maybe_unused]] lua_State* L) noexcept {
	prof.ResetCounter();

	return 0;
}

int ResetCounterFor(lua_State* L) noexcept {
	prof.ResetCounter(luaL_checkstring(L, 1));

	return 0;
}

int GetCounter(lua_State* L) noexcept {
	lua_pushinteger(L, static_cast<lua_Integer>(prof.GetCounter()));

	return 1;
}

int GetCounterFor(lua_State* L) noexcept {
	lua_pushinteger(L, static_cast<lua_Integer>(prof.GetCounter(luaL_checkstring(L, 1))));

	return 1;
}
#endif // FOR_LUA

#ifdef FOR_LUAJIT
extern "C" {
	__declspec(dllexport) bool IsEnabled() {
		return prof.IsEnabled();
	}

	__declspec(dllexport) const char* GetZone() {
		const std::string_view name = prof.GetZone();

		return name.data();
	}

	__declspec(dllexport) void Enable() {
		prof.Enable();

		return;
	}

	__declspec(dllexport) void Disable() {
		prof.Disable();

		return;
	}

	__declspec(dllexport) void SetZone(const char* name) {
		prof.SetZone(name);

		return;
	}

	__declspec(dllexport) void ClearZone() {
		prof.ClearZone();

		return;
	}

	__declspec(dllexport) void IncrementCounter(size_t number) noexcept {
		prof.IncrementCounter(number);

		return;
	}

	__declspec(dllexport) void DecrementCounter(size_t number) noexcept {
		prof.DecrementCounter(number);

		return;
	}

	__declspec(dllexport) void ResetCounter() noexcept {
		prof.ResetCounter();

		return;
	}

	__declspec(dllexport) void ResetCounterFor(const char* name) noexcept {
		prof.ResetCounter(name);

		return;
	}

	__declspec(dllexport) double GetCounter() noexcept {
		return static_cast<double>(prof.GetCounter());
	}

	__declspec(dllexport) double GetCounterFor(const char* name) noexcept {
		return static_cast<double>(prof.GetCounter(name));
	}
}
#endif // FOR_LUAJIT

int Handle__tostring(lua_State* L) {
	lua_pushstring(L, "lmemprof_handle");

	return 1;
}

int Handle__gc(lua_State* L) {
	void* ud;

	lua_getallocf(L, &ud);
	lua_setallocf(L, memprof::original_allocator, ud);

	return 0;
}

} // namespace lua
} // namespace memprof

extern "C" __declspec(dllexport) int luaopen_lmemprof(lua_State * L) noexcept {
	void* ud;

	lua_Alloc allocator = lua_getallocf(L, &ud);
	memprof::original_allocator = allocator;
	lua_setallocf(L, memprof::lua::Hook, ud);

#ifdef FOR_LUA
	lua_createtable(L, 0, 7);
	lua_pushcfunction(L, memprof::lua::GetZone);
	lua_setfield(L, -2, "GetZone");

	lua_pushcfunction(L, memprof::lua::IsEnabled);
	lua_setfield(L, -2, "IsEnabled");

	lua_pushcfunction(L, memprof::lua::Enable);
	lua_setfield(L, -2, "Enable");

	lua_pushcfunction(L, memprof::lua::Disable);
	lua_setfield(L, -2, "Disable");

	lua_pushcfunction(L, memprof::lua::SetZone);
	lua_setfield(L, -2, "SetZone");

	lua_pushcfunction(L, memprof::lua::ClearZone);
	lua_setfield(L, -2, "ClearZone");

	lua_pushcfunction(L, memprof::lua::IncrementCounter);
	lua_setfield(L, -2, "IncrementCounter");

	lua_pushcfunction(L, memprof::lua::DecrementCounter);
	lua_setfield(L, -2, "DecrementCounter");

	lua_pushcfunction(L, memprof::lua::ResetCounter);
	lua_setfield(L, -2, "ResetCounter");

	lua_pushcfunction(L, memprof::lua::ResetCounterFor);
	lua_setfield(L, -2, "ResetCounterFor");

	lua_pushcfunction(L, memprof::lua::GetCounter);
	lua_setfield(L, -2, "GetCounter");

	lua_pushcfunction(L, memprof::lua::GetCounterFor);
	lua_setfield(L, -2, "GetCounterFor");
#endif // FOR_LUA

	lua_newuserdata(L, 0);
	lua_createtable(L, 0, 2);
	
	lua_pushcfunction(L, memprof::lua::Handle__tostring);
	lua_setfield(L, -2, "__tostring");

	lua_pushcfunction(L, memprof::lua::Handle__gc);
	lua_setfield(L, -2, "__gc");

	lua_setmetatable(L, -2);
	luaL_ref(L, LUA_REGISTRYINDEX);

#ifdef FOR_LUA
	return 1;
#elif FOR_LUAJIT
	return 0;
#endif
}
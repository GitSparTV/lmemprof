require("lmemprof")

local ffi = require("ffi")

ffi.cdef([[
	bool IsEnabled();
	const char* GetZone();
	void Enable();
	void Disable();
	void SetZone(const char* name);
	void ClearZone();
	void IncrementCounter(size_t number);
	void DecrementCounter(size_t number);
	void ResetCounter();
	void ResetCounterFor(const char* name);
	double GetCounter();
	double GetCounterFor(const char* name);
]])

return ffi.load("lmemprof.dll")
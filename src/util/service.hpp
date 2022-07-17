#pragma once

#include <utility>
#include "util/verify.hpp"

namespace minote {

// Wrapper for providing globally available scoped access to a class instance
// Singleton on steroids
template<typename T>
struct Service {
	
	struct Stub {
		
		template<typename... Args>
		Stub(Service<T>& service, Args&&... args):
			m_service(service),
			m_instance(std::forward<Args>(args)...),
			m_prevInstance(m_service.m_handle)
			{ m_service.m_handle = &m_instance; }
		~Stub() { m_service.m_handle = m_prevInstance; }
		
		Stub(Stub const&) = delete;
		auto operator=(Stub const&) -> Stub& = delete;
		
	private:
		
		Service<T>& m_service;
		T m_instance;
		T* m_prevInstance;
		
	};
	
	template<typename... Args>
	auto provide(Args&&... args) -> Stub { return Stub(*this, std::forward<Args>(args)...); }
	
	auto operator*() -> T& { return *ASSUME(m_handle); }
	auto operator->() -> T* { return ASSUME(m_handle); }
	
private:
	
	T* m_handle = nullptr;
	
};

}

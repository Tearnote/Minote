#pragma once

#include <utility>
#include "stx/verify.hpp"

namespace minote::util {

// Wrapper for providing globally available scoped access to a class instance
// Singleton on steroids
//
// The list of services currently provided within the project:
// s_system (sys/system.hpp): OS-level functionality
// s_vulkan (sys/vulkan.hpp): Vulkan instance and device properties
// s_renderer (gfx/renderer.hpp): World properties, camera, object list
template<typename T>
class Service {

public:

	class Stub;
	
	// Create an instance of the underlying service. The service will be destroyed
	// once the returned stub goes out of scope.
	template<typename... Args>
	auto provide(Args&&... args) -> Stub {
		
		return Stub(*this, std::forward<Args>(args)...);
		
	}
	
	// Gain access to the currently provisioned instance
	auto operator*() -> T& { return *ASSUME(m_handle); }
	auto operator->() -> T* { return ASSUME(m_handle); }
	
private:

	class Stub {

	public:

		~Stub() { m_service.m_handle = m_prevInstance; }

		template<typename... Args>
		Stub(Service<T>& service, Args&&... args):
			m_service(service),
			m_instance(std::forward<Args>(args)...),
			m_prevInstance(m_service.m_handle) {

			m_service.m_handle = &m_instance;

		}

		// Not copyable or movable
		Stub(Stub const&) = delete;
		auto operator=(Stub const&)->Stub & = delete;

	private:

		Service<T>& m_service;
		T m_instance;
		T* m_prevInstance;

	};
	
	T* m_handle = nullptr;
	
};

}

namespace minote {

template<typename T>
template<typename... Args>
Service<T>::Stub::Stub(Service<T>& _service, Args&&... _args):
	m_service(_service),
	m_instance(std::forward<Args>(_args)...),
	m_prevInstance(m_service.m_handle) {
	
	m_service.m_handle = *m_instance;
	
}

}

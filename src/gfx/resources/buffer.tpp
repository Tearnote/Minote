namespace minote::gfx {

template<typename T>
auto Buffer<T>::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eBuffer, _access);
	
}

template<typename T>
void Buffer<T>::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_buffer(name, *handle, _initial, _final);
	
}

}

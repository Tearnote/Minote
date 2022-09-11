[[vk::binding(0)]] RWStructuredBuffer<uint> b_foo;

[numthreads(64, 1, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	b_foo[_tid.x] = _tid.x;
	//printf("%u/n", _tid.x); // Uncomment for vuk assert
	
}

//Basic Buffer/Layout-less fullscreen triangle vertex shader
vec2 triangleVertex(in int vertID, out vec2 texcoord)
{
	/*
	//See: https://web.archive.org/web/20140719063725/http://www.altdev.co/2011/08/08/interesting-vertex-shader-trick/

	   1
	( 0, 2)
	[-1, 3]   [ 3, 3]
		.
		|`.
		|  `.
		|    `.
		'------`
	   0         2
	( 0, 0)   ( 2, 0)
	[-1,-1]   [ 3,-1]

	ID=0 -> Pos=[-1,-1], Tex=(0,0)
	ID=1 -> Pos=[-1, 3], Tex=(0,2)
	ID=2 -> Pos=[ 3,-1], Tex=(2,0)
	*/

	texcoord.x = (vertID == 2) ?  2.0 :  0.0;
	texcoord.y = (vertID == 1) ?  2.0 :  0.0;

	return texcoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
}

vec2 flipTexCoord(in vec2 tc) {
	return tc * vec2(1.0, -1.0) + vec2(0.0, 1.0);
}

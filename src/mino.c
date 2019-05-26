#include "mino.h"

#include "linmath.h"

vec4 minoColors[MinoSize] = {
	{   0,   0,   0, 0 }, // MinoNone
	{   1,   0,   0, 1 }, // MinoI
	{   1, 0.5,   0, 1 }, // MinoL
	{   1,   1,   0, 1 }, // MinoO
	{   0,   1,   0, 1 }, // MinoZ
	{   0,   1,   1, 1 }, // MinoT
	{   0,   0,   1, 1 }, // MinoJ
	{   1,   0,   1, 1 }, // MinoS
	{ 0.5, 0.5, 0.5, 1 }, // MinoGarbage
	{   1,   1,   1, 1 }  // MinoPending
};

rotationSystem rs = {
	{ // PieceI
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoI,    MinoI,    MinoI,    MinoI    },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoI,    MinoNone },
			{ MinoNone, MinoNone, MinoI,    MinoNone },
			{ MinoNone, MinoNone, MinoI,    MinoNone },
			{ MinoNone, MinoNone, MinoI,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoI,    MinoI,    MinoI,    MinoI    },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoI,    MinoNone, MinoNone },
			{ MinoNone, MinoI,    MinoNone, MinoNone },
			{ MinoNone, MinoI,    MinoNone, MinoNone },
			{ MinoNone, MinoI,    MinoNone, MinoNone }
		}
	},
	{ // PieceL
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoL,    MinoL,    MinoL,    MinoNone },
			{ MinoL,    MinoNone, MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoL,    MinoL,    MinoNone, MinoNone },
			{ MinoNone, MinoL,    MinoNone, MinoNone },
			{ MinoNone, MinoL,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoL,    MinoNone },
			{ MinoL,    MinoL,    MinoL,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoL,    MinoNone, MinoNone },
			{ MinoNone, MinoL,    MinoNone, MinoNone },
			{ MinoNone, MinoL,    MinoL,    MinoNone }
		}
	},
	{ // PieceO
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone },
			{ MinoNone, MinoO,    MinoO,    MinoNone }
		}
	},
	{ // PieceZ
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoZ,    MinoZ,    MinoNone, MinoNone },
			{ MinoNone, MinoZ,    MinoZ,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoZ,    MinoNone },
			{ MinoNone, MinoZ,    MinoZ,    MinoNone },
			{ MinoNone, MinoZ,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoZ,    MinoZ,    MinoNone, MinoNone },
			{ MinoNone, MinoZ,    MinoZ,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoZ,    MinoNone },
			{ MinoNone, MinoZ,    MinoZ,    MinoNone },
			{ MinoNone, MinoZ,    MinoNone, MinoNone }
		}
	},
	{ // PieceT
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoT,    MinoT,    MinoT,    MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone },
			{ MinoT,    MinoT,    MinoNone, MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone },
			{ MinoT,    MinoT,    MinoT,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone },
			{ MinoNone, MinoT,    MinoT,    MinoNone },
			{ MinoNone, MinoT,    MinoNone, MinoNone }
		}
	},
	{ // PieceJ
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoJ,    MinoJ,    MinoJ,    MinoNone },
			{ MinoNone, MinoNone, MinoJ,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoJ,    MinoNone, MinoNone },
			{ MinoNone, MinoJ,    MinoNone, MinoNone },
			{ MinoJ,    MinoJ,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoJ,    MinoNone, MinoNone, MinoNone },
			{ MinoJ,    MinoJ,    MinoJ,    MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoJ,    MinoJ,    MinoNone },
			{ MinoNone, MinoJ,    MinoNone, MinoNone },
			{ MinoNone, MinoJ,    MinoNone, MinoNone }
		}
	},
	{ // PieceS
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoS,    MinoS,    MinoNone },
			{ MinoS,    MinoS,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoS,    MinoNone },
			{ MinoNone, MinoS,    MinoS,    MinoNone },
			{ MinoNone, MinoS,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoS,    MinoS,    MinoNone },
			{ MinoS,    MinoS,    MinoNone, MinoNone }
		},
		{
			{ MinoNone, MinoNone, MinoNone, MinoNone },
			{ MinoNone, MinoNone, MinoS,    MinoNone },
			{ MinoNone, MinoS,    MinoS,    MinoNone },
			{ MinoNone, MinoS,    MinoNone, MinoNone }
		}
	}
};
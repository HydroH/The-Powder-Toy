#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_DOOR PT_DOOR 181
Element_DOOR::Element_DOOR()
{
	Identifier = "DEFAULT_PT_DOOR";
	Name = "DOOR";
	Colour = PIXPACK(0xFA3232);
	MenuVisible = 1;
	MenuSection = SC_POWERED;
	Enabled = 1;
	
	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;
	
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 15;
	
	Weight = 100;
	
	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 164;
	Description = "Allows particles when activated.";
	
	Properties = TYPE_SOLID;
	
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	
	Update = &Element_DOOR::update;
	Graphics = &Element_DOOR::graphics;
}

//#TPT-Directive ElementHeader Element_DOOR static int update(UPDATE_FUNC_ARGS)
int Element_DOOR::update(UPDATE_FUNC_ARGS)
 {
	int r, rx, ry;
	if (parts[i].life>0 && parts[i].life!=10)
		parts[i].life--;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)==PT_SPRK)
				{
					if (parts[r>>8].life>0 && parts[r>>8].life<4)
					{
						if (parts[r>>8].ctype==PT_PSCN)
							parts[i].life = 10;
						else if (parts[r>>8].ctype==PT_NSCN)
							parts[i].life = 9;
					}
				}
				else if ((r&0xFF)==PT_DOOR)
				{
					if (parts[i].life==10&&parts[r>>8].life<10&&parts[r>>8].life>0)
						parts[i].life = 9;
					else if (parts[i].life==0&&parts[r>>8].life==10)
						parts[i].life = 10;
				}
			}
	return 0;
}

//#TPT-Directive ElementHeader Element_DOOR static int graphics(GRAPHICS_FUNC_ARGS)
int Element_DOOR::graphics(GRAPHICS_FUNC_ARGS)

{
	int lifemod = ((cpart->life>10?10:cpart->life)*20);
	*colr -= lifemod;
	*colg += lifemod;
	return 0;
}


Element_DOOR::~Element_DOOR() {}

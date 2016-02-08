#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_ONC PT_ONC 182
Element_ONC::Element_ONC()
{
	Identifier = "DEFAULT_PT_ONC";
	Name = "ONC";
	Colour = PIXPACK(0xF71687);
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
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
	Hardness = 5;
	
	Weight = 100;
	
	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 230;
	Description = "Octanitrocubane. Extremely powerful explosive. Can only be ignited by sparks and the flame of itself.";
	
	Properties = TYPE_SOLID;
	
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	
	Update = &Element_ONC::update;
	
}

//#TPT-Directive ElementHeader Element_ONC static int update(UPDATE_FUNC_ARGS)
int Element_ONC::update(UPDATE_FUNC_ARGS)
 {
	int r, rx, ry;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ( (r&0xFF)==PT_SPRK )
				{
						sim->part_change_type(i,x,y,PT_PLSM);
						parts[r>>8].temp = parts[i].temp = MAX_TEMP;
                        parts[r>>8].tmp = parts[i].tmp2 = 42;
						parts[i].life = rand()%150+60;
						sim->pv[y/CELL][x/CELL] += 256;
				}
				if ( (r&0xFF)==PT_PLSM )
				{
					if (parts[r>>8].tmp2==42)
					{
						sim->part_change_type(i,x,y,PT_PLSM);
						parts[r>>8].temp = parts[i].temp = MAX_TEMP;
                        parts[r>>8].tmp = parts[i].tmp2 = 42;
						parts[i].life = rand()%150+60;
						sim->pv[y/CELL][x/CELL] += 256;
					}
				}
			}
	return 0;
}


Element_ONC::~Element_ONC() {}

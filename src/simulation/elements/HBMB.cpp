#include "simulation/Elements.h"
//#TPT-Directive ElementClass Element_HBMB PT_HBMB 183
Element_HBMB::Element_HBMB()
{
	Identifier = "DEFAULT_PT_HBMB";
	Name = "HBMB";
	Colour = PIXPACK(0xFF7112);
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;
	
	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;
	
	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;
	
	Weight = 30;
	
	Temperature = R_TEMP+0.0f	+273.15f;
	HeatConduct = 255;
	Description = "Heat bomb. Sticks to the first object it touches then heats up particles around.";
	
	Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;
	
	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
	
	Update = &Element_HBMB::update;
	Graphics = &Element_HBMB::graphics;
}

//#TPT-Directive ElementHeader Element_HBMB static int update(UPDATE_FUNC_ARGS)
int Element_HBMB::update(UPDATE_FUNC_ARGS)
 {
	int rx,ry,r;
	if (parts[i].life<=0)
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
			{
				if (BOUNDS_CHECK)
				{
					r = pmap[y+ry][x+rx];
					if(!r)
						continue;
					if((r&0xFF)!=PT_BOMB && (r&0xFF)!=PT_HBMB &&
					   (r&0xFF)!=PT_CLNE && (r&0xFF)!=PT_PCLN &&
					   (r&0xFF)!=PT_DMND && (r&0xFF)!=PT_ADMN && ((r&0xFF)!=PT_SPRK && parts[r>>8].ctype!=PT_ADMN))
					{
						parts[i].life=75;
						break;
					}
				}
			}
	}
	if (parts[i].life>0)
	{
		parts[i].temp=MAX_TEMP;
		for (rx=-10; rx<11; rx++)
			for (ry=-10; ry<11; ry++)
			{
				if (BOUNDS_CHECK && (rx*rx+ry*ry<=100))
				{
					r = pmap[y+ry][x+rx];
					if(!r)
						continue;
					if ((sim->elements[(r&0xFF)].HeatConduct!=0) && ((rx!=0) || (ry!=0)))
					{
						if (parts[r>>8].temp+ (parts[r>>8].temp*0.3f*(1/sqrt(rx*rx+ry*ry))<=MAX_TEMP))
				    	{
					    	parts[r>>8].temp+=parts[r>>8].temp*0.3f*(1/sqrt(rx*rx+ry*ry));
					    }
						else
							parts[r>>8].temp=MAX_TEMP;
					}
				}
			}
	}
	return 0;
}


//#TPT-Directive ElementHeader Element_HBMB static int graphics(GRAPHICS_FUNC_ARGS)
int Element_HBMB::graphics(GRAPHICS_FUNC_ARGS)

{
	if (cpart->life <= 0) {
		*pixel_mode |= PMODE_FLARE;
	}
	else
	{
		*pixel_mode |= PMODE_SPARK;
	}
	return 0;
}


Element_HBMB::~Element_HBMB() {}

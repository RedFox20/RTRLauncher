#include "RomeAPI.h"


float RTW::UnitScale()
{
	int pSettings = *(int*)(0x273845C);
	return *(float*)(pSettings + 0x2A14);
}
void RTW::UnitScale(float scale)
{
	int pSettings = *(int*)(0x273845C);
	*(float*)(pSettings + 0x2A14) = scale;
}
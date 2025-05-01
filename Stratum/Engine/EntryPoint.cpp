#include "Platform.h"
#include "znmsp.h"

DLLEXPORT int CheckCPUCapabilities()
{
	//Platform::ShowSystemMessageBox("Stratum-Editor.exe - Error del sistema", "El programa no puede iniciarse porque falta MSVCP140.dll en el equipo. Reinstale el programa para corregir este problema", Platform::MessageBoxButtons::OK, Platform::Error);
	if (!Platform::DoesCpuSupportSSE42()) {
		Platform::ShowSystemMessageBox("Fatal Error Loading Engine", "A CPU with the SSE 4.2 Instruction set is required", Platform::OK, Platform::Error);
		return -1;
	}
	return 0;
}
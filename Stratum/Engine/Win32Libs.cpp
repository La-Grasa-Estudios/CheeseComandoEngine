#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "zlibstatic.lib")

#pragma comment(lib, "av/avcodec.lib")
#pragma comment(lib, "av/avdevice.lib")
#pragma comment(lib, "av/avfilter.lib")
#pragma comment(lib, "av/avformat.lib")
#pragma comment(lib, "av/avutil.lib")
#pragma comment(lib, "av/swresample.lib")
#pragma comment(lib, "av/swscale.lib")

#ifdef _DEBUG
#pragma comment(lib, "db/nvrhi.lib")
#pragma comment(lib, "db/nvrhi_d3d12.lib")
#pragma comment(lib, "db/SDL3.lib")
#else
#pragma comment(lib, "rel/nvrhi.lib")
#pragma comment(lib, "rel/nvrhi_d3d12.lib")
#pragma comment(lib, "rel/SDL3.lib")
#endif
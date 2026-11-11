#include <MainFormPresenterFactory.hpp>

#ifdef _MSC_VER
#include <windows.h>

// nana is per-monitor DPI aware but lays out widgets in unscaled pixels, so text overlaps above 100% scaling.
// Make this thread's windows DPI unaware instead, with GDI scaling where available (Windows 10 1809+).
static void use_gdi_scaling()
{
    typedef HANDLE(WINAPI* set_context_t)(HANDLE);
    auto set_context = reinterpret_cast<set_context_t>(::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
    if (set_context == nullptr)
        return;

    const auto unaware_gdi_scaled = reinterpret_cast<HANDLE>(-5);   // DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED
    const auto unaware = reinterpret_cast<HANDLE>(-1);              // DPI_AWARENESS_CONTEXT_UNAWARE
    if (set_context(unaware_gdi_scaled) == nullptr)
        set_context(unaware);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
#ifdef _MSC_VER
    use_gdi_scaling();
#endif

    auto program = MainFormPresenterFactory::Create();

    program->Run();

    return 0;
}

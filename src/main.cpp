#include <MainFormPresenterFactory.hpp>

#ifdef _MSC_VER
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
    auto program = MainFormPresenterFactory::Create();

    program->Run();

    return 0;
}

#include <fstream>
#include <iostream>

#include "byteswap.hpp"

#include "MovAvInfoDetect.hpp"

#include "MainForm.hpp"

#include "MainFormPanel.hpp"

#include "FileSelectionUserControl.hpp"
#include "DebugTabUserControl.hpp"
#include "AboutTabUserControl.hpp"
#include <FileSelectionPresenter.hpp>
#include <MainFormPanelPresenter.hpp>
#include <InputFileInfoPresenter.hpp>
#include <InputFileInfoUserControl.hpp>

#ifdef _MSC_VER
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
    MainForm form;

    MainFormPanel control;
    form.UserControl__set(control);

    FileSelectionUserControl fsuc0;
    DebugTabUserControl dtuc0;
    AboutTabUserControl atuc0;
    InputFileInfoUserControl ifiuc0;

    control.AddTab("File Selection", fsuc0);
    control.AddTab("Info", ifiuc0);
    control.AddTab("Debug", dtuc0);
    control.AddTab("About", atuc0);

    FileSelectionPresenter fsp0(fsuc0, dtuc0.Console());
    InputFileInfoPresenter ifip0(ifiuc0, dtuc0.Console());
    MainFormPanelPresenter mfpp0(fsp0, ifip0, control, dtuc0.Console());
    

    form.Show();

    return 0;
}

#pragma once

#include <string>
#include <vector>

#include <Event.h>
#include <IUserControl.hpp>

class IBatchConversionListView : public IUserControl
{
public:
    DECLARE_EVENT(void, std::vector<std::string>&) EventInputFilesAdded;
    DECLARE_EVENT(void, std::string, bool) EventInputFileEnabled;
    DECLARE_EVENT(void, std::string) EventInputFileSelected;

    virtual void Lock() = 0;
    virtual void Unlock() = 0;

    virtual void AddPath(std::string path, std::string type, bool checked) = 0;
    virtual void RemovePath(std::string path) = 0;

    virtual void SetPathEnabled(std::string path, bool checked) = 0;
    virtual void ChangeInputFilePercent(std::string path, unsigned int percent) = 0;
    virtual void ChangeInputFileColor(std::string path, unsigned int r, unsigned int g, unsigned int b, double alpha) = 0;
};

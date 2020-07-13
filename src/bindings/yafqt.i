#if defined(SWIGRUBY)
  %module yafqt_ruby
#else
  %module yafqt
#endif

%include "cpointer.i"
%pointer_functions(float, floatp);
%pointer_functions(int, intp);
%pointer_functions(unsigned int, uintp);

%include "carrays.i"
%array_functions(float, floatArray);

%include "std_string.i"

%{
#include <yafray_constants.h>
#include <gui/yafqtapi.h>
%}

struct Settings {
    bool autoSave;
    bool autoSaveAlpha;
    bool closeAfterFinish;
    std::string fileName;
};

void initGui();
int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX, int bStartY, Settings settings);


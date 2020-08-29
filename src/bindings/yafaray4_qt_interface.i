#if defined(SWIGRUBY)
  %module yafaray4_qt_interface_ruby
#else
  %module yafaray4_qt_interface
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
    bool auto_save_;
    bool auto_save_alpha_;
    bool close_after_finish_;
    std::string file_name_;
};

void initGui__();
int createRenderWidget__(yafaray4::Interface *interf, int xsize, int ysize, int b_start_x, int b_start_y, Settings settings);


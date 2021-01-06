/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

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
#include "constants.h"
#include "gui/interface_qt.h"
%}

struct Settings {
    bool auto_save_;
    bool auto_save_alpha_;
    bool close_after_finish_;
    std::string file_name_;
};

void initGui_global();
int createRenderWidget_global(yafaray4::Interface *interf, int xsize, int ysize, int b_start_x, int b_start_y, Settings settings);


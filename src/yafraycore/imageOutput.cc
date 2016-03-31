/****************************************************************************
 *
 * 		imageOutput.cc: generic color output based on imageHandlers
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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
 *      
 */

#include <core_api/renderpasses.h>
#include <yafraycore/imageOutput.h>
#include <sstream>
#include <iomanip>

__BEGIN_YAFRAY

imageOutput_t::imageOutput_t(imageHandler_t * handle, const std::string &name, int bx, int by) : image(handle), fname(name), bX(bx), bY(by)
{
	//empty
}

imageOutput_t::imageOutput_t()
{
	image = nullptr;
}

imageOutput_t::~imageOutput_t()
{
	image = nullptr;
}

bool imageOutput_t::putPixel(int numView, int x, int y, const renderPasses_t *renderPasses, const std::vector<colorA_t> &colExtPasses, bool alpha)
{
	if(image)
	{
		for(size_t idx = 0; idx < colExtPasses.size(); ++idx)
		{
			colorA_t col(0.f);
			col.set(colExtPasses[idx].R, colExtPasses[idx].G, colExtPasses[idx].B, ( (alpha || idx > 0) ? colExtPasses[idx].A : 1.f ) );
			image->putPixel(x + bX , y + bY, col, idx);
		}
	}
	return true;
}

void imageOutput_t::flush(int numView, const renderPasses_t *renderPasses)
{
    std::string fnamePass, path, name, base_name, ext, num_view="";

    size_t sep = fname.find_last_of("\\/");
    if (sep != std::string::npos)
        name = fname.substr(sep + 1, fname.size() - sep - 1);

    path = fname.substr(0, sep+1);

    if(path == "") name = fname;

    size_t dot = name.find_last_of(".");

    if (dot != std::string::npos)
    {
        base_name = name.substr(0, dot);
        ext  = name.substr(dot, name.size() - dot);
    }
    else
    {
        base_name = name;
        ext  = "";
    }
                
    std::string view_name = renderPasses->view_names.at(numView);
    
    if(view_name != "") base_name += " (view " + view_name + ")";

    if(image)
    {
        if(image->isMultiLayer())
        {
            if(numView == 0) image->saveToFile(fname, 0); //This should not be necessary but Blender API seems to be limited and the API "load_from_file" function does not work (yet) with multilayer EXR, so I have to generate this extra combined pass file so it's displayed in the Blender window.
         
            fnamePass = path + base_name + " [" + "multilayer" + "]"+ ext;
         
            image->saveToFileMultiChannel(fnamePass, renderPasses);
	    yafLog.setImagePath(fnamePass);		//to show the image in the HTML log output
        }
        else
        {
            for(int idx = 0; idx < renderPasses->extPassesSize(); ++idx)
            {
                std::string passName = renderPasses->intPassTypeStringFromType(renderPasses->intPassTypeFromExtPassIndex(idx));
                
                if(numView == 0 && idx == 0)
		{
		    image->saveToFile(fname, idx);	//default image filename, when not using views nor passes and for reloading into Blender
		    yafLog.setImagePath(fname);		//to show the image in the HTML log output
		}
		
		if(passName != "not found" && (renderPasses->extPassesSize()>=2 || renderPasses->view_names.size() >= 2))
                {
                    fnamePass = path + base_name + " [pass " + passName + "]"+ ext;
                    image->saveToFile(fnamePass, idx);
		    if(idx == 0) yafLog.setImagePath(fnamePass);	//to show the image in the HTML log output
                }
            }
        }
    }
    
    std::string fLogName = path + base_name + "_log.txt";
    yafLog.saveTxtLog(fLogName);
    
    std::string fLogHtmlName = path + base_name + "_log.html";
    yafLog.saveHtmlLog(fLogHtmlName);
    
    yafLog.clearMemoryLog();
}

__END_YAFRAY


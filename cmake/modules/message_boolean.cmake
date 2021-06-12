#****************************************************************************
#      This is part of the libYafaRay package
#
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2.1 of the License, or (at your option) any later version.
#
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
#
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library; if not, write to the Free Software
#      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

include_guard()

# Function to show a message with an appended text that depends on a boolean variable (for example: "with PNG: yes" (or no))
function(message_boolean MESSAGE BOOL_VAR TEXT_TRUE TEXT_FALSE)
	if(${BOOL_VAR})
		message("${MESSAGE}: ${TEXT_TRUE}")
	else()
		message("${MESSAGE}: ${TEXT_FALSE}")
	endif()
endfunction()
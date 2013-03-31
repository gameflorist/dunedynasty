/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <assert.h>

#include "fmopl.h"

#include "opl_dosbox.h"
#include "opl_mame.h"

namespace OPL {


bool OPL::_hasInstance = false;

} // End of namespace OPL

void OPLDestroy(FM_OPL *OPL) {
	delete OPL;
}

void OPLResetChip(FM_OPL *OPL) {
	OPL->reset();
}

void OPLWrite(FM_OPL *OPL, int a, int v) {
	OPL->write(a, v);
}

unsigned char OPLRead(FM_OPL *OPL, int a) {
	return OPL->read(a);
}

void OPLWriteReg(FM_OPL *OPL, int r, int v) {
	OPL->writeReg(r, v);
}

void YM3812UpdateOne(FM_OPL *OPL, int16_t *buffer, int length) {
	OPL->readBuffer(buffer, length);
}

FM_OPL *makeAdLibOPL(int rate, bool bMAME) {
	FM_OPL *opl	;
	if(bMAME) {
		opl = new OPL::MAME::OPL();
	} else {
		opl = new OPL::DOSBox::OPL(OPL::Config::kOpl2);
	}

	if (opl) {
		if (!opl->init(rate)) {
			delete opl;
			opl = 0;
		}
	}

	return opl;
}

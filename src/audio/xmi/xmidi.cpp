/*
Copyright (C) 2000, 2001  Ryan Nunn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* DSW: stripped some unused code. */

// XMIDI/MIDI Converter/Loader

#include <cassert>
#include <stdio.h>
#include <iostream>
//#include "randgen.h"
#include "xmidi.h"

using namespace std;

#define TRUE 1
#define FALSE 0

// This is used to correct incorrect patch, vol and pan changes in midi files
// The bias is just a value to used to work out if a vol and pan belong with a
// patch change. This is to ensure that the default state of a midi file is with
// the tracks centred, unless the first patch change says otherwise.
#define PATCH_VOL_PAN_BIAS	5

// Constructor
XMIDI::XMIDI(DataSource *source, int pconvert) : events(NULL),timing(NULL),
						fixed(NULL)
{
	int i = 16;
	while (i--) bank127[i] = 0;

	ExtractTracks (source);
}

XMIDI::~XMIDI()
{
	if (events)
	{
		for (int i=0; i < info.tracks; i++)
			DeleteEventList (events[i]);
		delete [] events;
	}
	if (timing) delete [] timing;
	if (fixed) delete [] fixed;
}

int XMIDI::retrieve (unsigned int track, DataSource *dest)
{
	int len = 0;

	if (!events)
	{
		cerr << "No midi data in loaded." << endl;
		return 0;
	}

	// Convert type 1 midi's to type 0
	assert(info.type == 0);

	if (track >= info.tracks)
	{
		cerr << "Can't retrieve MIDI data, track out of range" << endl;
		return 0;
	}

	// And fix the midis if they are broken
	if (!fixed[track])
	{
		list = events[track];
		MovePatchVolAndPan ();
		fixed[track] = TRUE;
		events[track] = list;
	}

	// This is so if using buffer datasource, the caller can know how big to make the buffer
	if (!dest)
	{
		// Header is 14 bytes long and add the rest as well
		len = ConvertListToMTrk (NULL, events[track]);
		return 14 + len;
	}

	dest->write1 ('M');
	dest->write1 ('T');
	dest->write1 ('h');
	dest->write1 ('d');

	dest->write4high (6);

	dest->write2high (0);
	dest->write2high (1);
	dest->write2high (timing[track]);

	len = ConvertListToMTrk (dest, events[track]);

	return len + 14;
}

void XMIDI::DeleteEventList (midi_event *mlist)
{
	midi_event *event;
	midi_event *next;

	next = mlist;
	event = mlist;

	while ((event = next))
	{
		next = event->next;
		delete event;
	}
}

// Sets current to the new event and updates list
void XMIDI::CreateNewEvent (int time)
{
	if (!list)
	{
		list = current = new midi_event;
		current->next = NULL;
		if (time < 0)
			current->time = 0;
		else
			current->time = time;
		current->buffer = NULL;
		current->len = 0;
		return;
	}

	if (time < 0)
	{
		midi_event *event = new midi_event;
		event->next = list;
		list = current = event;
		current->time = 0;
		current->buffer = NULL;
		current->len = 0;
		return;
	}

	if (current->time > time)
		current = list;

	while (current->next)
	{
		if (current->next->time > time)
		{
			midi_event *event = new midi_event;

			event->next = current->next;
			current->next = event;
			current = event;
			current->time = time;
			current->buffer = NULL;
			current->len = 0;
			return;
		}

		current = current->next;
	}

	current->next = new midi_event;
	current = current->next;
	current->next = NULL;
	current->time = time;
	current->buffer = NULL;
	current->len = 0;
}


// Conventional Variable Length Quantity
int XMIDI::GetVLQ (DataSource *source, unsigned int &quant)
{
	int i;
	quant = 0;
	unsigned int data;

	for (i = 0; i < 4; i++)
	{
		data = source->read1();
		quant <<= 7;
		quant |= data & 0x7F;

		if (!(data & 0x80))
		{
			i++;
			break;
		}

	}
	return i;
}

// XMIDI Delta Variable Length Quantity
int XMIDI::GetVLQ2 (DataSource *source, unsigned int &quant)
{
	int i;
	quant = 0;
	int data = 0;

	for (i = 0; i < 4; i++)
	{
		data = source->read1();
		if (data & 0x80)
		{
			source->skip(-1);
			break;
		}
		quant += data;
	}
	return i;
}

int XMIDI::PutVLQ(DataSource *dest, unsigned int value)
{
	int buffer;
	int i = 1;
	buffer = value & 0x7F;
	while (value >>= 7)
	{
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
		i++;
	}
	if (!dest) return i;
	for (int j = 0; j < i; j++)
	{
		dest->write1(buffer & 0xFF);
		buffer >>= 8;
	}

	return i;
}

// MovePatchVolAndPan
//
// This little function attempts to correct errors in midi files
// that relate to patch, volume and pan changing
void XMIDI::MovePatchVolAndPan (int channel)
{
	if (channel == -1)
	{
		for (int i = 0; i < 16; i++)
			MovePatchVolAndPan (i);

		return;
	}

	midi_event *patch = NULL;
	midi_event *vol = NULL;
	midi_event *pan = NULL;
	midi_event *bank = NULL;
	midi_event *temp;

	for (current = list; current; )
	{
		if (!patch && (current->status >> 4) == 0xC && (current->status & 0xF) == channel)
			patch = current;
		else if (!vol && (current->status >> 4) == 0xB && current->data[0] == 7 && (current->status & 0xF) == channel)
			vol = current;
		else if (!pan && (current->status >> 4) == 0xB && current->data[0] == 10 && (current->status & 0xF) == channel)
			pan = current;
		else if (!bank && (current->status >> 4) == 0xB && current->data[0] == 0 && (current->status & 0xF) == channel)
			bank = current;

		if (pan && vol && patch) break;

		if (current) current = current->next;
		else current = list;
	}

	// Got no patch change, return and don't try fixing it
	if (!patch) return;


	// Copy Patch Change Event
	temp = patch;
	patch = new midi_event;
	patch->time = temp->time;
	patch->status = channel + 0xC0;
	patch->len = 0;
	patch->buffer = NULL;
	patch->data[0] = temp->data[0];


	// Copy Volume
	if (vol && (vol->time > patch->time+PATCH_VOL_PAN_BIAS || vol->time < patch->time-PATCH_VOL_PAN_BIAS))
		vol = NULL;

	temp = vol;
	vol = new midi_event;
	vol->status = channel + 0xB0;
	vol->data[0] = 7;
	vol->len = 0;
	vol->buffer = NULL;

	if (!temp)
		vol->data[1] = 64;
	else
		vol->data[1] = temp->data[1];


	// Copy Bank
	if (bank && (bank->time > patch->time+PATCH_VOL_PAN_BIAS || bank->time < patch->time-PATCH_VOL_PAN_BIAS))
		bank = NULL;

	temp = bank;

	bank = new midi_event;
	bank->status = channel + 0xB0;
	bank->data[0] = 0;
	bank->len = 0;
	bank->buffer = NULL;

	if (!temp)
		bank->data[1] = 0;
	else
		bank->data[1] = temp->data[1];

	// Copy Pan
	if (pan && (pan->time > patch->time+PATCH_VOL_PAN_BIAS || pan->time < patch->time-PATCH_VOL_PAN_BIAS))
		pan = NULL;

	temp = pan;
	pan = new midi_event;
	pan->status = channel + 0xB0;
	pan->data[0] = 10;
	pan->len = 0;
	pan->buffer = NULL;

	if (!temp)
		pan->data[1] = 64;
	else
		pan->data[1] = temp->data[1];


	vol->time = 0;
	pan->time = 0;
	patch->time = 0;
	bank->time = 0;

	bank->next = vol;
	vol->next = pan;
	pan->next = patch;
	patch->next = list;
	list = bank;
}

// Converts Events
//
// Source is at the first data byte
// size 1 is single data byte
// size 2 is dual data byte
// size 3 is XMI Note on
// Returns bytes converted

int XMIDI::ConvertEvent (const int time, const unsigned char status, DataSource *source, const int size)
{
	unsigned int	delta = 0;
	int	data;

	data = source->read1();


	// Bank changes are handled here
	if ((status >> 4) == 0xB && data == 0)
	{
		data = source->read1();

		bank127[status&0xF] = FALSE;

		CreateNewEvent (time);
		current->status = status;
		current->data[0] = 0;
		current->data[1] = data;

		return 2;
	}

	CreateNewEvent (time);
	current->status = status;

	current->data[0] = data;

	if (size == 1)
		return 1;

	current->data[1] = source->read1();

	if (size == 2)
		return 2;

	// XMI Note On handling
	midi_event *prev = current;
	int i = GetVLQ (source, delta);
	CreateNewEvent (time+delta*3);

	current->status = status;
	current->data[0] = data;
	current->data[1] = 0;
	current = prev;

	return i + 2;
}

// Simple routine to convert system messages
int XMIDI::ConvertSystemMessage (const int time, const unsigned char status, DataSource *source)
{
	int i=0;

	CreateNewEvent (time);
	current->status = status;

	// Handling of Meta events
	if (status == 0xFF)
	{
		current->data[0] = source->read1();
		i++;
	}

	i += GetVLQ (source, current->len);

	if (!current->len) return i;

	current->buffer = new unsigned char[current->len];

	source->read ((char *) current->buffer, current->len);

	return i+current->len;
}

// XMIDI and Midi to List
// Returns XMIDI PPQN
int XMIDI::ConvertFiletoList (DataSource *source, const BOOL is_xmi)
{
	int 		time = 0;
	unsigned int 	data;
	int		end = 0;
	int		tempo = 500000;
	int		tempo_set = 0;
	unsigned int	status = 0;
	int		play_size = 2;
	unsigned int	file_size = source->getSize();

	assert(is_xmi);
	if (is_xmi) play_size = 3;

	while (!end && source->getPos() < file_size)
	{
		{
			GetVLQ2 (source, data);
			time += data*3;

			status = source->read1();
		}

		switch (status >> 4)
		{
			case MIDI_STATUS_NOTE_ON:
			ConvertEvent (time, status, source, play_size);
			break;

			// 2 byte data
			case MIDI_STATUS_NOTE_OFF:
			case MIDI_STATUS_AFTERTOUCH:
			case MIDI_STATUS_CONTROLLER:
			case MIDI_STATUS_PITCH_WHEEL:
			ConvertEvent (time, status, source, 2);
			break;


			// 1 byte data
			case MIDI_STATUS_PROG_CHANGE:
			case MIDI_STATUS_PRESSURE:
			ConvertEvent (time, status, source, 1);
			break;


			case MIDI_STATUS_SYSEX:
			if (status == 0xFF)
			{
				int	pos = source->getPos();
				data = source->read1();

				if (data == 0x2F) // End
					end = 1;
				else if (data == 0x51 && !tempo_set) // Tempo. Need it for PPQN
				{
					source->skip(1);
					tempo = source->read1() << 16;
					tempo += source->read1() << 8;
					tempo += source->read1();
					tempo *= 3;
					tempo_set = 1;
				}
				else if (data == 0x51 && tempo_set && is_xmi) // Skip any other tempo changes
				{
					GetVLQ (source, data);
					source->skip(data);
					break;
				}

				source->seek (pos);
			}
			ConvertSystemMessage (time, status, source);
			break;

			default:
			break;
		}

	}
	return (tempo*3)/25000;
}

// Converts and event list to a MTrk
// Returns bytes of the array
// buf can be NULL
unsigned int XMIDI::ConvertListToMTrk (DataSource *dest, midi_event *mlist)
{
	int time = 0;
	midi_event	*event;
	unsigned int	delta;
	unsigned char	last_status = 0;
	unsigned int 	i = 8;
	unsigned int 	j;
	unsigned int	size_pos=0;
	BOOL end = FALSE;

	if (dest)
	{
		dest->write1('M');
		dest->write1('T');
		dest->write1('r');
		dest->write1('k');

		size_pos = dest->getPos();
		dest->skip(4);
	}

	for (event = mlist; event && !end; event=event->next)
	{
		delta = (event->time - time);
		time = event->time;

		i += PutVLQ (dest, delta);

		if ((event->status != last_status) || (event->status >= 0xF0))
		{
			if (dest) dest->write1(event->status);
			i++;
		}

		last_status = event->status;

		switch (event->status >> 4)
		{
			// 2 bytes data
			// Note off, Note on, Aftertouch, Controller and Pitch Wheel
			case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
			if (dest)
			{
				dest->write1(event->data[0]);
				dest->write1(event->data[1]);
			}
			i += 2;
			break;


			// 1 bytes data
			// Program Change and Channel Pressure
			case 0xC: case 0xD:
			if (dest) dest->write1(event->data[0]);
			i++;
			break;


			// Variable length
			// SysEx
			case 0xF:
			if (event->status == 0xFF)
			{
				if (event->data[0] == 0x2f) end = TRUE;
				if (dest) dest->write1(event->data[0]);
				i++;
			}

			i += PutVLQ (dest, event->len);

			if (event->len)
			{
				for (j = 0; j < event->len; j++)
				{
					if (dest) dest->write1(event->buffer[j]);
					i++;
				}
			}

			break;


			// Never occur
			default:
			cerr << "Not supposed to see this" << endl;
			break;
		}
	}

	if (dest)
	{
		int cur_pos = dest->getPos();
		dest->seek (size_pos);
		dest->write4high (i-8);
		dest->seek (cur_pos);
	}
	return i;
}

// Assumes correct xmidi
int XMIDI::ExtractTracksFromXmi (DataSource *source)
{
	int		num = 0;
	signed short	ppqn;
	unsigned int		len = 0;
	char		buf[32];

	while (source->getPos() < source->getSize() && num != info.tracks)
	{
		// Read first 4 bytes of name
		source->read (buf, 4);
		len = source->read4high();

		// Skip the FORM entries
		if (!memcmp(buf,"FORM",4))
		{
			source->skip (4);
			source->read (buf, 4);
			len = source->read4high();
		}

		if (memcmp(buf,"EVNT",4))
		{
			source->skip ((len+1)&~1);
			continue;
		}

		list = NULL;
		int begin = source->getPos ();

		// Convert it
		if (!(ppqn = ConvertFiletoList (source, TRUE)))
		{
			cerr << "Unable to convert data" << endl;
			break;
		}
		timing[num] = ppqn;
		events[num] = list;

		// Increment Counter
		num++;

		// go to start of next track
		source->seek (begin + ((len+1)&~1));
	}


	// Return how many were converted
	return num;
}

int XMIDI::ExtractTracks (DataSource *source)
{
	unsigned int		i = 0;
	int		start;
	unsigned int		len;
	unsigned int		chunk_len;
	int 		count;
	char		buf[32];

	// Read first 4 bytes of header
	source->read (buf, 4);

	// Could be XMIDI
	if (!memcmp (buf, "FORM", 4))
	{
		// Read length of
		len = source->read4high();

		start = source->getPos();

		// Read 4 bytes of type
		source->read (buf, 4);

		// XDIRless XMIDI, we can handle them here.
		if (!memcmp (buf, "XMID", 4))
		{
			cerr << "Warning: XMIDI doesn't have XDIR" << endl;
			info.tracks = 1;

		} // Not an XMIDI that we recognise
		else if (memcmp (buf, "XDIR", 4))
		{
			cerr << "Not a recognised XMID" << endl;
			return 0;

		} // Seems Valid
		else
		{
			info.tracks = 0;

			for (i = 4; i < len; i++)
			{
				// Read 4 bytes of type
				source->read (buf, 4);

				// Read length of chunk
				chunk_len = source->read4high();

				// Add eight bytes
				i+=8;

				if (memcmp (buf, "INFO", 4))
				{
					// Must allign
					source->skip((chunk_len+1)&~1);
					i+= (chunk_len+1)&~1;
					continue;
				}

				// Must be at least 2 bytes long
				if (chunk_len < 2)
					break;

				info.tracks = source->read2();
				break;
			}

			// Didn't get to fill the header
			if (info.tracks == 0)
			{
				cerr << "Not a valid XMID" << endl;
				return 0;
			}

			// Ok now to start part 2
			// Goto the right place
			source->seek (start+((len+1)&~1));

			// Read 4 bytes of type
			source->read (buf, 4);

			// Not an XMID
			if (memcmp (buf, "CAT ", 4))
			{
				cerr << "Not a recognised XMID (" << buf[0] << buf[1] << buf[2] << buf[3] << ") should be (CAT )" << endl;
				return 0;
			}

			// Now read length of this track
			len = source->read4high();

			// Read 4 bytes of type
			source->read (buf, 4);

			// Not an XMID
			if (memcmp (buf, "XMID", 4))
			{
				cerr << "Not a recognised XMID (" << buf[0] << buf[1] << buf[2] << buf[3] << ") should be (XMID)" << endl;
				return 0;
			}

		}

		// Ok it's an XMID, so pass it to the ExtractCode

		events = new midi_event *[info.tracks];
		timing = new short[info.tracks];
		fixed = new BOOL[info.tracks];
		info.type = 0;

		for (i = 0; i < info.tracks; i++)
		{
			events[i] = NULL;
			fixed[i] = FALSE;
		}

		count = ExtractTracksFromXmi (source);

		if (count != info.tracks)
		{
			cerr << "Error: unable to extract all (" << info.tracks << ") tracks specified from XMIDI. Only ("<< count << ")" << endl;

			for (i = 0; i < info.tracks; i++)
				DeleteEventList (events[i]);

			delete [] events;
			delete [] timing;

			return 0;
		}

		return 1;
	}

	return 0;
}

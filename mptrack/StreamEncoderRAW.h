/*
 * StreamEncoderRAW.h
 * ------------------
 * Purpose: Exporting streamed music files.
 * Notes  : none
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "StreamEncoder.h"


OPENMPT_NAMESPACE_BEGIN

	
class RawStreamWriter;


class RAWEncoder : public EncoderFactoryBase
{

	friend class RawStreamWriter;

public:

	std::unique_ptr<IAudioStreamEncoder> ConstructStreamEncoder(std::ostream &file) const;
	bool IsAvailable() const;

public:

	RAWEncoder();
	virtual ~RAWEncoder();

};


OPENMPT_NAMESPACE_END
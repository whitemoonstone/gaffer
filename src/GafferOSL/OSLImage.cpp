//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2013, John Haddon. All rights reserved.
//  Copyright (c) 2015, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "GafferOSL/OSLImage.h"

#include "GafferOSL/ClosurePlug.h"
#include "GafferOSL/OSLShader.h"
#include "GafferOSL/ShadingEngine.h"

#include "Gaffer/Context.h"
#include "Gaffer/NameValuePlug.h"
#include "Gaffer/ScriptNode.h"
#include "Gaffer/StringPlug.h"
#include "Gaffer/UndoScope.h"

#include "IECore/CompoundData.h"
#include "IECore/MessageHandler.h"

#include "boost/bind.hpp"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace Gaffer;
using namespace GafferImage;
using namespace GafferOSL;

GAFFER_NODE_DEFINE_TYPE( OSLImage );

size_t OSLImage::g_firstPlugIndex = 0;

OSLImage::OSLImage( const std::string &name )
	:	ImageProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new GafferImage::FormatPlug( "defaultFormat" ) );
	addChild( new GafferScene::ShaderPlug( "__shader", Plug::In, Plug::Default & ~Plug::Serialisable ) );

	addChild( new Gaffer::ObjectPlug( "__shading", Gaffer::Plug::Out, new CompoundData() ) );

	addChild( new Plug( "channels", Plug::In, Plug::Default & ~Plug::AcceptsInputs ) );
	addChild( new OSLCode( "__oslCode" ) );
	addChild( new GafferImage::Constant( "__defaultConstant" ) );
	addChild( new GafferImage::ImagePlug( "__defaultIn", Plug::In, Plug::Default & ~Plug::Serialisable ) );
	shaderPlug()->setInput( oslCode()->outPlug() );
	defaultConstant()->formatPlug()->setInput( defaultFormatPlug() );
	defaultInPlug()->setInput( defaultConstant()->outPlug() );

	channelsPlug()->childAddedSignal().connect( boost::bind( &OSLImage::channelsAdded, this, ::_1, ::_2 ) );
	channelsPlug()->childRemovedSignal().connect( boost::bind( &OSLImage::channelsRemoved, this, ::_1, ::_2 ) );

	// We don't ever want to change these, so we make pass-through connections.
	outPlug()->metadataPlug()->setInput( inPlug()->metadataPlug() );
	outPlug()->deepPlug()->setInput( inPlug()->deepPlug() );
	outPlug()->sampleOffsetsPlug()->setInput( inPlug()->sampleOffsetsPlug() );
}

OSLImage::~OSLImage()
{
}

GafferImage::FormatPlug *OSLImage::defaultFormatPlug()
{
	return getChild<GafferImage::FormatPlug>( g_firstPlugIndex );
}

const GafferImage::FormatPlug *OSLImage::defaultFormatPlug() const
{
	return getChild<GafferImage::FormatPlug>( g_firstPlugIndex );
}

GafferScene::ShaderPlug *OSLImage::shaderPlug()
{
	return getChild<GafferScene::ShaderPlug>( g_firstPlugIndex + 1 );
}

const GafferScene::ShaderPlug *OSLImage::shaderPlug() const
{
	return getChild<GafferScene::ShaderPlug>( g_firstPlugIndex + 1 );
}

Gaffer::ObjectPlug *OSLImage::shadingPlug()
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::ObjectPlug *OSLImage::shadingPlug() const
{
	return getChild<ObjectPlug>( g_firstPlugIndex + 2 );
}

Gaffer::Plug *OSLImage::channelsPlug()
{
	return getChild<Gaffer::Plug>( g_firstPlugIndex + 3 );
}

const Gaffer::Plug *OSLImage::channelsPlug() const
{
	return getChild<Gaffer::Plug>( g_firstPlugIndex + 3 );
}

GafferOSL::OSLCode *OSLImage::oslCode()
{
	return getChild<GafferOSL::OSLCode>( g_firstPlugIndex + 4 );
}

const GafferOSL::OSLCode *OSLImage::oslCode() const
{
	return getChild<GafferOSL::OSLCode>( g_firstPlugIndex + 4 );
}

GafferImage::Constant *OSLImage::defaultConstant()
{
	return getChild<GafferImage::Constant>( g_firstPlugIndex + 5 );
}

const GafferImage::Constant *OSLImage::defaultConstant() const
{
	return getChild<GafferImage::Constant>( g_firstPlugIndex + 5 );
}

GafferImage::ImagePlug *OSLImage::defaultInPlug()
{
	return getChild<GafferImage::ImagePlug>( g_firstPlugIndex + 6 );
}

const GafferImage::ImagePlug *OSLImage::defaultInPlug() const
{
	return getChild<GafferImage::ImagePlug>( g_firstPlugIndex + 6 );
}

const GafferImage::ImagePlug *OSLImage::defaultedInPlug() const
{
	if( inPlug()->getInput() )
	{
		return inPlug();
	}
	else
	{
		return defaultInPlug();
	}
}


void OSLImage::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	ImageProcessor::affects( input, outputs );

	if(
		input == shaderPlug() ||
		input == inPlug()->formatPlug() ||
		input == defaultInPlug()->formatPlug() ||
		input == inPlug()->channelNamesPlug() ||
		input == inPlug()->channelDataPlug() ||
		input == inPlug()->deepPlug() ||
		input == inPlug()->sampleOffsetsPlug()
	)
	{
		outputs.push_back( shadingPlug() );
	}
	else if( input == shadingPlug() )
	{
		outputs.push_back( outPlug()->channelNamesPlug() );
		outputs.push_back( outPlug()->channelDataPlug()	);
	}

	if(
		input == inPlug()->formatPlug() ||
		input == defaultInPlug()->formatPlug()
	)
	{
		outputs.push_back( outPlug()->formatPlug() );
	}

	if(
		input == inPlug()->dataWindowPlug() ||
		input == defaultInPlug()->dataWindowPlug()
	)
	{
		outputs.push_back( outPlug()->dataWindowPlug() );
	}
}

bool OSLImage::enabled() const
{
	if( !ImageProcessor::enabled() )
	{
		return false;
	}
	// generally the connectedness of plugs should not be queried by compute() or hash() (which
	// will end up calling this function), because Shaders are not ComputeNodes and must be
	// accessed directly anyway, it's ok.
	return shaderPlug()->getInput();
}

void OSLImage::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ImageProcessor::hash( output, context, h );

	if( output == shadingPlug() )
	{
		hashShading( context, h );
	}
}

void OSLImage::compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const
{
	if( output == shadingPlug() )
	{
		static_cast<ObjectPlug *>( output )->setValue( computeShading( context ) );
		return;
	}

	ImageProcessor::compute( output, context );
}

Gaffer::ValuePlug::CachePolicy OSLImage::computeCachePolicy( const Gaffer::ValuePlug *output ) const
{
	if( output == outPlug()->channelDataPlug() )
	{
		// We disable caching for the channel data plug, because our compute
		// simply references data direct from the shading plug, which will itself
		// be cached. We don't want to count the memory usage for that twice.
		return ValuePlug::CachePolicy::Uncached;
	}

	return ImageProcessor::computeCachePolicy( output );
}

void OSLImage::hashChannelNames( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ImageProcessor::hashChannelNames( output, context, h );
	defaultedInPlug()->channelNamesPlug()->hash( h );

	const Box2i dataWindow = defaultedInPlug()->dataWindowPlug()->getValue();
	if( !dataWindow.isEmpty() )
	{
		ImagePlug::ChannelDataScope channelDataScope( context );
		channelDataScope.setTileOrigin( ImagePlug::tileOrigin( dataWindow.min ) );
		shadingPlug()->hash( h );
	}
}

IECore::ConstStringVectorDataPtr OSLImage::computeChannelNames( const Gaffer::Context *context, const GafferImage::ImagePlug *parent ) const
{
	ConstStringVectorDataPtr channelNamesData = defaultedInPlug()->channelNamesPlug()->getValue();

	set<string> result( channelNamesData->readable().begin(), channelNamesData->readable().end() );

	const Box2i dataWindow = defaultedInPlug()->dataWindowPlug()->getValue();
	if( !dataWindow.isEmpty() )
	{
		ImagePlug::ChannelDataScope channelDataScope( context );
		channelDataScope.setTileOrigin( ImagePlug::tileOrigin( dataWindow.min ) );

		ConstCompoundDataPtr shading = runTimeCast<const CompoundData>( shadingPlug()->getValue() );
		for( CompoundDataMap::const_iterator it = shading->readable().begin(), eIt = shading->readable().end(); it != eIt; ++it )
		{
			result.insert( it->first );
		}
	}

	return new StringVectorData( vector<string>( result.begin(), result.end() ) );
}

void OSLImage::hashChannelData( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ImageProcessor::hashChannelData( output, context, h );
	const std::string &channelName = context->get<std::string>( ImagePlug::channelNameContextName );
	h.append( channelName );
	shadingPlug()->hash( h );

	ConstStringVectorDataPtr channelNamesData;
	{
		ImagePlug::GlobalScope c( context );
		channelNamesData = defaultedInPlug()->channelNamesPlug()->getValue();
	}

	if(
		std::find( channelNamesData->readable().begin(), channelNamesData->readable().end(), channelName ) !=
		channelNamesData->readable().end()
	)
	{
		defaultedInPlug()->channelDataPlug()->hash( h );
	}
}

IECore::ConstFloatVectorDataPtr OSLImage::computeChannelData( const std::string &channelName, const Imath::V2i &tileOrigin, const Gaffer::Context *context, const GafferImage::ImagePlug *parent ) const
{
	ConstCompoundDataPtr shadedPoints = runTimeCast<const CompoundData>( shadingPlug()->getValue() );
	ConstFloatVectorDataPtr result = shadedPoints->member<FloatVectorData>( channelName );

	if( !result )
	{
		result = defaultedInPlug()->channelDataPlug()->getValue();
	}

	return result;
}

void OSLImage::hashFormat( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	h = defaultedInPlug()->formatPlug()->hash();
}

GafferImage::Format OSLImage::computeFormat( const Gaffer::Context *context, const GafferImage::ImagePlug *parent ) const
{
	return defaultedInPlug()->formatPlug()->getValue();
}

void OSLImage::hashDataWindow( const GafferImage::ImagePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	h = defaultedInPlug()->dataWindowPlug()->hash();
}

Imath::Box2i OSLImage::computeDataWindow( const Gaffer::Context *context, const GafferImage::ImagePlug *parent ) const
{
	return defaultedInPlug()->dataWindowPlug()->getValue();
}

void OSLImage::hashShading( const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	ConstShadingEnginePtr shadingEngine;
	if( auto shader = runTimeCast<const OSLShader>( shaderPlug()->source()->node() ) )
	{
		ImagePlug::GlobalScope globalScope( context );
		shadingEngine = shader->shadingEngine();
	}

	if( !shadingEngine )
	{
		h = shadingPlug()->defaultValue()->hash();
		return;
	}

	h.append( context->get<V2i>( ImagePlug::tileOriginContextName ) );

	ConstStringVectorDataPtr channelNamesData;
	bool deep;
	{
		ImagePlug::GlobalScope c( context );
		defaultedInPlug()->formatPlug()->hash( h );
		channelNamesData = defaultedInPlug()->channelNamesPlug()->getValue();
		deep = defaultedInPlug()->deepPlug()->getValue();
	}

	{
		ImagePlug::ChannelDataScope c( context );
		if( deep )
		{
			c.remove( ImagePlug::channelNameContextName );
			defaultedInPlug()->sampleOffsetsPlug()->hash( h );
		}

		for( const auto &channelName : channelNamesData->readable() )
		{
			if( shadingEngine->needsAttribute( channelName ) )
			{
				c.setChannelName( channelName );
				defaultedInPlug()->channelDataPlug()->hash( h );
			}
		}
	}

	shadingEngine->hash( h );
}

IECore::ConstCompoundDataPtr OSLImage::computeShading( const Gaffer::Context *context ) const
{
	ConstShadingEnginePtr shadingEngine;
	if( auto shader = runTimeCast<const OSLShader>( shaderPlug()->source()->node() ) )
	{
		ImagePlug::GlobalScope globalScope( context );
		shadingEngine = shader->shadingEngine();
	}

	if( !shadingEngine )
	{
		return static_cast<const CompoundData *>( shadingPlug()->defaultValue() );
	}

	const V2i tileOrigin = context->get<V2i>( ImagePlug::tileOriginContextName );
	Format format;
	ConstStringVectorDataPtr channelNamesData;
	bool deep;
	{
		ImagePlug::GlobalScope c( context );
		format = defaultedInPlug()->formatPlug()->getValue();
		channelNamesData = defaultedInPlug()->channelNamesPlug()->getValue();
		deep = defaultedInPlug()->deepPlug()->getValue();
	}

	CompoundDataPtr shadingPoints = new CompoundData();
	ConstIntVectorDataPtr sampleOffsetsData;


	{
		ImagePlug::ChannelDataScope c( context );
		if( deep )
		{
			c.remove( ImagePlug::channelNameContextName );
			sampleOffsetsData = defaultedInPlug()->sampleOffsetsPlug()->getValue();
		}

		for( const auto &channelName : channelNamesData->readable() )
		{
			if( shadingEngine->needsAttribute( channelName ) )
			{
				c.setChannelName( channelName );
				shadingPoints->writable()[channelName] = boost::const_pointer_cast<FloatVectorData>(
					defaultedInPlug()->channelDataPlug()->getValue()
				);
			}
		}
	}

	int numSamples = sampleOffsetsData ? sampleOffsetsData->readable().back() : ImagePlug::tilePixels();

	V3fVectorDataPtr pData = new V3fVectorData;
	FloatVectorDataPtr uData = new FloatVectorData;
	FloatVectorDataPtr vData = new FloatVectorData;

	vector<V3f> &pWritable = pData->writable();
	vector<float> &uWritable = uData->writable();
	vector<float> &vWritable = vData->writable();

	pWritable.reserve( numSamples );
	uWritable.reserve( numSamples );
	vWritable.reserve( numSamples );

	const V2f uvStep = V2f( 1.0f ) / format.getDisplayWindow().size();
	// UV value for the pixel at 0,0
	const V2f uvOrigin = (V2f(0.5) - format.getDisplayWindow().min) * uvStep;
	const V2i pMax = tileOrigin + V2i( ImagePlug::tileSize() );

	if( !deep )
	{
		V2i p;
		for( p.y = tileOrigin.y; p.y < pMax.y; ++p.y )
		{
			const float v = uvOrigin.y + p.y * uvStep.y;
			for( p.x = tileOrigin.x; p.x < pMax.x; ++p.x )
			{
				uWritable.push_back( uvOrigin.x + p.x * uvStep.x );
				vWritable.push_back( v );
				pWritable.push_back( V3f( p.x + 0.5f, p.y + 0.5f, 0.0f ) );
			}
		}
	}
	else
	{
		const std::vector< int > &sampleOffsets = sampleOffsetsData->readable();
		int prevOffset = 0;
		int index = 0;
		V2i p;
		for( p.y = tileOrigin.y; p.y < pMax.y; ++p.y )
		{
			const float v = uvOrigin.y + p.y * uvStep.y;
			for( p.x = tileOrigin.x; p.x < pMax.x; ++p.x )
			{
				int offset = sampleOffsets[index];
				for( int j = 0; j < offset - prevOffset; j++ )
				{
					uWritable.push_back( uvOrigin.x + p.x * uvStep.x );
					vWritable.push_back( v );
					pWritable.push_back( V3f( p.x + 0.5f, p.y + 0.5f, j ) );
				}
				prevOffset = offset;
				index++;
			}
		}
	}

	shadingPoints->writable()["P"] = pData;
	shadingPoints->writable()["u"] = uData;
	shadingPoints->writable()["v"] = vData;


	CompoundDataPtr result = shadingEngine->shade( shadingPoints.get() );

	// remove results that aren't suitable to become channels
	for( CompoundDataMap::iterator it = result->writable().begin(); it != result->writable().end();  )
	{
		CompoundDataMap::iterator nextIt = it; nextIt++;
		if( !runTimeCast<FloatVectorData>( it->second ) )
		{
			result->writable().erase( it );
		}
		it = nextIt;
	}

	return result;
}

void OSLImage::updateChannels()
{
	// Disable undo for the actions we perform, because anything that can
	// trigger an update is undoable itself, and we will take care of everything as a whole
	// when we are undone.
	UndoScope undoDisabler( scriptNode(), UndoScope::Disabled );

	// Currently the OSLCode node will recompile every time an input is added.
	// We're hoping in the future to avoid doing this until the network is actually needed,
	// but in the meantime, we can save some time by emptying the code first, so that at least
	// all the redundant recompiles are of shorter code.
	oslCode()->codePlug()->setValue( "" );

	oslCode()->parametersPlug()->clearChildren();

	std::string code = "Ci = 0;\n";

	for( NameValuePlugIterator inputPlug( channelsPlug() ); !inputPlug.done(); ++inputPlug )
	{
		std::string prefix = "";
		BoolPlug* enabledPlug = (*inputPlug)->enabledPlug();
		if( enabledPlug )
		{
			IntPlugPtr codeEnablePlug = new IntPlug( "enable" );
			oslCode()->parametersPlug()->addChild( codeEnablePlug );
			codeEnablePlug->setInput( enabledPlug );
			prefix = "if( " + codeEnablePlug->getName().string() + " ) ";
		}

		Plug *valuePlug = (*inputPlug)->valuePlug();

		if( valuePlug->typeId() == ClosurePlug::staticTypeId() )
		{
			// Closures are a special case that doesn't need a wrapper function
			ClosurePlugPtr codeClosurePlug = new ClosurePlug( "closureIn" );
			oslCode()->parametersPlug()->addChild( codeClosurePlug );
			codeClosurePlug->setInput( valuePlug );

			code += prefix + "Ci += " + codeClosurePlug->getName().string() + ";\n";
			continue;
		}

		std::string outFunction;
		PlugPtr codeValuePlug;
		const Gaffer::TypeId valueType = (Gaffer::TypeId)valuePlug->typeId();
		switch( (int)valueType )
		{
			case FloatPlugTypeId :
				codeValuePlug = new FloatPlug( "value" );
				outFunction = "outChannel";
				break;
			case Color3fPlugTypeId :
				codeValuePlug = new Color3fPlug( "value" );
				outFunction = "outLayer";
				break;
		}

		if( codeValuePlug )
		{

			StringPlugPtr codeNamePlug = new StringPlug( "name" );
			oslCode()->parametersPlug()->addChild( codeNamePlug );
			codeNamePlug->setInput( (*inputPlug)->namePlug() );

			oslCode()->parametersPlug()->addChild( codeValuePlug );
			codeValuePlug->setInput( valuePlug );

			code += prefix + "Ci += " + outFunction + "( " + codeNamePlug->getName().string() + ", "
				+ codeValuePlug->getName().string() + ");\n";
			continue;
		}

		IECore::msg( IECore::Msg::Warning, "OSLImage::updateChannels",
			"Could not create channel from plug: " + (*inputPlug)->fullName()
		);
	}

	oslCode()->codePlug()->setValue( code );
}

void OSLImage::channelsAdded( const Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
	updateChannels();
}

void OSLImage::channelsRemoved( const Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
	updateChannels();
}

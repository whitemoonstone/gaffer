//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2012-2013, Image Engine Design Inc. All rights reserved.
//  Copyright (c) 2013-2014, John Haddon. All rights reserved.
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

#include "boost/python.hpp"

#include "IECoreGL/Texture.h"

#include "IECorePython/RunTimeTypedBinding.h"

#include "GafferBindings/SignalBinding.h"

#include "GafferUI/Style.h"

#include "GafferUIBindings/StyleBinding.h"

using namespace boost::python;
using namespace GafferBindings;
using namespace GafferUIBindings;
using namespace GafferUI;

struct UnarySlotCaller
{
	boost::signals::detail::unusable operator()( boost::python::object slot, StylePtr s )
	{
		slot( s );
		return boost::signals::detail::unusable();
	}
};

void GafferUIBindings::bindStyle()
{
	scope s = IECorePython::RunTimeTypedClass<Style>()

		.def( "renderImage", &Style::renderImage )
		.def( "renderLine", &Style::renderLine )
		.def( "renderSolidRectangle", &Style::renderSolidRectangle )
		.def( "renderRectangle", &Style::renderRectangle )

		.def( "characterBound", &Style::characterBound )
		.def( "textBound", &Style::textBound )
		.def( "renderText", &Style::renderText )
		.def( "renderWrappedText", &Style::renderWrappedText )

		.def( "renderFrame", &Style::renderFrame )
		.def( "renderSelectionBox", &Style::renderSelectionBox )
		.def( "renderHorizontalRule", &Style::renderHorizontalRule )

		.def( "renderNodeFrame", &Style::renderNodeFrame )
		.def( "renderNodule", &Style::renderNodule )
		.def( "renderConnection", &Style::renderConnection )
		.def( "renderBackdrop", &Style::renderBackdrop )

		.def( "renderTranslateHandle", &Style::renderTranslateHandle )

		.def( "changedSignal", &Style::changedSignal, return_internal_reference<1>() )
		.def( "getDefaultStyle", &Style::getDefaultStyle ).staticmethod( "getDefaultStyle" )
		.def( "setDefaultStyle", &Style::getDefaultStyle ).staticmethod( "setDefaultStyle" )

	;

	enum_<Style::State>( "State" )
		.value( "NormalState", Style::NormalState )
		.value( "DisabledState", Style::DisabledState )
		.value( "HighlightedState", Style::HighlightedState )
	;

	enum_<Style::TextType>( "TextType" )
		.value( "LabelText", Style::LabelText )
		.value( "BodyText", Style::BodyText )
	;

	enum_<Style::Axes>( "Axes" )
		.value( "X", Style::X )
		.value( "Y", Style::Y )
		.value( "Z", Style::Z )
		.value( "XYZ", Style::XYZ )
	;

	SignalClass<Style::UnarySignal, DefaultSignalCaller<Style::UnarySignal>, UnarySlotCaller>( "UnarySignal" );

}

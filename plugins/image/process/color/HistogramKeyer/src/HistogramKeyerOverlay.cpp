#include "HistogramKeyerOverlay.hpp"
#include "HistogramKeyerPlugin.hpp"
#include "HistogramKeyerHistogramDisplay.hpp"
#include "tuttle/plugin/ImageGilProcessor.hpp"

#include <tuttle/plugin/opengl/gl.h>
#include <tuttle/plugin/interact/interact.hpp>
#include <tuttle/plugin/interact/overlay.hpp>
#include <tuttle/plugin/image/ofxToGil.hpp>

#include <ofxsImageEffect.h>
#include <ofxsInteract.h>
#include <boost/gil/gil_all.hpp>
#include <boost/gil/extension/algorithm.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>

namespace tuttle {
namespace plugin {
namespace histogramKeyer {

HistogramKeyerOverlay::HistogramKeyerOverlay( OfxInteractHandle handle, OFX::ImageEffect* effect )
	: OFX::OverlayInteract( handle ), _infos( effect )
{    
	_plugin = static_cast<HistogramKeyerPlugin*>( _effect );
	_histogramDisplay = HistogramKeyerHistogramDisplay(_plugin);
	_penDown = false;
	
	
	_size = _plugin->_clipSrc->getPixelRodSize(NULL);
	//allocate and initialize bool img tab 2D
	bool_2d::extent_gen extents;
	_imgBool.resize(extents[_size.y][_size.x]);
	
	for(unsigned int i=0; i<_size.y; ++i)
	{
		for(unsigned int j=0; j<_size.x; ++j)
			_imgBool[i][j] = false;
	}
}

bool HistogramKeyerOverlay::draw( const OFX::DrawArgs& args )
{
	typedef boost::gil::point2<Scalar> Point2;
	bool displaySomething = false;

	if(_plugin->_isCleaned) // clean button pressed
	{
		//Clear all the OpenGL scene
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_MODELVIEW);
		
		for(unsigned int i=0; i<_size.y; ++i) // remove user selection
		{
			for(unsigned int j=0; j<_size.x; ++j)
				_imgBool[i][j] = false;
		}
		_plugin->_isCleaned = false;
	}
	
	if(_plugin->_isNbStepChanged) //nbSteps option modified
	{
		_plugin->_selectionData._step = _plugin->_data._step;
		_plugin->resetHistogramBufferData(_plugin->_selectionData);
		this->computeSelectionHistograms(args.time,args.renderScale);
		_plugin->_isNbStepChanged = false;
	}
	
	if(_plugin->_clipSrc->isConnected())
	{
		displaySomething = true;
        //is the display adapted to each channel or global (IHM)        
        if(this->_plugin->_paramDisplayTypeSelection->getValue() == 1)
			this->_histogramDisplay._isAdaptedToHeight = true;
        else
            this->_histogramDisplay._isAdaptedToHeight = false;
        
		///@todo : remove next line when Nuke curves overlay works
		this->_histogramDisplay._translateHSL = false;		
		this->_histogramDisplay.displayHistogramOnScreenRGB(_plugin->_data,_plugin->_selectionData,args.time);
		
		///@todo : remove next line when Nuke curves overlay works
		this->_histogramDisplay._translateHSL = true;		
		this->_histogramDisplay.displayHistogramOnScreenHSL(_plugin->_data,_plugin->_selectionData,args.time);
		
		//DisplaySelection
		if(_plugin->_paramDisplaySelection->getValue())
			this->displaySelectedAreas();
		//Display selection zone
		if(_penDown)
			this->displaySelectionZone();
	}
	return displaySomething;
}

bool HistogramKeyerOverlay::penMotion( const OFX::PenArgs& args )
{	
	if(_penDown)
	{
		_end.x = args.penPosition.x;
		_end.y = args.penPosition.y;
	}
	return true;
}

bool HistogramKeyerOverlay::penDown( const OFX::PenArgs& args )
{
	if(!_penDown)
	{
		if(args.penPosition.x < _size.x && args.penPosition.x > 0)
		{
			if(args.penPosition.y < _size.y && args.penPosition.y > 0)
			{
				_origin.x = args.penPosition.x;
				_origin.y = args.penPosition.y;
				_penDown = true;
			}
		}
	}
	return true;
}

bool HistogramKeyerOverlay::penUp( const OFX::PenArgs& args )
{
	if(_penDown && args.penPosition.x < _size.x && args.penPosition.x > 0 && args.penPosition.y < _size.y && args.penPosition.y > 0)
	{
		_end.x = args.penPosition.x;
		_end.y = args.penPosition.y;
		
		//pixel traitement
		int startX,endX,startY,endY;
		if(_origin.x > _end.x)
		{
			startX = _origin.x;
			endX = _end.x;
		}
		else
		{
			endX =_origin.x;
			startX = _end.x;
		}
		
		if(_origin.y > _end.y)
		{
			startY = _origin.y;
			endY = _end.y;
		}
		else
		{ 
			endY= _origin.y;
			startY = _end.y;
		}
		int step_x = startX-endX;
		int step_y = startY-endY;
		for(unsigned int val_y=0; val_y<step_y; ++val_y)
		{
			for(unsigned int val_x=0; val_x<step_x; ++val_x )
			{
				_imgBool[endY+val_y][endX+val_x] = true;
			}
		}
		_penDown = false;
		//update selection histograms buffer datas
		this->computeSelectionHistograms(args.time,args.renderScale);
		_plugin->correctHistogramBufferData(_plugin->_selectionData);
	}
	_penDown = false;
	
	return true;
}

/**
 *Display the selected areas on the clip (color : gray)
 */
void HistogramKeyerOverlay::displaySelectedAreas()
{
	glEnable(GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);
	glBegin( GL_POINTS );
	glColor3f(.01f,0.01f,0.01f);
	for(unsigned int i=0; i<_size.y; ++i)
	{
		for(unsigned int j=0; j<_size.x; ++j)
		{	
			if(_imgBool[i][j])
				glVertex2f((float)j,(float)i);
		}
	}
	glDisable(GL_BLEND);
	glEnd();
}

/**
 *Display the selection zone on the clip (color : border gray)
 */
void HistogramKeyerOverlay::displaySelectionZone()
{
	glEnable(GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(1, (short) 0x0101);
	glBegin( GL_LINE_LOOP );
	glColor3f(.5f,0.5f,0.5f);
	glVertex2f(_origin.x,_origin.y);
	glVertex2f(_origin.x,_end.y);
	glVertex2f(_end.x,_end.y);
	glVertex2f(_end.x,_origin.y);
	glEnd();
	glDisable(GL_LINE_STIPPLE);
	glDisable(GL_BLEND);
	glEnd();
}

/**
 * Update selection areas buffer to selection histograms overlay
 * @param args needed to have current time
 */
void HistogramKeyerOverlay::computeSelectionHistograms(const OfxTime time, const OfxPointD renderScale )
{
	//reload view 
	boost::scoped_ptr<OFX::Image> src( _plugin->_clipSrc->fetchImage(time ) );
	typedef boost::gil::rgba32f_view_t SView;
	OfxRectI srcPixelRod = _plugin->_clipSrc->getPixelRod( time,renderScale );
	SView srcView = tuttle::plugin::getView<SView>( src.get(), srcPixelRod );
	//view isn't accessible
	if( !src.get() )
	{
		return;
	}
	if( src->getRowBytes() == 0 )
	{
		BOOST_THROW_EXCEPTION( exception::WrongRowBytes() );
	}
	//reset selection data
	_plugin->resetHistogramBufferData(_plugin->_selectionData);
	
	//functor creation
	Pixel_compute_selection_histograms funct;
	funct._width = _size.x;
	funct._height= _size.y;
	bool_2d::extent_gen extents;
	funct._imgBool.resize(extents[_size.y][_size.x]);
	for(unsigned int i=0; i<_size.y; ++i)
	{
		for(unsigned int j=0; j<_size.x;++j)
			funct._imgBool[i][j] = _imgBool[i][j];
	}
	funct._data = _plugin->_selectionData;
	funct._y = 0;
	funct._x = 0;
	//treatment
	boost::gil::transform_pixels(srcView, funct);
	//keep infos
	_plugin->_selectionData = funct._data;
}

/*
//test
void HistogramKeyerOverlay::gainFocus( const FocusArgs& args )
{
	std::cout << "gain focus" << std::endl;
}*/

}
}
}
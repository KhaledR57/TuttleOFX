from pyTuttle import tuttle

import os


def generate( pluginName, *orderedParams, **namedParams ):
	"""
	generate an image
	wrote automaticaly the command line in the center of the image
	the graph process will be:
	pluginName -> burn command line -> write

	orderedParams are pluginName default ordered parameters (HD)
	namedParams are pluginName parameters (for example: format=HD)
	"""
	filename = pluginName + '_' + '_'.join(['%s_%s' % (key, value) for (key, value) in namedParams.items()]).replace(' ', '_')
	
	cmdLine = pluginName + ' ' + ' '.join(['%s=%s' % (key, value) for (key, value) in namedParams.items()])

	graph = tuttle.Graph()
	generator = graph.createNode( pluginName, *orderedParams, **namedParams )
	text = graph.createNode( "tuttle.text", vAlign="center", bold=1, color=[0.5, 0.5, 0.5, 1.0], text=cmdLine )
	write = graph.createNode( "tuttle.pngwriter", filename=os.path.join( os.environ["DOC_DIR"], "images", filename + ".png" ) )
	graph.connect( [generator, text, write] )
	graph.compute( write )
	#print "######################################################################################"
	#print pluginName + '_'.join(['%s_%s' % (key, value) for (key, value) in namedParams.items()]).replace(' ', '_')
	#print a.join( '_' )
	#print str(*orderedParams)
	#print str(*namedParams)
	#print "######################################################################################"


def generateEXR( pluginName, *orderedParams, **namedParams ):
	"""
	generate an image in exr format (float)
	wrote automaticaly the command line in the center of the image
	the graph process will be:
	pluginName -> burn command line -> write exr

	orderedParams are pluginName default ordered parameters (HD)
	namedParams are pluginName parameters (for example: format=HD)
	"""
	filename = pluginName + '_' + '_'.join(['%s_%s' % (key, value) for (key, value) in namedParams.items()]).replace(' ', '_')
	
	cmdLine = pluginName + ' ' + ' '.join(['%s=%s' % (key, value) for (key, value) in namedParams.items()])

	graph = tuttle.Graph()
	generator = graph.createNode( pluginName, *orderedParams, **namedParams )
	text = graph.createNode( "tuttle.text", vAlign="center", bold=1, color=[0.5, 0.5, 0.5, 1.0], text=cmdLine )
	write = graph.createNode( "tuttle.exrwriter", filename=os.path.join( os.environ["DOC_DIR"], "images", filename + ".exr" ) )
	graph.connect( [generator, text, write] )
	graph.compute( write )
	

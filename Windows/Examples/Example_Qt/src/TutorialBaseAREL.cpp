#include "TutorialBaseAREL.h"
#include "MainWindow.h"

#include <math.h>
#include <sstream>

#include <QtGui>
#include <QtOpenGL>
#include <QGraphicsWebView>
#include <QWebFrame>
#include <QFileInfo>

#include <GL/gl.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#include <metaioSDK/IARELInterpreter.h>
#include <metaioSDK/IMetaioSDKWin32.h>
#include <metaioSDK/GestureHandler.h>

TutorialBaseAREL::TutorialBaseAREL(MainWindow* mainWindow, QString tutorialName) :
	QGraphicsScene(),
	m_initialized(false),
	m_pGestureHandler(0),
	m_pMetaioSDK(0),
	m_tutorialName(tutorialName)
{
	// Create a webview	and connect the load finished event
	m_pWebView = new QGraphicsWebView();
	QObject::connect(m_pWebView, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
	m_pWebView->hide();
	addItem(m_pWebView);
	m_pWebView->resize(800, 600);

	// Create the AREL interpreter
	m_pArelInterpreter = metaio::CreateARELInterpreterQT(this, m_pWebView);
}


TutorialBaseAREL::~TutorialBaseAREL()
{
	// Should deinitialize SDK first so that the camera can be reused when starting another tutorial
	m_pMetaioSDK->pauseTracking();
	m_pMetaioSDK->pauseSensors();
	m_pMetaioSDK->stopCamera();
	m_pMetaioSDK->pause();

	delete m_pArelInterpreter;
	m_pArelInterpreter = 0;

	delete m_pMetaioSDK;
	m_pMetaioSDK = 0;

	delete m_pGestureHandler;
	m_pGestureHandler = 0;

	removeItem(m_pWebView);
	delete m_pWebView;
	m_pWebView = 0;
}


void TutorialBaseAREL::drawBackground(QPainter* painter, const QRectF & rect)
{
	painter->save();
	if (painter->paintEngine()->type()	!= QPaintEngine::OpenGL2)
	{
		qWarning("TutorialBaseAREL: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}

	if(!m_initialized)
	{
		m_initialized = true;

		m_pMetaioSDK = metaio::CreateMetaioSDKWin32();
		m_pMetaioSDK->initializeRenderer(800, 600, metaio::ESCREEN_ROTATION_0, metaio::ERENDER_SYSTEM_OPENGL_EXTERNAL);
	
		// Activate first camera
		std::vector<metaio::Camera> cameras = m_pMetaioSDK->getCameraList();
		if (!cameras.empty())
		{
			// Set the resolution to 640x480
			cameras[0].resolution = metaio::Vector2di(640, 480);
			cameras[0].downsample = 1;
			m_pMetaioSDK->startCamera(cameras[0]);
		}
		else
		{
			qCritical("TutorialBaseAREL: No camera found");
		}

		m_pGestureHandler = new metaio::GestureHandler(m_pMetaioSDK);
		m_pArelInterpreter->initialize(m_pMetaioSDK, m_pGestureHandler);
		m_pArelInterpreter->registerCallback(this);
	}

	// Enable anti-aliasing
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_LINE_SMOOTH);

	m_pArelInterpreter->update();

	// Trigger an update
	QTimer::singleShot(20, this, SLOT(update()));

	painter->restore();

	// This is a workaround to render the webpages correctly
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}


void TutorialBaseAREL::loadFinished(bool loaded)
{
	if(loaded)
		m_pArelInterpreter->loadFinished();
}


void TutorialBaseAREL::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	QGraphicsScene::mouseMoveEvent(mouseEvent);

	// Forward event to gesture handler (needed for drag gesture, just like the mouse press/release events)
	if(m_pGestureHandler)
		m_pGestureHandler->touchesMoved(mouseEvent->scenePos().x(), mouseEvent->scenePos().y());
}


void TutorialBaseAREL::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	QGraphicsScene::mousePressEvent(mouseEvent);

	// Forward event to gesture handler
	if(m_pGestureHandler)
		m_pGestureHandler->touchesBegan(mouseEvent->scenePos().x(), mouseEvent->scenePos().y());
}


void TutorialBaseAREL::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
	QGraphicsScene::mouseReleaseEvent(mouseEvent);

	// Forward event to gesture handler
	if(m_pGestureHandler)
		m_pGestureHandler->touchesEnded(mouseEvent->scenePos().x(), mouseEvent->scenePos().y());
}

void TutorialBaseAREL::loadContent()
{
	// path to AREL xml configuration
	QString fileName  = "../../../tutorialContent_crossplatform/Tutorial" + m_tutorialName + "/arelConfig.xml";
	m_pArelInterpreter->loadARELFile(fileName.toUtf8().constData());
}

void TutorialBaseAREL::onSDKReady()
{
	loadContent();

	// Show the web view after content is loaded and splash screen is finished
	m_pWebView->show();
}
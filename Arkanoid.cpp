#include <windows.h>		// Header File For Windows
#include <math.h>			// Header File For Math Library Routines
#include <stdio.h>			// Header File For Standard I/O Routines
#include <stdlib.h>			// Header File For Standard Library
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include "tvector.h"
#include "tmatrix.h"
#include "tray.h"
#include <mmsystem.h>
#include "image.h"
#include <time.h>

GLfloat spec[]={1.0, 1.0 ,1.0 ,1.0};      //sets specular highlight of balls
GLfloat posl[]={0,0,1000};               //position of ligth source
GLfloat amb[]={0.2f, 0.2f, 0.2f ,1.0f};   //global ambient
GLfloat amb2[]={0.3f, 0.3f, 0.3f ,1.0f};  //ambient of lightsource

TVector dir(0,0,-10);                     //initial direction of camera
TVector pos(0,0,1000);                  //initial position of camera

TVector accel(0,0,0);                 //acceleration ie. gravity of balls

double Time=0.6;                          //timestep of simulation

//Plane structure
struct Plane{
	TVector _Position;
	TVector _Normal;
};

struct Ball
{
	TVector _Velocity;
	TVector _Position;
	TVector _OldPos;
};

struct Box
{
	TVector _Position;
	bool _Active;
	TVector _Point[4];
};

//Explosion structure
struct Explosion{
	TVector _Position;
	float   _Alpha;
	float   _Scale;
};

#define PLANE_COUNT	4
#define PLANE_POS	610
#define BALL_RADIUS		14
Plane planes[PLANE_COUNT];                //the 5 planes of the room
Ball ball;
#define PADDLE_LENGTH 100
#define PADDLE_HEIGHT 10
#define PADDLE_INITY -400
Box paddle;
#define TARGET_HEIGHT		60
#define TARGET_LENGTH		80
#define TARGET_SPACE		30
#define TARGET_ROW_COUNT	4
#define TARGET_COLUMN_COUNT	10
Box target[TARGET_ROW_COUNT][TARGET_COLUMN_COUNT];
GLUquadricObj *cylinder_obj;              //Quadratic object to render the cylinders
GLuint texture[5], dlist;                 //stores texture objects and display list
TVector normal[4];
bool gameOver=FALSE;
Explosion ExplosionArray[20];             //holds max 20 explosions at once
float cameraRotationX=0, cameraRotationY=0;

//Perform Intersection tests with primitives
int TestIntersectionPlane(const Plane& plane,const TVector& position,const TVector& direction, double& lamda, TVector& pNormal);
int TestIntersectionTarget(const Box& target, const TVector& position, const TVector& direction, double& lamda, TVector& pNormal);
void LoadGLTextures();                    //Loads Texture Objects
void InitVars();
void idle();

HDC				hDC=NULL;			// Private GDI Device Context
HGLRC			hRC=NULL;			// Permanent Rendering Context
HWND			hWnd=NULL;			// Holds Our Window Handle
HINSTANCE		hInstance;			// Holds The Instance Of The Application

DEVMODE			DMsaved;			// Saves the previous screen settings (NEW)

bool			keys[256];			// Array Used For The Keyboard Routine
bool			active=TRUE;		// Window Active Flag Set To TRUE By Default
bool			fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default


int ProcessKeys();
LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

/************************************************************************************/


/************************************************************************************/
// (no changes)

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(50.0f,(GLfloat)width/(GLfloat)height,10.f,1700.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

/************************************************************************************/

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	float df=100.0;

	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	glClearColor(0,0,0,0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glMaterialfv(GL_FRONT,GL_SPECULAR,spec);
	glMaterialfv(GL_FRONT,GL_SHININESS,&df);

	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0,GL_POSITION,posl);
	glLightfv(GL_LIGHT0,GL_AMBIENT,amb2);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glEnable(GL_TEXTURE_2D);
	LoadGLTextures();

	//Construct billboarded explosion primitive as display list
	//4 quads at right angles to each other
	glNewList(dlist=glGenLists(1), GL_COMPILE);
	glBegin(GL_QUADS);
	glRotatef(-45,0,1,0);
	glNormal3f(0,0,1);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-50,-40,0);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(50,-40,0);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(50,40,0);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-50,40,0);
	glNormal3f(0,0,-1);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-50,40,0);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(50,40,0);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(50,-40,0);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-50,-40,0);

	glNormal3f(1,0,0);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0,-40,50);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0,-40,-50);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0,40,-50);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0,40,50);
	glNormal3f(-1,0,0);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(0,40,50);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(0,40,-50);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(0,-40,-50);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(0,-40,50);
	glEnd();
	glEndList();

	return TRUE;										// Initialization Went OK
}

/************************************************************************************/

int DrawGLScene(GLvoid)	            // Here's Where We Do All The Drawing
{								
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(pos.X(),pos.Y(),pos.Z(), pos.X()+dir.X(),pos.Y()+dir.Y(),pos.Z()+dir.Z(), 0,1.0,0.0);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	//render balls
	glColor3f(1.0f,0.0f,1.0f);

	glPushMatrix();
	glTranslated(ball._Position.X(),ball._Position.Y(),ball._Position.Z());
	gluSphere(cylinder_obj,BALL_RADIUS,20,20);
	glPopMatrix();

	glPushMatrix();
	glColor3f(1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);
	glNormal3f( 0.0f, 0.0f, 1.0f);
	glVertex3d(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,paddle._Position.Z());
	glVertex3d(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,paddle._Position.Z());
	glVertex3d(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,paddle._Position.Z());
	glVertex3d(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,paddle._Position.Z());
	glEnd();
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);

	for (int i = 0; i < TARGET_ROW_COUNT; i++)
	{
		for (int j = 0; j  < TARGET_COLUMN_COUNT; j ++)
		{
			if (target[i][j]._Active)
			{
				glBindTexture(GL_TEXTURE_2D, texture[i%4]);
				glPushMatrix();
				glTranslated(target[i][j]._Position.X(),target[i][j]._Position.Y(),target[i][j]._Position.Z());
				glBegin(GL_QUADS);
				glNormal3f( 0.0f, 0.0f, 1.0f);
				glTexCoord2f(0.0f,0.0f); glVertex3f(-1*TARGET_LENGTH/2, -1*TARGET_HEIGHT/2, 0);
				glTexCoord2f(1.0f,0.0f); glVertex3f(TARGET_LENGTH/2, -1*TARGET_HEIGHT/2, 0);
				glTexCoord2f(1.0f,1.0f); glVertex3f(TARGET_LENGTH/2, TARGET_HEIGHT/2, 0);
				glTexCoord2f(0.0f,1.0f); glVertex3f(-1*TARGET_LENGTH/2, TARGET_HEIGHT/2, 0);
				glEnd();
				glPopMatrix();
			}
		}
	}
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, texture[4]);   
	for(int i=0; i<20; i++)
	{
		if(ExplosionArray[i]._Alpha>=0)
		{
			glPushMatrix();
			ExplosionArray[i]._Alpha-=0.01f;
			ExplosionArray[i]._Scale+=0.03f;
			glColor4f(1,1,0,ExplosionArray[i]._Alpha);	 
			glScalef(ExplosionArray[i]._Scale,ExplosionArray[i]._Scale,ExplosionArray[i]._Scale);
			glTranslatef((float)ExplosionArray[i]._Position.X()/ExplosionArray[i]._Scale, (float)ExplosionArray[i]._Position.Y()/ExplosionArray[i]._Scale, (float)ExplosionArray[i]._Position.Z()/ExplosionArray[i]._Scale);
			glCallList(dlist);
			glPopMatrix();
		}
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);


	return TRUE;										// Keep Going
}

/************************************************************************************/

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		if (!ChangeDisplaySettings(NULL,CDS_TEST)) { // if the shortcut doesn't work
			ChangeDisplaySettings(NULL,CDS_RESET);		// Do it anyway (to get the values out of the registry)
			ChangeDisplaySettings(&DMsaved,CDS_RESET);	// change it to the saved settings
		} else {
			ChangeDisplaySettings(NULL,CDS_RESET);
		}

		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/************************************************************************************/

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
*	title			- Title To Appear At The Top Of The Window				*
*	width			- Width Of The GL Window Or Fullscreen Mode				*
*	height			- Height Of The GL Window Or Fullscreen Mode			*
*	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
*	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DMsaved); // save the current display state (NEW)

	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
		"OpenGL",							// Class Name
		title,								// Window Title
		dwStyle |							// Defined Window Style
		WS_CLIPSIBLINGS |					// Required Window Style
		WS_CLIPCHILDREN,					// Required Window Style
		0, 0,								// Window Position
		WindowRect.right-WindowRect.left,	// Calculate Window Width
		WindowRect.bottom-WindowRect.top,	// Calculate Window Height
		NULL,								// No Parent Window
		NULL,								// No Menu
		hInstance,							// Instance
		NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

/************************************************************************************/
// (no changes)

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
						 UINT	uMsg,			// Message For This Window
						 WPARAM	wParam,			// Additional Message Information
						 LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
	case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

	case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
			case SC_SCREENSAVE:					// Screensaver Trying To Start?
			case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

	case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

	case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

	case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

	case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

/************************************************************************************/

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
				   HINSTANCE	hPrevInstance,		// Previous Instance
				   LPSTR		lpCmdLine,			// Command Line Parameters
				   int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	// Ask The User Which Screen Mode They Prefer
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
		fullscreen=FALSE;							// Windowed Mode
	}

	InitVars();                                     // Initialize Variables

	// Create Our OpenGL Window
	if (!CreateGLWindow("ARKANOID",640,480,16,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}
	srand(time(NULL));
	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
			if (active)
			{
				// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
				if (keys[VK_ESCAPE])	// Active?  Was There A Quit Received?
				{
					done=TRUE;							// ESC or DrawGLScene Signalled A Quit
				}
				else									// Not Time To Quit, Update Screen
				{
					idle();                             // Advance Simulation
					DrawGLScene();                      // Draw Scene
					SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
				}

				if (!ProcessKeys()) return 0;
			}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	glDeleteTextures(4,texture);
	return (msg.wParam);							// Exit The Program
}



/*************************************************************************************/
/*************************************************************************************/
/***             Main loop of the simulation                                      ****/
/***      Moves, finds the collisions and responses of the objects in the         ****/
/***      current time step.                                                      ****/
/*************************************************************************************/
/*************************************************************************************/
void idle()
{
	double rt,rt2,rt4,lamda=10000;
	TVector norm,uveloc;
	TVector normal,point;
	double RestTime;

	RestTime=Time;
	lamda=1000;

	//Compute velocity for next timestep using Euler equations

	ball._Velocity+=accel*RestTime;

	//While timestep not over
	while (RestTime>ZERO)
	{
		lamda=10000;   //initialize to very large value

		//For all the balls find closest intersection between balls and planes/cylinders
		//compute new position and distance
		ball._OldPos=ball._Position;
		TVector::unit(ball._Velocity,uveloc);
		ball._Position=ball._Position+ball._Velocity*RestTime;
		rt2=ball._OldPos.dist(ball._Position);

		//Test if collision occured between ball and all 5 planes
		for (int j = 0; j < PLANE_COUNT; j++)
		{
			if (TestIntersectionPlane(planes[j],ball._OldPos,uveloc,rt,norm))
			{  
				//Find intersection time
				rt4=rt*RestTime/rt2;

				//if smaller than the one already stored replace and in timestep
				if (rt4<=lamda)
				{ 
					if (rt4<=RestTime+ZERO)
						if (! ((rt<=ZERO)&&(uveloc.dot(norm)>ZERO)) )
						{
							if(j==PLANE_COUNT-1) gameOver=TRUE;
							normal=norm;
							point=ball._OldPos+uveloc*rt;
							lamda=rt4;
						}
				}
			}
		}

		int intTarRow = -1;
		int intTarColumn = -1;

		for (int i = 0; i < TARGET_ROW_COUNT; i++)
		{
			for (int j = 0; j < TARGET_COLUMN_COUNT; j++)
			{
				if (target[i][j]._Active && TestIntersectionTarget(target[i][j],ball._OldPos,uveloc,rt,norm))
				{
					//Find intersection time
					rt4=rt*RestTime/rt2;

					//if smaller than the one already stored replace and in timestep
					if (rt4<=lamda)
					{ 

						if (rt4<=RestTime+ZERO)
							if (! ((rt<=ZERO)&&(uveloc.dot(norm)>ZERO)) )
							{
								intTarRow=i;
								intTarColumn=j;
								gameOver=FALSE;
								normal=norm;
								point=ball._OldPos+uveloc*rt;
								lamda=rt4;
							}
					}
				}
			}
		}

		if (TestIntersectionTarget(paddle,ball._OldPos,uveloc,rt,norm))
		{
			//Find intersection time
			rt4=rt*RestTime/rt2;

			//if smaller than the one already stored replace and in timestep
			if (rt4<=lamda)
			{ 

				if (rt4<=RestTime+ZERO)
					if (! ((rt<=ZERO)&&(uveloc.dot(norm)>ZERO)) )
					{
						intTarRow=-1;
						intTarColumn=-1;
						gameOver=FALSE;
						normal=norm;
						point=ball._OldPos+uveloc*rt;
						lamda=rt4;
					}
			}
		}
		//End of tests 
		//If test occured move simulation for the correct timestep
		//and compute response for the colliding ball
		if(gameOver)
		{
			InitVars();
			RestTime=0;
			gameOver=FALSE;
		}
		else if (lamda!=10000)
		{		 
			RestTime-=lamda;

			ball._Position=ball._OldPos+ball._Velocity*lamda;

			rt2=ball._Velocity.mag();
			ball._Velocity.unit();
			ball._Velocity=TVector::unit( (normal*(2*normal.dot(-ball._Velocity))) + ball._Velocity );
			ball._Velocity=ball._Velocity*rt2;
			if(intTarRow!=-1 && intTarColumn!=-1)
				target[intTarRow][intTarColumn]._Active=FALSE;

			//Update explosion array
			for(int j=0;j<20;j++)
			{
				if (ExplosionArray[j]._Alpha<=0)
				{
					ExplosionArray[j]._Alpha=1;
					ExplosionArray[j]._Position=point;
					ExplosionArray[j]._Scale=1;
					break;
				}
			}
		}
		else RestTime=0;

	}

}

/*************************************************************************************/
/*************************************************************************************/
/***        Init Variables                                                        ****/
/*************************************************************************************/
/*************************************************************************************/
void InitVars()
{
	int planePos=PLANE_POS-BALL_RADIUS;
	//create planes
	planes[0]._Position=TVector(planePos,0,0);
	planes[0]._Normal=TVector(-1,0,0);
	planes[1]._Position=TVector(-1*planePos,0,0);
	planes[1]._Normal=TVector(1,0,0);
	planes[2]._Position=TVector(0,planePos*0.75,0);
	planes[2]._Normal=TVector(0,-1,0);
	planes[3]._Position=TVector(0,-1*planePos,0);
	planes[3]._Normal=TVector(0,1,0);

	//create quadratic object to render cylinders
	cylinder_obj= gluNewQuadric();
	gluQuadricTexture(cylinder_obj, GL_TRUE);

	//Set initial positions and velocities of balls
	//also initialize array which holds explosions
	double velocX = rand()%20;
	velocX -= 10;
	velocX /= 10;
	ball._Velocity=TVector(velocX,-1,0);
	ball._Position=TVector(0,70,0);

	paddle._Position=TVector(0,PADDLE_INITY,0);
	paddle._Active = TRUE;
	paddle._Point[0]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
	paddle._Point[1]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
	paddle._Point[2]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);
	paddle._Point[3]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);

	for (int i = 0; i < TARGET_ROW_COUNT; i++)
	{
		for (int j = 0; j  < TARGET_COLUMN_COUNT/2; j ++)
		{
			target[i][j]._Position=TVector((j-TARGET_COLUMN_COUNT/2+0.5)*(TARGET_LENGTH+TARGET_SPACE) ,100+i*(TARGET_HEIGHT+TARGET_SPACE) ,0);
			target[i][j]._Active=TRUE;
			target[i][j]._Point[0]=TVector(target[i][j]._Position.X()-TARGET_LENGTH/2,target[i][j]._Position.Y()-TARGET_HEIGHT/2,0);
			target[i][j]._Point[1]=TVector(target[i][j]._Position.X()+TARGET_LENGTH/2,target[i][j]._Position.Y()-TARGET_HEIGHT/2,0);
			target[i][j]._Point[2]=TVector(target[i][j]._Position.X()+TARGET_LENGTH/2,target[i][j]._Position.Y()+TARGET_HEIGHT/2,0);
			target[i][j]._Point[3]=TVector(target[i][j]._Position.X()-TARGET_LENGTH/2,target[i][j]._Position.Y()+TARGET_HEIGHT/2,0);

			target[i][TARGET_COLUMN_COUNT-j-1]._Position=TVector((TARGET_COLUMN_COUNT/2-j-0.5)*(TARGET_LENGTH+TARGET_SPACE) ,100+i*(TARGET_HEIGHT+TARGET_SPACE) ,0);
			target[i][TARGET_COLUMN_COUNT-j-1]._Active=TRUE;
			target[i][TARGET_COLUMN_COUNT-j-1]._Point[0]=TVector(target[i][TARGET_COLUMN_COUNT-j-1]._Position.X()-TARGET_LENGTH/2,target[i][TARGET_COLUMN_COUNT-j-1]._Position.Y()-TARGET_HEIGHT/2,0);
			target[i][TARGET_COLUMN_COUNT-j-1]._Point[1]=TVector(target[i][TARGET_COLUMN_COUNT-j-1]._Position.X()+TARGET_LENGTH/2,target[i][TARGET_COLUMN_COUNT-j-1]._Position.Y()-TARGET_HEIGHT/2,0);
			target[i][TARGET_COLUMN_COUNT-j-1]._Point[2]=TVector(target[i][TARGET_COLUMN_COUNT-j-1]._Position.X()+TARGET_LENGTH/2,target[i][TARGET_COLUMN_COUNT-j-1]._Position.Y()+TARGET_HEIGHT/2,0);
			target[i][TARGET_COLUMN_COUNT-j-1]._Point[3]=TVector(target[i][TARGET_COLUMN_COUNT-j-1]._Position.X()-TARGET_LENGTH/2,target[i][TARGET_COLUMN_COUNT-j-1]._Position.Y()+TARGET_HEIGHT/2,0);
		}
	}
	normal[0]=TVector(0,-1,0);
	normal[1]=TVector(1,0,0);
	normal[2]=TVector(0,1,0);
	normal[3]=TVector(-1,0,0);
	for (int i = 0; i < 20; i++)
	{
		ExplosionArray[i]._Alpha=0;
		ExplosionArray[i]._Scale=1;
	}
}

/*************************************************************************************/
/*************************************************************************************/
/***        Fast Intersection Function between ray/plane                          ****/
/*************************************************************************************/
/*************************************************************************************/
int TestIntersectionPlane(const Plane& plane,const TVector& position,const TVector& direction, double& lamda, TVector& pNormal)
{

	double DotProduct=direction.dot(plane._Normal);
	double l2;

	//determine if ray paralle to plane
	if ((DotProduct<ZERO)&&(DotProduct>-ZERO)) 
		return 0;

	l2=(plane._Normal.dot(plane._Position-position))/DotProduct;

	if (l2<-ZERO) 
		return 0;

	pNormal=plane._Normal;
	lamda=l2;
	return 1;

}

int TestIntersectionTarget(const Box& target, const TVector& position, const TVector& direction, double& lamda, TVector& pNormal)
{
	TVector result;
	double chosenL2=10000;
	int chosen=-1;
	double l2;
	for (int i = 0; i < 4; i++)
	{
		double DotProduct=direction.dot(normal[i]);

		//determine if ray paralle to plane
		if ((DotProduct<ZERO)&&(DotProduct>-ZERO)) 
			continue;

		l2=(normal[i].dot(target._Point[i]-position))/DotProduct;

		if (l2<-ZERO)
			continue;
		TVector intersect = position + TVector::multiply(direction,l2,result);
		if (intersect.X()+BALL_RADIUS >= target._Point[0].X() && intersect.Y()+BALL_RADIUS >= target._Point[0].Y() &&
			intersect.X()-BALL_RADIUS <= target._Point[1].X() && intersect.Y()+BALL_RADIUS >= target._Point[1].Y() &&
			intersect.X()-BALL_RADIUS <= target._Point[2].X() && intersect.Y()-BALL_RADIUS <= target._Point[2].Y() &&
			intersect.X()+BALL_RADIUS >= target._Point[3].X() && intersect.Y()-BALL_RADIUS <= target._Point[3].Y())
		{
			if(l2 < chosenL2)
			{
				chosenL2 = l2;
				chosen	= i;
			}
		}
	}
	if(chosen != -1)
	{
		pNormal=normal[chosen];
		lamda=chosenL2;
		return 1;
	}
	else
	{
		return 0;
	}
}

/*************************************************************************************/
/*************************************************************************************/
/***        Load Bitmaps And Convert To Textures                                  ****/
/*************************************************************************************/
void LoadGLTextures() {	
	/* Load Texture*/
	Image *image1, *image2, *image3, *image4, *image5;

	/* allocate space for texture*/
	image1 = (Image *) malloc(sizeof(Image));
	if (image1 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	image2 = (Image *) malloc(sizeof(Image));
	if (image2 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	image3 = (Image *) malloc(sizeof(Image));
	if (image3 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	image4 = (Image *) malloc(sizeof(Image));
	if (image4 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}
	image5 = (Image *) malloc(sizeof(Image));
	if (image5 == NULL) {
		printf("Error allocating space for image");
		exit(0);
	}

	if (!ImageLoad("Data/Marble.bmp", image1)) {
		exit(1);
	} 
	if (!ImageLoad("Data/Wand.bmp", image2)) {
		exit(1);
	}
	if (!ImageLoad("Data/Boden.bmp", image3)) {
		exit(1);
	} 
	if (!ImageLoad("Data/Glass.bmp", image4)) {
		exit(1);
	}
	if (!ImageLoad("Data/Spark.bmp", image5)) {
		exit(1);
	}
	/* Create Texture	*****************************************/
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);   /* 2d texture (x and y size)*/

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, */
	/* border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image1->sizeX, image1->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image1->data);

	/* Create Texture	******************************************/
	glGenTextures(1, &texture[1]);
	glBindTexture(GL_TEXTURE_2D, texture[1]);   /* 2d texture (x and y size)*/

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, */
	/* border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image2->sizeX, image2->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image2->data);


	/* Create Texture	********************************************/
	glGenTextures(1, &texture[2]);
	glBindTexture(GL_TEXTURE_2D, texture[2]);   /* 2d texture (x and y size)*/

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, */
	/* border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image3->sizeX, image3->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image3->data);

	/* Create Texture	*********************************************/
	glGenTextures(1, &texture[3]);
	glBindTexture(GL_TEXTURE_2D, texture[3]);   /* 2d texture (x and y size)*/

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, */
	/* border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image4->sizeX, image4->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image4->data);

	/* Create Texture	*********************************************/
	glGenTextures(1, &texture[4]);
	glBindTexture(GL_TEXTURE_2D, texture[4]);   /* 2d texture (x and y size)*/

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); /* scale linearly when image bigger than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); /* scale linearly when image smalled than texture*/
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, */
	/* border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image5->sizeX, image5->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image5->data);

	free(image1->data);
	free(image1);
	free(image2->data);
	free(image2);
	free(image3->data);
	free(image3);
	free(image4->data);
	free(image4);
	free(image5->data);
	free(image5);

};

int ProcessKeys()
{
	if (keys[VK_DOWN])  pos+=TVector(0,0,10);
	if (keys[VK_LEFT] && paddle._Position.X() > -1*PLANE_POS+PADDLE_LENGTH)
	{
		paddle._Position+=TVector(-2,0,0);
		paddle._Point[0]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
		paddle._Point[1]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
		paddle._Point[2]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);
		paddle._Point[3]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);
	}
	if (keys[VK_RIGHT]&& paddle._Position.X() < PLANE_POS-PADDLE_LENGTH)
	{
		paddle._Position+=TVector(2,0,0);
		paddle._Point[0]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
		paddle._Point[1]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()-PADDLE_HEIGHT,0);
		paddle._Point[2]=TVector(paddle._Position.X()+PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);
		paddle._Point[3]=TVector(paddle._Position.X()-PADDLE_LENGTH,paddle._Position.Y()+PADDLE_HEIGHT,0);
	}
	if (keys[VK_F3]) 
	{   
		Time+=0.1;
		keys[VK_F3]=FALSE;
	}
	if (keys[VK_F2])
	{
		if(Time>0.1)
			Time-=0.1;
		keys[VK_F2]=FALSE;
	}
	if (keys[VK_F1])						// Is F1 Being Pressed?
	{
		keys[VK_F1]=FALSE;					// If So Make Key FALSE
		KillGLWindow();						// Kill Our Current Window
		fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
		// Recreate Our OpenGL Window
		if (!CreateGLWindow("ARKANOID",640,480,16,fullscreen))
		{
			return 0;						// Quit If Window Was Not Created
		}
	}

	return 1;
}

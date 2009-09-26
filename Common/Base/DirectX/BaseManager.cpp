/*!
	@file
	@author		Albert Semenov
	@date		05/2009
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "precompiled.h"
#include "BaseManager.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#	include <windows.h>
#endif

// ��� ������ ����
const char * WND_CLASS_NAME = "MyGUI_DirectX_Demo_window";

LRESULT CALLBACK DXWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_CREATE:
		{
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);
			break;
		}

		case WM_MOVE:
		case WM_SIZE:
		{
			base::BaseManager *baseManager = (base::BaseManager*)GetWindowLongPtr(hWnd, GWL_USERDATA);
			if (baseManager) baseManager->_windowResized();
				break;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}

		default:
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

namespace base
{

	BaseManager * BaseManager::m_instance = nullptr;
	BaseManager & BaseManager::getInstance()
	{
		assert(m_instance);
		return *m_instance;
	}

	BaseManager::BaseManager() :
		mGUI(nullptr),
		mPlatform(nullptr),
		mInfo(nullptr),
		hwnd(0),
		d3d(nullptr),
		device(nullptr),
		m_exit(false),
		mResourceFileName("core.xml")
	{
		assert(!m_instance);
		m_instance = this;
	}

	BaseManager::~BaseManager()
	{
		m_instance = nullptr;
	}

	void BaseManager::_windowResized()
	{
		RECT rect = { 0, 0, 0, 0 };
		GetClientRect(hwnd, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (mGUI)
		{
			mGUI->resizeWindow(MyGUI::IntSize(width, height));
		}

		if (mMouse)
		{
			const OIS::MouseState &ms = mMouse->getMouseState();
			ms.width = width;
			ms.height = height;
		}
	}

	void BaseManager::createInput() // ������� ������� �����
	{
		OIS::ParamList pl;
		size_t windowHnd = 0;
		std::ostringstream windowHndStr;

		void *pData = &windowHnd;
		HWND *pHWnd = (HWND*)pData;
		*pHWnd = hwnd;

		windowHndStr << windowHnd;
		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

		mInputManager = OIS::InputManager::createInputSystem( pl );

		mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
		mKeyboard->setEventCallback(this);

		mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, true ));
		mMouse->setEventCallback(this);

		_windowResized();
	}

	void BaseManager::destroyInput() // ������� ������� �����
	{
		if( mInputManager )
		{
			if (mMouse)
			{
				mInputManager->destroyInputObject( mMouse );
				mMouse = nullptr;
			}
			if (mKeyboard)
			{
				mInputManager->destroyInputObject( mKeyboard );
				mKeyboard = nullptr;
			}
			OIS::InputManager::destroyInputSystem(mInputManager);
			mInputManager = nullptr;
		}
	}

	bool BaseManager::mouseMoved(const OIS::MouseEvent& _arg)
	{
		if (mGUI)
			return injectMouseMove(_arg.state.X.abs, _arg.state.Y.abs, _arg.state.Z.abs);
		return true;
	}

	bool BaseManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
	{
		if (mGUI)
			return injectMousePress(_arg.state.X.abs, _arg.state.Y.abs, MyGUI::MouseButton::Enum(_id));
		return true;
	}

	bool BaseManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
	{
		if (mGUI)
			return injectMouseRelease(_arg.state.X.abs, _arg.state.Y.abs, MyGUI::MouseButton::Enum(_id));
		return true;
	}

	bool BaseManager::keyPressed(const OIS::KeyEvent& _arg)
	{
		if (mGUI)
			return injectKeyPress(MyGUI::KeyCode::Enum(_arg.key), (MyGUI::Char)_arg.text);
		return true;
	}

	bool BaseManager::keyReleased(const OIS::KeyEvent& _arg)
	{
		if (mGUI)
			return injectKeyRelease(MyGUI::KeyCode::Enum(_arg.key));
		return true;
	}

	bool BaseManager::create()
	{
		// ������������ ����� ����
		WNDCLASS wc = {
			0, (WNDPROC)DXWndProc, 0, 0, GetModuleHandle(NULL), LoadIcon(NULL, IDI_APPLICATION),
			LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, TEXT(WND_CLASS_NAME),
		};
		RegisterClass(&wc);

		// ������� ������� ����
		hwnd = CreateWindow(wc.lpszClassName, TEXT("MyGUI Demo [Direct3D9]"), WS_POPUP,
		0, 0, 0, 0, GetDesktopWindow(), NULL, wc.hInstance, NULL);
		if (!hwnd)
		{
			//OutException("fatal error!", "failed create window");
			return false;
		}

		hInstance = wc.hInstance;

		// ������������� direct3d
		d3d = Direct3DCreate9(D3D_SDK_VERSION);

		D3DDISPLAYMODE d3ddm;
		d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

		const unsigned int width = 1024;
		const unsigned int height = 768;

		d3dpp;
		memset(&d3dpp, 0, sizeof(d3dpp));
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.BackBufferCount  = 1;
		d3dpp.BackBufferFormat = d3ddm.Format;
		d3dpp.BackBufferWidth  = width;
		d3dpp.BackBufferHeight = height;
		d3dpp.hDeviceWindow = hwnd;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		d3dpp.Windowed = TRUE;

		if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &device)))
		{
			//OutException("fatal error!", "failed create d3d9 device");
			return false;
		}

		windowAdjustSettings(hwnd, width, height, !d3dpp.Windowed);

		createInput();
		createGui();
		createScene();

		return true;
	}

	void BaseManager::run()
	{
		MSG msg;
		for (;;)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (m_exit)
				break;
			else if (msg.message == WM_QUIT)
				break;

			if (GetActiveWindow() == hwnd)
			{
				if (mMouse) mMouse->capture();
				mKeyboard->capture();
				injectFrameEntered();

				// �������� ��������� ����������
				HRESULT hr = device->TestCooperativeLevel();
				if (SUCCEEDED(hr))
				{
					if (SUCCEEDED(device->BeginScene()))
					{
						device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
						mPlatform->getRenderManagerPtr()->drawOneFrame();
						device->EndScene();
					}
					device->Present(NULL, NULL, 0, NULL);
				}
				else
				{
					if (hr == D3DERR_DEVICENOTRESET)
					{
						//gui->deviceLost();
						if (SUCCEEDED(device->Reset(&d3dpp)))
						{
							//gui->deviceReset();
							Sleep(10);
						}
					}
				}
			}
		}
	}

	void BaseManager::destroy() // ������� ��� ��������� ������� ����������
	{

		destroyScene();
		destroyGui();

		destroyInput();

		if (device) { device->Release(); device = 0; }
		if (d3d) { d3d->Release(); d3d = 0; }

		if (hwnd)
		{
			DestroyWindow(hwnd);
			hwnd = 0;
		}

		//MyGUI_d.lib DirectXRenderSystem_d.lib
		UnregisterClass(WND_CLASS_NAME, hInstance);
	}

	void BaseManager::setupResources() // ��������� ��� ������� ����������
	{
		MyGUI::xml::Document doc;

		if (!doc.open(std::string("resources.xml")))
			doc.getLastError();

		MyGUI::xml::ElementPtr root = doc.getRoot();
		if (root == nullptr || root->getName() != "Paths")
			return;

		MyGUI::xml::ElementEnumerator node = root->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Path")
			{
				bool root = false;
				if (node->findAttribute("root") != "")
				{
					root = MyGUI::utility::parseBool(node->findAttribute("root"));
					if (root) mRootMedia = node->getContent();
				}
				addResourceLocation(node->getContent(), false);
			}
		}
	}

	void BaseManager::createGui()
	{
		mPlatform = new MyGUI::DirectXPlatform();
		mPlatform->initialise(device);

		setupResources();

		mGUI = new MyGUI::Gui();
		mGUI->initialise(mResourceFileName);

		mInfo = new statistic::StatisticInfo();
	}

	void BaseManager::destroyGui()
	{
		if (mGUI)
		{
			if (mInfo)
			{
				delete mInfo;
				mInfo = nullptr;
			}

			mGUI->shutdown();
			delete mGUI;
			mGUI = nullptr;
		}

		if (mPlatform)
		{
			mPlatform->shutdown();
			delete mPlatform;
			mPlatform = nullptr;
		}
	}

	void BaseManager::setWindowCaption(const std::string & _text)
	{
		SetWindowText(hwnd, _text.c_str());
	}

	void BaseManager::prepare(int argc, char **argv)
	{
	}

	void BaseManager::addResourceLocation(const std::string & _name, bool _recursive)
	{
		mPlatform->getDataManagerPtr()->addResourceLocation(_name, _recursive);
	}

	void BaseManager::windowAdjustSettings(HWND hWnd, int width, int height, bool fullScreen)
	{
		// ��������� ��������� ��������, ��� ���������������� ����
		static int desk_width  = GetSystemMetrics(SM_CXSCREEN);
		static int desk_height = GetSystemMetrics(SM_CYSCREEN);

		// ����� ����
		HWND hwndAfter;
		unsigned long style, style_ex;

		RECT rc;
		SetRect(&rc, 0, 0, width, height);

		if (fullScreen)
		{
			style = WS_POPUP | WS_VISIBLE;
			style_ex = GetWindowLong(hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST);
			hwndAfter = HWND_TOPMOST;
		}
		else
		{
			style = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
			style_ex = GetWindowLong(hWnd, GWL_EXSTYLE) &(~WS_EX_TOPMOST);
			hwndAfter = HWND_NOTOPMOST;
			AdjustWindowRect(&rc, style, false);
		}

		SetWindowLong(hWnd, GWL_STYLE,   style);
		SetWindowLong(hWnd, GWL_EXSTYLE, style_ex);

		unsigned int x, y, w, h;
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
		x = fullScreen ? 0 : (desk_width  - w) / 2;
		y = fullScreen ? 0 : (desk_height - h) / 2;

		SetWindowPos(hWnd, hwndAfter, x, y, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	void BaseManager::injectFrameEntered()
	{
		static unsigned long last_time = 0;
		static MyGUI::Timer timer;
		unsigned long now_time = timer.getMilliseconds();
		unsigned long time = now_time - last_time;

		mGUI->injectFrameEntered((float)((double)(time) / (double)1000));

		last_time = now_time;

		if (mInfo)
		{
			// calc FPS
			const unsigned long interval = 1000; 
			static unsigned long accumulate = 0;
			static int count_frames = 0;
			accumulate += time;
			if (accumulate > interval)
			{
				mInfo->change("FPS", (int)((unsigned long)count_frames * 1000 / accumulate));
				mInfo->update();

				accumulate = 0;
				count_frames = 0;
			}
			count_frames ++;
		}
	}

	bool BaseManager::injectMouseMove(int _absx, int _absy, int _absz)
	{
		mGUI->injectMouseMove(_absx, _absy, _absz);
		return true;
	}

	bool BaseManager::injectMousePress(int _absx, int _absy, MyGUI::MouseButton _id)
	{
		mGUI->injectMousePress(_absx, _absy, _id);
		return true;
	}

	bool BaseManager::injectMouseRelease(int _absx, int _absy, MyGUI::MouseButton _id)
	{
		mGUI->injectMouseRelease(_absx, _absy, _id);
		return true;
	}

	bool BaseManager::injectKeyPress(MyGUI::KeyCode _key, MyGUI::Char _text)
	{
		if (_key == MyGUI::KeyCode::Escape)
		{
			m_exit = true;
			return false;
		}
		else if (_key == MyGUI::KeyCode::F12)
		{
			bool visible = MyGUI::InputManager::getInstance().getShowFocus();
			MyGUI::InputManager::getInstance().setShowFocus(!visible);
		}

		mGUI->injectKeyPress(_key, _text);
		return true;
	}

	bool BaseManager::injectKeyRelease(MyGUI::KeyCode _key)
	{
		mGUI->injectKeyRelease(_key);
		return true;
	}

} // namespace base

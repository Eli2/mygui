/*!
	@file
	@author		Albert Semenov
	@date		08/2008
	@module
*/
#include "MyGUI_ScrollView.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_SubWidgetTextInterface.h"
#include "MyGUI_CastWidget.h"
#include "MyGUI_VScroll.h"
#include "MyGUI_HScroll.h"

namespace MyGUI
{

	Ogre::String ScrollView::WidgetTypeName = "ScrollView";

	const int SCROLL_VIEW_MOUSE_WHEEL = 50; // ����������� �������� ��� ������ ����
	const int SCROLL_VIEW_SCROLL_PAGE = 16; // ����������� �������� ��� ������ ������

	ScrollView::ScrollView(const IntCoord& _coord, Align _align, const WidgetSkinInfoPtr _info, CroppedRectanglePtr _parent, WidgetCreator * _creator, const Ogre::String & _name) :
		Widget(_coord, _align, _info, _parent, _creator, _name),
		mIsPressed(false),
		mIsFocus(false),
		mVScroll(null),
		mHScroll(null),
		mShowHScroll(true),
		mShowVScroll(true),
		mVRange(0),
		mHRange(0),
		mWidgetCanvas(null),
		mAlignCanvas(Align::Center)
	{
		// ��� ����� ����� �����
		mNeedKeyFocus = true;

		for (VectorWidgetPtr::iterator iter=mWidgetChild.begin(); iter!=mWidgetChild.end(); ++iter) {
			if ((*iter)->_getInternalString() == "Client") {
				MYGUI_DEBUG_ASSERT( ! mWidgetClient, "widget already assigned");
				mWidgetClient = (*iter);
				mWidgetClient->eventMouseSetFocus = newDelegate(this, &ScrollView::notifyMouseSetFocus);
				mWidgetClient->eventMouseLostFocus = newDelegate(this, &ScrollView::notifyMouseLostFocus);
				mWidgetClient->eventMouseWheel = newDelegate(this, &ScrollView::notifyMouseWheel);

				// ������� ����, �������� �������� �����
				mWidgetCanvas = mWidgetClient->createWidget<Widget>("Default", IntCoord(), Align::Default);
				mWidgetCanvas->eventMouseWheel = newDelegate(this, &ScrollView::notifyMouseWheel);
				mWidgetCanvas->eventMouseSetFocus = newDelegate(this, &ScrollView::notifyMouseSetFocus);
				mWidgetCanvas->eventMouseLostFocus = newDelegate(this, &ScrollView::notifyMouseLostFocus);
			}
			else if ((*iter)->_getInternalString() == "VScroll") {
				MYGUI_DEBUG_ASSERT( ! mVScroll, "widget already assigned");
				mVScroll = castWidget<VScroll>(*iter);
				mVScroll->eventScrollChangePosition = newDelegate(this, &ScrollView::notifyScrollChangePosition);
			}
			else if ((*iter)->_getInternalString() == "HScroll") {
				MYGUI_DEBUG_ASSERT( ! mHScroll, "widget already assigned");
				mHScroll = castWidget<HScroll>(*iter);
				mHScroll->eventScrollChangePosition = newDelegate(this, &ScrollView::notifyScrollChangePosition);
			}
		}

		MYGUI_ASSERT(null != mWidgetClient, "Child Widget Client not found in skin (ScrollView must have Client)");

		updateView();
	}

	ScrollView::~ScrollView()
	{
	}

	void ScrollView::notifyMouseSetFocus(WidgetPtr _sender, WidgetPtr _old)
	{
		if ( (_old == mWidgetClient) || (mIsFocus) ) return;
		mIsFocus = true;
		updateScrollViewState();
	}

	void ScrollView::notifyMouseLostFocus(WidgetPtr _sender, WidgetPtr _new)
	{
		if ( (_new == mWidgetClient) || (false == mIsFocus) ) return;
		mIsFocus = false;
		updateScrollViewState();
	}

	void ScrollView::_onKeySetFocus(WidgetPtr _old)
	{
		if (false == mIsPressed) {
			mIsPressed = true;
			updateScrollViewState();
		}
		// !!! ����������� �������� � ����� ������
		Widget::_onKeySetFocus(_old);
	}

	void ScrollView::_onKeyLostFocus(WidgetPtr _new)
	{
		if (mIsPressed) {
			mIsPressed = false;
			updateScrollViewState();
		}
		// !!! ����������� �������� � ����� ������
		Widget::_onKeyLostFocus(_new);
	}

	void ScrollView::updateScrollViewState()
	{
		if (mIsFocus) {
			if (mIsPressed) setState("select");
			else setState("active");
		} else {
			if (mIsPressed) setState("pressed");
			else setState("normal");
		}
	}

	void ScrollView::setPosition(const IntCoord& _coord)
	{
		Widget::setPosition(_coord);
		updateView();
	}

	void ScrollView::setSize(const IntSize& _size)
	{
		Widget::setSize(_size);
		updateView();
	}

	void ScrollView::setTextAlign(Align _align)
	{
		Widget::setTextAlign(_align);
		// ��� ��� �� ���� ����� ����������
		updateView();
	}

	void ScrollView::updateView()
	{

		// ��������� ������
		updateScroll();

		// ������ ���������
		IntSize size = mWidgetCanvas->getSize();
		// ������� �������� ���������
		IntPoint point(-mWidgetCanvas->getLeft(), -mWidgetCanvas->getTop());
		// ��������� ��������
		IntPoint offset = point;

		if (size.width > mWidgetClient->getWidth()) {
			// ������������ ����� �����
			if ((offset.left + mWidgetClient->getWidth()) > size.width) {
				offset.left = size.width - mWidgetClient->getWidth();
			}
			// ������������ ����� ������
			else if (offset.left < 0) {
				offset.left = 0;
			}
		}
		else {
			if (mAlignCanvas.isLeft()) {
				offset.left = 0;
			}
			else if (mAlignCanvas.isRight()) {
				offset.left = size.width - mWidgetClient->getWidth();
			}
			else {
				offset.left = (size.width - mWidgetClient->getWidth()) / 2;
			}
		}

		if (size.height > mWidgetClient->getHeight()) {
			// ������������ ����� �����
			if ((offset.top + mWidgetClient->getHeight()) > size.height) {
				offset.top = size.height - mWidgetClient->getHeight();
			}
			// ������������ ����� ����
			else if (offset.top < 0) {
				offset.top = 0;
			}
		}
		else {
			if (mAlignCanvas.isTop()) {
				offset.top = 0;
			}
			else if (mAlignCanvas.isBottom()) {
				offset.top = size.height - mWidgetClient->getHeight();
			}
			else {
				offset.top = (size.height - mWidgetClient->getHeight()) / 2;
			}
		}
		
		if (offset != point) {
			if (null != mVScroll) mVScroll->setScrollPosition(offset.top);
			if (null != mHScroll) mHScroll->setScrollPosition(offset.left);
			mWidgetCanvas->setPosition(-offset.left, -offset.top);
		}
	}

	void ScrollView::updateScroll()
	{
		IntSize size = mWidgetCanvas->getSize();

		// ������������ �� ����������
		if (size.height > mWidgetClient->getHeight()) {
			if (mVScroll != null) {
				if (( ! mVScroll->isShow()) && (mShowVScroll)) {
					mVScroll->show();
					mWidgetClient->setSize(mWidgetClient->getWidth() - mVScroll->getWidth(), mWidgetClient->getHeight());

					// ������ ����� ����������
					//size = mWidgetCanvas->getSize();

					if (mHScroll != null) {
						mHScroll->setSize(mHScroll->getWidth() - mVScroll->getWidth(), mHScroll->getHeight());

						// ���� �������� ������������ ����� ���, ����������� ��� �� �����������,
						// ������������� �������������� ����� �� ������� ������
						if ((size.width > mWidgetClient->getWidth()) && ( ! mHScroll->isShow()) && (mShowHScroll)) {
							mHScroll->show();
							mWidgetClient->setSize(mWidgetClient->getWidth(), mWidgetClient->getHeight() - mHScroll->getHeight());
							mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() - mHScroll->getHeight());

							// ������ ����� ����������
							//size = mWidgetCanvas->getSize();
						}
					}
				}
			}
		}
		// ������������ ����������
		else {
			if (mVScroll != null) {
				if (mVScroll->isShow()) {
					mVScroll->hide();
					mWidgetClient->setSize(mWidgetClient->getWidth() + mVScroll->getWidth(), mWidgetClient->getHeight());

					// ������ ����� ����������
					//size = mWidgetCanvas->getSize();

					if (mHScroll != null) {
						mHScroll->setSize(mHScroll->getWidth() + mVScroll->getWidth(), mHScroll->getHeight());

						// ���� ������ ������������ ����� ���, ����������� ��� �� �����������,
						// ������������� �������������� ����� �� ������� �������
						if ((size.width <= mWidgetClient->getWidth()) && (mHScroll->isShow())) {
							mHScroll->hide();
							mWidgetClient->setSize(mWidgetClient->getWidth(), mWidgetClient->getHeight() + mHScroll->getHeight());
							mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() + mHScroll->getHeight());

							// ������ ����� ����������
							//size = mWidgetCanvas->getSize();
						}
					}
				}
			}
		}


		// �������������� �� ����������
		if (size.width > mWidgetClient->getWidth()) {
			if (mHScroll != null) {
				if (( ! mHScroll->isShow()) && (mShowHScroll)) {
					mHScroll->show();
					mWidgetClient->setSize(mWidgetClient->getWidth(), mWidgetClient->getHeight() - mHScroll->getHeight());

					// ������ ����� ����������
					//size = mWidgetCanvas->getSize();

					if (mVScroll != null) {
						mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() - mHScroll->getHeight());

						// ���� �������� �������������� ����� ���, ����������� ��� �� ���������,
						// ������������� ������������ ����� �� ������� ������
						if ((size.height > mWidgetClient->getHeight()) && ( ! mVScroll->isShow()) && (mShowVScroll)) {
							mVScroll->show();
							mWidgetClient->setSize(mWidgetClient->getWidth() - mVScroll->getWidth(), mWidgetClient->getHeight());
							mHScroll->setSize(mHScroll->getWidth() - mVScroll->getWidth(), mHScroll->getHeight());

							// ������ ����� ����������
							//size = mWidgetCanvas->getSize();
						}
					}
				}
			}
		}
		// �������������� ����������
		else {
			if (mHScroll != null) {
				if (mHScroll->isShow()) {
					mHScroll->hide();
					mWidgetClient->setSize(mWidgetClient->getWidth(), mWidgetClient->getHeight() + mHScroll->getHeight());

					// ������ ����� ����������
					//size = mWidgetCanvas->getSize();

					if (mVScroll != null) {
						mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() + mHScroll->getHeight());

						// ���� ������ �������������� ����� ���, ����������� ��� �� ���������,
						// ������������� ������������ ����� �� ������� �������
						if ((size.height <= mWidgetClient->getHeight()) && (mVScroll->isShow())) {
							mVScroll->hide();
							mWidgetClient->setSize(mWidgetClient->getWidth() + mVScroll->getWidth(), mWidgetClient->getHeight());
							mHScroll->setSize(mHScroll->getWidth() + mVScroll->getWidth(), mHScroll->getHeight());

							// ������ ����� ����������
							//size = mWidgetCanvas->getSize();
						}
					}
				}
			}
		}

		mVRange = (mWidgetClient->getHeight() >= size.height) ? 0 : size.height - mWidgetClient->getHeight();
		mHRange = (mWidgetClient->getWidth() >= size.width) ? 0 : size.width - mWidgetClient->getWidth();

		size_t page = SCROLL_VIEW_SCROLL_PAGE;
		if (mVScroll != null) {
			mVScroll->setScrollPage(page);
			mVScroll->setScrollViewPage(mCoord.width > (int)page ? mCoord.width : page);
			mVScroll->setScrollRange(mVRange + 1);
			if (size.height) mVScroll->setTrackSize(1. * mVScroll->getLineSize() * mWidgetClient->getHeight() / size.height);
		}
		if (mHScroll != null) {
			mHScroll->setScrollPage(page);
			mHScroll->setScrollViewPage(mCoord.height > (int)page ? mCoord.height : page);
			mHScroll->setScrollRange(mHRange + 1);
			if (size.width) mHScroll->setTrackSize(1. * mHScroll->getLineSize() * mWidgetClient->getWidth() / size.width);
		}

	}

	void ScrollView::notifyScrollChangePosition(WidgetPtr _sender, size_t _position)
	{
		if (_sender == mVScroll) {
			IntPoint point = mWidgetCanvas->getPosition();
			point.top = -(int)_position;
			mWidgetCanvas->setPosition(point);
		}
		else if (_sender == mHScroll) {
			IntPoint point = mWidgetCanvas->getPosition();
			point.left = -(int)_position;
			mWidgetCanvas->setPosition(point);
		}
	}

	void ScrollView::notifyMouseWheel(WidgetPtr _sender, int _rel)
	{
		if (mVRange != 0) {
			IntPoint point = mWidgetCanvas->getPosition();
			int offset = -point.top;
			if (_rel < 0) offset += SCROLL_VIEW_MOUSE_WHEEL;
			else  offset -= SCROLL_VIEW_MOUSE_WHEEL;

			if (offset < 0) offset = 0;
			else if (offset > (int)mVRange) offset = mVRange;

			if (offset != point.top) {
				point.top = -offset;
				if (mVScroll != null) {
					mVScroll->setScrollPosition(offset);
				}
				mWidgetCanvas->setPosition(point);
			}
		}
		else if (mHRange != 0) {
			IntPoint point = mWidgetCanvas->getPosition();
			int offset = -point.left;
			if (_rel < 0) offset += SCROLL_VIEW_MOUSE_WHEEL;
			else  offset -= SCROLL_VIEW_MOUSE_WHEEL;

			if (offset < 0) offset = 0;
			else if (offset > (int)mHRange) offset = mHRange;

			if (offset != point.left) {
				point.left = -offset;
				if (mHScroll != null) {
					mHScroll->setScrollPosition(offset);
				}
				mWidgetCanvas->setPosition(point);
			}
		}
	}

	WidgetPtr ScrollView::_createWidget(const std::string & _type, const std::string & _skin, const IntCoord& _coord, Align _align, const std::string & _layer, const std::string & _name)
	{
		return mWidgetCanvas->createWidgetT(_type, _skin, _coord, _align, _name);
	}

	EnumeratorWidgetPtr ScrollView::getEnumerator()
	{
		return mWidgetCanvas->getEnumerator();
	}

} // namespace MyGUI

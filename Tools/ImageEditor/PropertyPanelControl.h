/*!
	@file
	@author		Albert Semenov
	@date		07/2012
*/

#ifndef _a90345b7_27c4_4613_9148_281c8f996ded_
#define _a90345b7_27c4_4613_9148_281c8f996ded_

#include "Control.h"
#include "sigslot.h"
#include "Data.h"
#include "PropertyControl.h"

namespace tools
{

	class PropertyPanelControl :
		public Control,
		public sigslot::has_slots<>
	{
	public:
		PropertyPanelControl();
		virtual ~PropertyPanelControl();

		void setCurrentData(Data* _data);

	protected:
		virtual void OnInitialise(Control* _parent, MyGUI::Widget* _place, const std::string& _layoutName);

	private:
		void HideControls();
		void InitialiseProperty(Property* _property, int& _height);

	private:
		Data* mCurrentData;
		typedef std::vector<std::pair<std::string, PropertyControl*> > VectorPairControl;
		VectorPairControl mPropertyControls;
		int mDistance;
	};

}

#endif
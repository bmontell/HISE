/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef SLIDERPACK_H_INCLUDED
#define SLIDERPACK_H_INCLUDED

namespace hise { using namespace juce;

/** The data model for a SliderPack component. */
class SliderPackData: public SafeChangeBroadcaster
{
public:

	SliderPackData(UndoManager* undoManager, PooledUIUpdater* updater);

	~SliderPackData();

	void setRange(double minValue, double maxValue, double stepSize);

	Range<double> getRange() const;

	void startDrag()
	{
		if (undoManager != nullptr)
		{
			undoManager->beginNewTransaction();
		}
	}

	double getStepSize() const;

	void setNumSliders(int numSliders);

	int getNumSliders() const;

	void setValue(int sliderIndex, float value, NotificationType notifySliderPack=dontSendNotification, bool useUndoManager=false);

	float getValue(int index) const;

	void setFromFloatArray(const Array<float> &valueArray);

	void writeToFloatArray(Array<float> &valueArray) const;

	String toBase64() const;

	void fromBase64(const String &encodedValues);

	int getNextIndexToDisplay() const
	{
		return nextIndexToDisplay;
	}

	void swapData(Array<var> &otherData)
	{
		{
			ScopedWriteLock sl(arrayLock);
			values = var(otherData);
		}
		
		sendChangeMessage();
	}

	void setDisplayedIndex(int index)
	{
		if (index != nextIndexToDisplay)
		{
			nextIndexToDisplay = index;
			sendPooledChangeMessage();
		}
	}

	const float* getCachedData()
	{
		int size = getNumSliders();
		cachedData.buffer.setSize(1, size);

		for (int i = 0; i < size; i++)
		{
			cachedData.setSample(i, getValue(i));
		}

		return cachedData.buffer.getReadPointer(0);
	}

	var getDataArray() const { return values; }

	void setFlashActive(bool shouldBeShown) { flashActive = shouldBeShown; };
	void setShowValueOverlay(bool shouldBeShown) { showValueOverlay = shouldBeShown; };

	bool isFlashActive() const { return flashActive; }
	bool isValueOverlayShown() const { return showValueOverlay; }

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = var(newDefaultValue);
	}

	void setUndoManager(UndoManager* managerToUse)
	{
		undoManager = managerToUse;
	}

	void setNewUndoAction() const;
private:

	struct SliderPackAction : public UndoableAction
	{
		SliderPackAction(SliderPackData* data_, int sliderIndex_, float oldValue_, float newValue_, NotificationType n_) :
			UndoableAction(),
			data(data_),
			sliderIndex(sliderIndex_),
			oldValue(oldValue_),
			newValue(newValue_),
			n(n_)
		{};

		bool perform() override
		{
			if (data != nullptr)
			{
				data->setValue(sliderIndex, newValue, n, false);
				return true;
			}

			return false;
		}

		bool undo() override
		{
			if (data != nullptr)
			{
				data->setValue(sliderIndex, oldValue, n, false);
				return true;
			}

			return false;
		}

		WeakReference<SliderPackData> data;
		int sliderIndex;
		float oldValue, newValue;
		NotificationType n;
	};

	

	ReadWriteLock arrayLock;

	UndoManager* undoManager;

	bool flashActive;
	bool showValueOverlay;

	VariantBuffer cachedData;
	
	WeakReference<SliderPackData>::Master masterReference;

	friend class WeakReference < SliderPackData > ;

	int nextIndexToDisplay;
	
	Range<double> sliderRange;

	double stepSize;

	var values;

	var defaultValue;

	//Array<float> values;
};


/** A Component which contains multiple Sliders which support dragging & bipolar display. 
	@ingroup hise_ui
	
	This class is driven by the SliderPackData class, which acts as data container.
*/
class SliderPack : public Component,
				   public Slider::Listener,
				   public SafeChangeListener,
				   public Timer
{
public:

	/** Inherit from this class in order to get notified about changes to the slider pack. */
	class Listener
	{
	public:

		virtual ~Listener();

		/** Callback that will be executed when a slider is moved. You can get the actual value with SliderPack::getValue(int index). */
		virtual void sliderPackChanged(SliderPack *s, int index ) = 0;

	private:
		WeakReference<Listener>::Master masterReference;
		friend class WeakReference<Listener>;
		
	};

	SET_GENERIC_PANEL_ID("ArrayEditor");

	/** Creates a new SliderPack. */
	SliderPack(SliderPackData *data=nullptr);

	~SliderPack();

	/** Register a listener that will receive notification when the sliders are changed. */
	void addListener(Listener *listener)
	{
		listeners.addIfNotAlreadyThere(listener);
	}

	/** Removes a previously registered listener. */
	void removeListener(Listener *listener)
	{
		listeners.removeAllInstancesOf(listener);
	}

	void timerCallback() override;

	/** Sets the number of sliders shown. This clears all values. */
	void setNumSliders(int numSliders);

	/** Returns the value of the slider index. If the index is bigger than the slider amount, it will return -1. */
	double getValue(int sliderIndex);

	/** Sets the value of one of the sliders. If the index is bigger than the slider amount, it will do nothing. */
	void setValue(int sliderIndex, double newValue);

	void updateSliders();

	void changeListenerCallback(SafeChangeBroadcaster *b) override;

	void mouseDown(const MouseEvent &e) override;
	void mouseDrag(const MouseEvent &e) override;
	void mouseUp(const MouseEvent &e) override;
	void mouseDoubleClick(const MouseEvent &e) override;
	void mouseExit(const MouseEvent &e) override;

	void update();

	void sliderValueChanged(Slider *s) override;

	void notifyListeners(int index);

	void paintOverChildren(Graphics &g) override;

	void paint(Graphics &g);

	void setSuffix(const String &suffix);

	void setDisplayedIndex(int displayIndex);

	/** Sets the double click return value. */
	void setDefaultValue(double defaultValue);

	void setColourForSliders(int colourId, Colour c);

	const SliderPackData* getData() const { return data; }
	SliderPackData* getData() { return data; }

	void resized() override;
	void setValuesFromLine();

	/** Returns the number of slider. */
	int getNumSliders();

	void setFlashActive(bool setFlashActive);
	void setShowValueOverlay(bool shouldShowValueOverlay);
	void setStepSize(double stepSize);
    
	/** Set the slider widths to the given proportions. 
		
		For example { 0.25, 0.5, 0.25 } will make the middle slider twice as big. 
	*/
    void setSliderWidths(const Array<var>& newWidths)
    {
        sliderWidths = newWidths;
        resized();
    }
    
private:

	int currentDisplayIndex = -1;

	int getSliderIndexForMouseEvent(const MouseEvent& e);
    
	SliderPackData dummyData;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	String suffix;

	double defaultValue;

	Array<float> displayAlphas;

    Array<var> sliderWidths;
    
	Line<float> rightClickLine;

	bool currentlyDragged;

	int currentlyDraggedSlider;

	double currentlyDraggedSliderValue;

	BiPolarSliderLookAndFeel laf;

	WeakReference<SliderPackData> data;

	OwnedArray<Slider> sliders;

};


} // namespace hise
#endif  // SLIDERPACK_H_INCLUDED

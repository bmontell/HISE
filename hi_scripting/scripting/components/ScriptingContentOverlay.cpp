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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



namespace OverlayIcons
{
	static const unsigned char lockShape[] = { 110,109,41,100,31,68,33,48,94,67,98,156,188,33,68,33,48,94,67,248,163,35,68,211,205,101,67,248,163,35,68,92,47,111,67,108,248,163,35,68,223,111,184,67,98,248,163,35,68,164,32,189,67,139,188,33,68,125,239,192,67,41,100,31,68,125,239,192,67,108,37,182,
		213,67,125,239,192,67,98,96,5,209,67,125,239,192,67,135,54,205,67,164,32,189,67,135,54,205,67,223,111,184,67,108,135,54,205,67,92,47,111,67,98,135,54,205,67,211,205,101,67,96,5,209,67,33,48,94,67,37,182,213,67,33,48,94,67,108,41,100,31,68,33,48,94,67,
		99,109,166,91,248,67,68,11,76,67,108,166,171,219,67,68,11,76,67,108,166,171,219,67,160,186,20,67,108,137,129,219,67,160,186,20,67,108,137,129,219,67,184,126,20,67,98,137,129,219,67,254,20,196,66,172,252,239,67,229,80,100,66,84,155,4,68,229,80,100,66,
		98,98,56,17,68,229,80,100,66,227,117,27,68,254,20,196,66,227,117,27,68,184,126,20,67,108,227,117,27,68,160,186,20,67,108,49,112,27,68,160,186,20,67,108,49,112,27,68,193,234,76,67,108,41,28,13,68,193,234,76,67,108,41,28,13,68,160,186,20,67,108,229,24,
		13,68,160,186,20,67,98,229,24,13,68,168,166,20,67,246,24,13,68,176,146,20,67,246,24,13,68,184,126,20,67,98,246,24,13,68,0,192,1,67,242,74,9,68,98,16,229,66,84,155,4,68,98,16,229,66,98,35,235,255,67,98,16,229,66,133,91,248,67,66,128,1,67,231,59,248,67,
		180,8,20,67,108,166,91,248,67,180,8,20,67,108,166,91,248,67,68,11,76,67,99,101,0,0 };

	static const unsigned char penShape[] = { 110,109,96,69,112,67,182,243,141,64,108,154,73,133,67,143,194,240,65,98,158,95,136,67,201,118,16,66,59,111,136,67,92,15,56,66,172,108,133,67,125,191,80,66,108,51,179,122,67,100,123,137,66,108,240,7,74,67,172,28,170,65,108,20,46,90,67,82,184,150,64,98,
		51,51,96,67,12,2,187,191,88,25,106,67,131,192,202,191,96,69,112,67,182,243,141,64,99,109,14,173,62,67,164,240,1,66,108,113,29,111,67,213,120,159,66,108,127,42,171,66,0,32,109,67,108,117,147,20,66,190,223,61,67,108,14,173,62,67,164,240,1,66,99,109,236,
		81,200,65,121,9,75,67,108,123,148,145,66,53,158,121,67,108,0,0,0,0,74,60,138,67,108,236,81,200,65,121,9,75,67,99,101,0,0 };
};



ScriptEditHandler::ScriptEditHandler()
{

}

void ScriptEditHandler::createNewComponent(Widgets componentType, int x, int y)
{
	if (getScriptEditHandlerContent() == nullptr)
		return;

	if (getScriptEditHandlerEditor() == nullptr)
	{
		return;
	}

	String widgetType;

	switch (componentType)
	{
	case Widgets::Knob:				widgetType = "Knob"; break;
	case Widgets::Button:			widgetType = "Button"; break;
	case Widgets::Table:				widgetType = "Table"; break;
	case Widgets::ComboBox:			widgetType = "ComboBox"; break;
	case Widgets::Label:				widgetType = "Label"; break;
	case Widgets::Image:				widgetType = "Image"; break;
	case Widgets::Plotter:			widgetType = "Plotter"; break;
	case Widgets::ModulatorMeter:	widgetType = "ModulatorMeter"; break;
	case Widgets::Panel:				widgetType = "Panel"; break;
	case Widgets::AudioWaveform:		widgetType = "AudioWaveform"; break;
	case Widgets::SliderPack:		widgetType = "SliderPack"; break;
	case Widgets::duplicateWidget:
	{
		widgetType = getScriptEditHandlerContent()->getEditedComponent()->getObjectName().toString();
		widgetType = widgetType.replace("Scripted", "");
		widgetType = widgetType.replace("Script", "");
		widgetType = widgetType.replace("Slider", "Knob");
		break;
	}
	case Widgets::numWidgets: break;
	}

	String id = PresetHandler::getCustomName(widgetType);

	String errorMessage = isValidWidgetName(id);

	while (errorMessage.isNotEmpty() && PresetHandler::showYesNoWindow("Wrong variable name", errorMessage + "\nPress 'OK' to re-enter a valid variable name or 'Cancel' to abort", PresetHandler::IconType::Warning))
	{
		id = PresetHandler::getCustomName(widgetType);
		errorMessage = isValidWidgetName(id);
	}

	errorMessage = isValidWidgetName(id);

	if (errorMessage.isNotEmpty()) return;

	String textToInsert;

	textToInsert << "\nconst var " << id << " = Content.add" << widgetType << "(\"" << id << "\", " << x << ", " << y << ");\n";

	if (componentType == Widgets::duplicateWidget)
	{
		const int xOfOriginal = getScriptEditHandlerContent()->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::x);
		const int yOfOriginal = getScriptEditHandlerContent()->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::y);

		const String originalId = getScriptEditHandlerContent()->getEditedComponent()->getName().toString();

		if (getScriptEditHandlerEditor()->componentIsDefinedWithFactoryMethod(originalId))
		{
			textToInsert = getScriptEditHandlerEditor()->createNewDefinitionWithFactoryMethod(originalId, id, x, y);
		}
		else
		{
			String jsonDataOfNewComponent = CodeDragger::getText(getScriptEditHandlerContent()->getEditedComponent());

			jsonDataOfNewComponent = jsonDataOfNewComponent.replace(originalId, id); // change the id of the component
			jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"x\": " + String(xOfOriginal), "\"x\": " + String(x)); // change the id of the component
			jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"y\": " + String(yOfOriginal), "\"y\": " + String(y)); // change the id of the component

			textToInsert << jsonDataOfNewComponent;
		}
	}

	selectOnInitCallback();

	getScriptEditHandlerEditor()->moveCaretToEnd(false);

	getScriptEditHandlerEditor()->insertTextAtCaret(textToInsert);
	compileScript();

}

void ScriptEditHandler::setEditedScriptComponent(ReferenceCountedObject* component)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component);

	Component* scc = getScriptEditHandlerContent()->setEditedScriptComponent(sc);

	getScriptEditHandlerOverlay()->dragger->setDraggedControl(scc, sc);

	if (component != nullptr)
	{
		selectOnInitCallback();
	}
}

void ScriptEditHandler::toggleComponentSelectMode(bool shouldSelectOnClick)
{
	useComponentSelectMode = shouldSelectOnClick;

	getScriptEditHandlerContent()->setInterceptsMouseClicks(false, !useComponentSelectMode);
}

void ScriptEditHandler::changePositionOfComponent(ScriptingApi::Content::ScriptComponent* sc, int newX, int newY)
{
	const String regexMonster = "(Content\\.add\\w+\\s*\\(\\s*\\\"(" + sc->getName().toString() +
		")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*\\);)|(create\\w+\\s*\\(\\s*\\\"(" + sc->getName().toString() +
		")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*.*\\);)";

	CodeDocument* onInitC = getScriptEditHandlerProcessor()->getSnippet(0);

	String allText = onInitC->getAllContent();

	StringArray matches = RegexFunctions::getFirstMatch(regexMonster, allText);

	const bool isContentDefinition = matches[1].isNotEmpty();
	const bool isInlineDefinition = matches[7].isNotEmpty();

	if ((isContentDefinition || isInlineDefinition) && matches.size() > 12)
	{
		const String oldLine = matches[0];
		String replaceLine;

		if (isContentDefinition)
		{
			replaceLine << matches[1] << String(newX) << matches[4] << String(newY) << matches[6];
		}
		else
		{

			replaceLine << matches[7] << String(newX) << matches[10] << String(newY) << matches[12];
		}

		const int start = allText.indexOf(oldLine);
		const int end = start + oldLine.length();

		onInitC->replaceSection(start, end, replaceLine);

		sc->setDefaultPosition(newX, newY);
	}
}

void ScriptEditHandler::compileScript()
{
	ProcessorWithScriptingContent *s = dynamic_cast<ProcessorWithScriptingContent*>(getScriptEditHandlerProcessor());
	JavascriptProcessor* jsp = getScriptEditHandlerProcessor();
	Processor* p = dynamic_cast<Processor*>(getScriptEditHandlerProcessor());
	Component* thisAsComponent = dynamic_cast<Component*>(this);

	ReferenceCountedObject *component = s->checkContentChangedInPropertyPanel();

	if (component != nullptr)
	{
		if (!PresetHandler::showYesNoWindow("Discard changed properties?", "There are some properties for the component " + String(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component)->getName().toString()) + " that are not saved. Press OK to discard these changes or Cancel to abort compiling", PresetHandler::IconType::Warning))
		{
			p->getMainController()->setEditedScriptComponent(component, thisAsComponent);
			return;
		}
	}

	p->getMainController()->setEditedScriptComponent(nullptr, thisAsComponent);

	PresetHandler::setChanged(p);

	scriptEditHandlerCompileCallback();
}

void ScriptEditHandler::scriptComponentChanged(ReferenceCountedObject* scriptComponent, Identifier)
{
	if (getScriptEditHandlerEditor() == nullptr)
		return;

	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(scriptComponent);

	if (sc != nullptr)
	{
		const bool scriptComponentIsDefinedWithFactoryMethod = getScriptEditHandlerEditor()->componentIsDefinedWithFactoryMethod(sc->getName());

		if (scriptComponentIsDefinedWithFactoryMethod) return;

		if (!getScriptEditHandlerEditor()->selectJSONTag(sc->getName()))
		{
			getScriptEditHandlerEditor()->selectLineAfterDefinition(sc->getName());
		}
		getScriptEditHandlerEditor()->insertTextAtCaret(CodeDragger::getText(sc));

		getScriptEditHandlerEditor()->selectJSONTag(sc->getName());
	}
}

String ScriptEditHandler::isValidWidgetName(const String &id)
{
	if (id.isEmpty()) return "Identifier must not be empty";

	if (!Identifier::isValidIdentifier(id)) return "Identifier must not contain whitespace or weird characters";

	ScriptingApi::Content* content = dynamic_cast<ProcessorWithScriptingContent*>(getScriptEditHandlerProcessor())->getScriptingContent();

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		if (content->getComponentWithName(Identifier(id)) != nullptr)
			return  "Identifier " + id + " already exists";
	}

	return String();
}

ScriptingContentOverlay::ScriptingContentOverlay(ScriptEditHandler* parentHandler_) :
	parentHandler(parentHandler_)
{
	addAndMakeVisible(dragger = new Dragger(parentHandler));

	addAndMakeVisible(dragModeButton = new ShapeButton("Drag Mode", Colours::black.withAlpha(0.6f), Colours::black.withAlpha(0.8f), Colours::black.withAlpha(0.8f)));

	Path path;
	path.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));

	dragModeButton->setShape(path, true, true, false);

	dragModeButton->addListener(this);

	dragModeButton->setTooltip("Toggle between Edit / Performance mode");

	setEditMode(parentHandler->editModeEnabled());
}

void ScriptingContentOverlay::resized()
{
	dragModeButton->setBounds(getWidth() - 28, 12, 16, 16);
}


void ScriptingContentOverlay::buttonClicked(Button* /*buttonThatWasClicked*/)
{
	setEditMode(!dragMode);

	parentHandler->toggleComponentSelectMode(dragMode);
}

void ScriptingContentOverlay::setEditMode(bool editModeEnabled)
{
	dragMode = editModeEnabled;

	Path p;

	if (dragMode == false)
	{
		p.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));
		dragger->setDraggedControl(nullptr, nullptr);
		setInterceptsMouseClicks(false, true);
	}
	else
	{
		p.loadPathFromData(OverlayIcons::penShape, sizeof(OverlayIcons::penShape));
		setInterceptsMouseClicks(true, true);
	}

	dragModeButton->setShape(p, true, true, false);
	dragModeButton->setToggleState(dragMode, dontSendNotification);

	resized();
	repaint();
}

Rectangle<float> getFloatRectangle(const Rectangle<int> &r)
{
	return Rectangle<float>((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight());
}

void ScriptingContentOverlay::paint(Graphics& g)
{
	if (dragMode)
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillAll();

		const bool isInPopup = findParentComponentOfClass<ScriptingEditor>() == nullptr;

		Colour lineColour = isInPopup ? Colours::white : Colours::black;

		for (int x = 10; x < getWidth(); x += 10)
		{
			g.setColour(lineColour.withAlpha((x % 100 == 0) ? 0.12f : 0.05f));

			g.drawVerticalLine(x, 0.0f, (float)getHeight());
		}

		for (int y = 10; y < getHeight(); y += 10)
		{
			g.setColour(lineColour.withAlpha(((y) % 100 == 0) ? 0.1f : 0.05f));
			g.drawHorizontalLine(y, 0.0f, (float)getWidth());
		}
	}

	Colour c = Colours::white;

	g.setColour(c.withAlpha(0.2f));

	g.fillRoundedRectangle(getFloatRectangle(dragModeButton->getBounds().expanded(3)), 3.0f);
}


class ParameterConnector : public ThreadWithAsyncProgressWindow,
	public ComboBoxListener,
	public Timer
{
public:

	ParameterConnector(ScriptingApi::Content::ScriptComponent *sc_, ScriptEditHandler *editor_) :
		ThreadWithAsyncProgressWindow("Connect widget to module parameter"),
		sc(sc_),
		editor(editor_),
		sp(dynamic_cast<JavascriptMidiProcessor*>(editor_->getScriptEditHandlerProcessor())),
		processorToAdd(nullptr),
		parameterIndexToAdd(-1)
	{
		if (sp != nullptr)
		{
			Processor::Iterator<Processor> boxIter(sp->getOwnerSynth(), false);


			while (Processor *p = boxIter.getNextProcessor())
			{
				if (dynamic_cast<Chain*>(p) != nullptr) continue;

				processorList.add(p);
			}

			StringArray processorIdList;

			for (int i = 0; i < processorList.size(); i++)
			{
				processorIdList.add(processorList[i]->getId());
			}

			addComboBox("Processors", processorIdList, "Module");

			getComboBoxComponent("Processors")->addListener(this);



			addComboBox("Parameters", StringArray(), "Parameters");

			getComboBoxComponent("Parameters")->addListener(this);
			getComboBoxComponent("Parameters")->setTextWhenNothingSelected("Choose a module");

			addBasicComponents();

			showStatusMessage("Choose a module and its parameter and press OK");

			startTimer(50);
		}
		else
		{
			jassertfalse;
		}


	}

	void timerCallback()
	{
		selectProcessor();
		stopTimer();
	}

	void selectProcessor()
	{
		const String controlCode = sp->getSnippet(JavascriptMidiProcessor::onControl)->getAllContent();

		const String switchStatement = containsSwitchStatement(controlCode);

		if (switchStatement.isNotEmpty())
		{
			const String caseStatement = containsCaseStatement(switchStatement);

			if (switchStatement.isNotEmpty())
			{
				const String oldProcessorName = getOldProcessorName(caseStatement);
				const String oldParameterName = getOldParameterName(caseStatement, oldProcessorName).fromFirstOccurrenceOf(".", false, false);

				if (oldProcessorName.isNotEmpty())
				{
					ComboBox *b = getComboBoxComponent("Processors");

					for (int i = 0; i < b->getNumItems(); i++)
					{
						if (b->getItemText(i).removeCharacters(" \n\t\"\'!$%&/()") == oldProcessorName)
						{
							b->setSelectedItemIndex(i, dontSendNotification);
							comboBoxChanged(b);
							break;
						}
					}
				}

				if (oldParameterName.isNotEmpty())
				{
					ComboBox *b = getComboBoxComponent("Parameters");

					b->setText(oldParameterName, dontSendNotification);
					comboBoxChanged(b);

				}
			}
		}
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		if (comboBoxThatHasChanged->getName() == "Processors")
		{
			Processor *selectedProcessor = processorList[comboBoxThatHasChanged->getSelectedItemIndex()];

			ComboBox *parameterBox = getComboBoxComponent("Parameters");

			parameterBox->clear();

			if (ProcessorWithScriptingContent* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(selectedProcessor))
			{
				for (int i = 0; i < pwsc->getScriptingContent()->getNumComponents(); i++)
				{
					parameterBox->addItem("ScriptedParameters." + pwsc->getScriptingContent()->getComponent(i)->getName().toString(), i + 1);
				}
			}
			else
			{
				for (int i = 0; i < selectedProcessor->getNumParameters(); i++)
				{
					parameterBox->addItem(selectedProcessor->getIdentifierForParameterIndex(i).toString(), i + 1);
				}
			}



			setProgress(0.5);
		}
		else if (comboBoxThatHasChanged->getName() == "Parameters")
		{
			processorToAdd = processorList[getComboBoxComponent("Processors")->getSelectedItemIndex()];
			parameterIndexToAdd = comboBoxThatHasChanged->getSelectedItemIndex();

			setProgress(1.0);

			showStatusMessage("Press OK to add the connection code to this script.");
		}
	}

	void run() override
	{

	};

	void threadFinished() override
	{
		if (processorToAdd == nullptr || parameterIndexToAdd == -1) return;

		String onInitText = sp->getSnippet(JavascriptMidiProcessor::onInit)->getAllContent();
		String declaration = ProcessorHelpers::getScriptVariableDeclaration(processorToAdd, false);
		String processorId = declaration.fromFirstOccurrenceOf("const var ", false, false).upToFirstOccurrenceOf(" ", false, false);
		const String parameterId = getComboBoxComponent("Parameters")->getText();

		if (!onInitText.contains(declaration))
		{
			onInitText << "\n" << declaration << "\n";
			sp->getSnippet(JavascriptMidiProcessor::onInit)->replaceAllContent(onInitText);
		}

		const String onControlText = sp->getSnippet(JavascriptMidiProcessor::onControl)->getAllContent();

		const String switchStatement = containsSwitchStatement(onControlText);

		if (switchStatement.isNotEmpty())
		{
			String caseStatement = containsCaseStatement(switchStatement);

			if (caseStatement.isNotEmpty())
			{
				modifyCaseStatement(caseStatement, processorId, parameterId);
			}
			else
			{
				int index = getCaseStatementIndex(onControlText);
				addCaseStatement(index, processorId, parameterId);
			}
		}
		else
		{
			addSwitchStatementWithCaseStatement(onControlText, processorId, parameterId);
		}

		editor->compileScript();
	};

private:

	String containsSwitchStatement(const String &controlCode)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(controlCode, "");



			it.match(TokenTypes::function);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::openParen);
			it.match(TokenTypes::identifier);

			const Identifier widgetParameterName = Identifier(it.currentValue);

			it.match(TokenTypes::comma);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::closeParen);
			it.match(TokenTypes::openBrace);

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::switch_)
				{
					it.match(TokenTypes::switch_);
					it.match(TokenTypes::openParen);

					if (it.currentType == TokenTypes::identifier && Identifier(it.currentValue) == widgetParameterName)
					{
						it.match(TokenTypes::identifier);
						it.match(TokenTypes::closeParen);

						auto start = it.location.location;

						it.match(TokenTypes::openBrace);

						int braceLevel = 1;

						while (it.currentType != TokenTypes::eof && braceLevel > 0)
						{
							if (it.currentType == TokenTypes::openBrace) braceLevel++;
							else if (it.currentType == TokenTypes::closeBrace) braceLevel--;

							it.skip();
						}


						return String(start, it.location.location);
					}
				}

				it.skip();
			}

			return String();
		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at parsing the control statement", error, PresetHandler::IconType::Error);

			return String();
		}
	}

	String containsCaseStatement(const String &switchStatement)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(switchStatement, "");

			it.match(TokenTypes::openBrace);

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::case_)
				{
					it.match(TokenTypes::case_);

					const Identifier caseId = Identifier(it.currentValue.toString());

					it.match(TokenTypes::identifier);

					if (caseId == sc->getName())
					{
						it.match(TokenTypes::colon);

						auto start = it.location.location;

						while (it.currentType != TokenTypes::eof && it.currentType != TokenTypes::case_)
						{
							it.skip();
						}

						return String(start, it.location.location);
					}

				}

				it.skip();

			}

			return String();

		}
		catch (String errorMessage)
		{
			PresetHandler::showMessageWindow("Error at parsing the case statement", errorMessage, PresetHandler::IconType::Error);

			return String();
		}

	}

	String getOldProcessorName(const String &caseStatement)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(caseStatement, "");

			String previous2;
			String previous1;

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentValue == "setAttribute")
				{
					return previous1;
				}

				previous2 = previous1;
				previous1 = it.currentValue.toString();

				it.skip();
			}

			return String();
		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at modifying the case statement", error, PresetHandler::IconType::Error);

			return String();
		}
	}

	String getOldParameterName(const String &caseStatement, const String &processorName)
	{
		try
		{


			HiseJavascriptEngine::RootObject::TokenIterator it(caseStatement, "");

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentValue == processorName)
				{
					it.match(TokenTypes::identifier);
					it.match(TokenTypes::dot);

					if (it.currentValue == "setAttribute")
					{
						it.match(TokenTypes::identifier);
						it.match(TokenTypes::openParen);

						if (it.currentValue == processorName)
						{
							it.match(TokenTypes::identifier);
							it.match(TokenTypes::dot);

							return processorName + "." + it.currentValue.toString();
						}
						else
						{
							return String();
						}
					}
				}

				it.skip();
			}

			return String();

		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at modifying the case statement", error, PresetHandler::IconType::Error);
			return String();
		}
	}

	void modifyCaseStatement(const String &caseStatement, const String &processorId, const String &parameterId)
	{
		String newStatement = String(caseStatement);

		const String newParameterName = processorId + "." + parameterId;

		const String oldProcessorName = getOldProcessorName(caseStatement);
		const String oldParameterName = getOldParameterName(caseStatement, oldProcessorName);

		if (oldParameterName.isNotEmpty())
		{
			newStatement = caseStatement.replace(oldParameterName, newParameterName);
			newStatement = newStatement.replace(oldProcessorName, processorId);

			CodeDocument* doc = sp->getSnippet(JavascriptMidiProcessor::onControl);

			String allCode = doc->getAllContent();

			doc->replaceAllContent(allCode.replace(caseStatement, newStatement));
		}
	}

	void addCaseStatement(int &index, const String &processorId, const String &parameterId)
	{
		String codeToInsert;

		codeToInsert << "\t\tcase " << sc->getName().toString() << ":\n\t\t{\n\t\t\t";
		codeToInsert << processorId << ".setAttribute(" << processorId << "." << parameterId;
		codeToInsert << ", value);\n";
		codeToInsert << "\t\t\tbreak;\n\t\t}\n";

		sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, codeToInsert);

		index += codeToInsert.length();
	}

	void addSwitchStatementWithCaseStatement(const String &onControlText, const String &processorId, const String &parameterId)
	{
		const String switchStart = "\tswitch(number)\n\t{\n";
		const String switchEnd = "\t};\n";

		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(onControlText, "");

			it.match(TokenTypes::function);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::openParen);
			it.match(TokenTypes::identifier);

			const Identifier widgetParameterName = Identifier(it.currentValue);

			it.match(TokenTypes::comma);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::closeParen);
			it.match(TokenTypes::openBrace);

			int index = (int)(it.location.location - onControlText.getCharPointer());

			sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, switchStart);
			index += switchStart.length();
			addCaseStatement(index, processorId, parameterId);
			sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, switchEnd);
		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at adding the switch & case statement", error, PresetHandler::IconType::Error);
		}
	}

	int getCaseStatementIndex(const String &onControlText)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(onControlText, "");

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::switch_)
				{
					it.match(TokenTypes::switch_);
					it.match(TokenTypes::openParen);

					if (it.currentValue == "number")
					{
						it.match(TokenTypes::identifier);


						it.match(TokenTypes::closeParen);
						it.match(TokenTypes::openBrace);

						int braceLevel = 1;

						while (it.currentType != TokenTypes::eof && braceLevel > 0)
						{
							if (it.currentType == TokenTypes::openBrace) braceLevel++;

							else if (it.currentType == TokenTypes::closeBrace)
							{
								braceLevel--;
								if (braceLevel == 0)
								{
									return (int)(it.location.location - onControlText.getCharPointer() - 1);
								}
							}

							it.skip();
						}

					}
				}

				it.skip();
			}

			return -1;

		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at finding the case statement location", error, PresetHandler::IconType::Error);
			return -1;
		}
	}


	Processor *processorToAdd;
	int parameterIndexToAdd;

	ScriptingApi::Content::ScriptComponent *sc;
	ScriptEditHandler *editor;
	JavascriptMidiProcessor *sp;
	Array<WeakReference<Processor>> processorList;
};


void ScriptingContentOverlay::mouseDown(const MouseEvent& e)
{

	auto content = parentHandler->getScriptEditHandlerContent();

	jassert(content != nullptr);

	if (e.mods.isLeftButtonDown() && parentHandler->editModeEnabled())
	{
		ScriptingApi::Content::ScriptComponent *sc = content->getScriptComponentFor(e.getEventRelativeTo(content).getPosition());

		dynamic_cast<Processor*>(parentHandler->getScriptEditHandlerProcessor())->getMainController()->setEditedScriptComponent(sc, parentHandler->getAsComponent());
	}

	if (e.mods.isRightButtonDown())
	{
		enum ComponentOffsets
		{
			connectComponentOffset = 5000,
			addCallbackOffset = 10000,
			showCallbackOffset = 15000,
			editComponentOffset = 20000
		};

		PopupMenu m;
		ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
		m.setLookAndFeel(luf);

		Array<ScriptingApi::Content::ScriptComponent*> components;

		parentHandler->getScriptEditHandlerContent()->getScriptComponentsFor(components, e.getEventRelativeTo(content).getPosition());

		if (parentHandler->editModeEnabled())
		{
			m.addSectionHeader("Create new widget");
			m.addItem((int)ScriptEditHandler::Widgets::Knob, "Add new Slider");
			m.addItem((int)ScriptEditHandler::Widgets::Button, "Add new Button");
			m.addItem((int)ScriptEditHandler::Widgets::Table, "Add new Table");
			m.addItem((int)ScriptEditHandler::Widgets::ComboBox, "Add new ComboBox");
			m.addItem((int)ScriptEditHandler::Widgets::Label, "Add new Label");
			m.addItem((int)ScriptEditHandler::Widgets::Image, "Add new Image");
			m.addItem((int)ScriptEditHandler::Widgets::Plotter, "Add new Plotter");
			m.addItem((int)ScriptEditHandler::Widgets::ModulatorMeter, "Add new ModulatorMeter");
			m.addItem((int)ScriptEditHandler::Widgets::Panel, "Add new Panel");
			m.addItem((int)ScriptEditHandler::Widgets::AudioWaveform, "Add new AudioWaveform");
			m.addItem((int)ScriptEditHandler::Widgets::SliderPack, "Add new SliderPack");

			m.addItem((int)ScriptEditHandler::Widgets::duplicateWidget, "Duplicate selected component", content->getEditedComponent() != nullptr);

			if (components.size() != 0)
			{
				m.addSeparator();

				if (components.size() == 1)
				{
					m.addItem(editComponentOffset, "Edit \"" + components[0]->getName().toString() + "\" in Panel");
					m.addItem(connectComponentOffset, "Connect to Module Parameter");
					m.addItem(addCallbackOffset, "Add custom callback for " + components[0]->getName().toString(), components[0]->getCustomControlCallback() == nullptr);
					m.addItem(showCallbackOffset, "Show custom callback for " + components[0]->getName().toString(), components[0]->getCustomControlCallback() != nullptr);
				}
				else
				{
					PopupMenu editSub;
					PopupMenu connectSub;
					PopupMenu addSub;
					PopupMenu showSub;

					for (int i = 0; i < components.size(); i++)
					{
						editSub.addItem(editComponentOffset + i, components[i]->getName().toString());
						connectSub.addItem(connectComponentOffset + i, components[i]->getName().toString());
						addSub.addItem(addCallbackOffset + i, components[i]->getName().toString(), components[0]->getCustomControlCallback() == nullptr);
						showSub.addItem(showCallbackOffset + i, components[i]->getName().toString(), components[0]->getCustomControlCallback() != nullptr);
					}

					m.addSubMenu("Edit in Panel", editSub, components.size() != 0);
					m.addSubMenu("Connect to Module Parameter", connectSub, components.size() != 0);
					m.addSubMenu("Add custom callback for", addSub, components.size() != 0);
					m.addSubMenu("Show custom callback for", showSub, components.size() != 0);
				}
			}
		}
		else
		{
			return;
		}

		int result = m.show();

		if (result >= (int)ScriptEditHandler::Widgets::Knob && result < (int)ScriptEditHandler::Widgets::numWidgets)
		{
			const int insertX = e.getEventRelativeTo(content).getMouseDownPosition().getX();
			const int insertY = e.getEventRelativeTo(content).getMouseDownPosition().getY();

			parentHandler->createNewComponent((ScriptEditHandler::Widgets)result, insertX, insertY);
		}
		else if (result >= editComponentOffset) // EDIT IN PANEL
		{
			ReferenceCountedObject* sc = components[result - editComponentOffset];

			dynamic_cast<Processor*>(parentHandler->getScriptEditHandlerProcessor())->getMainController()->setEditedScriptComponent(sc, dynamic_cast<Component*>(parentHandler));
		}
		else if (result >= showCallbackOffset)
		{
			auto componentToUse = components[result - showCallbackOffset];

			auto func = dynamic_cast<DebugableObject*>(componentToUse->getCustomControlCallback());

			if (func != nullptr)
				func->doubleClickCallback(e, dynamic_cast<Component*>(parentHandler));
		}
		else if (result >= addCallbackOffset)
		{
			auto componentToUse = components[result - addCallbackOffset];

			auto name = componentToUse->getName();

			NewLine nl;
			String code;


			String callbackName = "on" + name.toString() + "Control";

			code << nl;
			code << "inline function " << callbackName << "(component, value)" << nl;
			code << "{" << nl;
			code << "\t//Add your custom logic here..." << nl;
			code << "};" << nl;
			code << nl;
			code << name.toString() << ".setControlCallback(" << callbackName << ");" << nl;

			auto ed = parentHandler->getScriptEditHandlerEditor();

			if (ed->selectJSONTag(name))
			{
				auto insertPos = ed->getHighlightedRegion().getEnd();
				ed->moveCaretTo(CodeDocument::Position(ed->getDocument(), insertPos), false);

				ed->insertTextAtCaret(nl);
			}
			else
			{
				ed->selectLineAfterDefinition(name);
			}

			ed->insertTextAtCaret(code);

			parentHandler->compileScript();
		}
		else if (result >= connectComponentOffset)
		{
			auto componentToUse = components[result - connectComponentOffset];

			if (dynamic_cast<JavascriptMidiProcessor*>(parentHandler->getScriptEditHandlerProcessor()))
			{
				ParameterConnector *comp = new ParameterConnector(componentToUse, parentHandler);

				comp->setModalBaseWindowComponent(parentHandler->getAsComponent()->findParentComponentOfClass<BackendRootWindow>());
			}

		}
	}
}

ScriptingContentOverlay::Dragger::Dragger(ScriptEditHandler* parentHandler_) :
	parentHandler(parentHandler_)
{
	constrainer.setMinimumOnscreenAmounts(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF);

	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

	resizer->addMouseListener(this, true);

	setWantsKeyboardFocus(true);
}

ScriptingContentOverlay::Dragger::~Dragger()
{
	setDraggedControl(nullptr, nullptr);
}

void ScriptingContentOverlay::Dragger::paint(Graphics &g)
{
	g.fillAll(Colours::black.withAlpha(0.2f));
	g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));

	if (!snapShot.isNull()) g.drawImageAt(snapShot, 0, 0);

	g.drawRect(getLocalBounds(), 1);

	if (copyMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));
		g.drawText("+", getLocalBounds().withTrimmedLeft(2).expanded(0, 4), Justification::topLeft);
	}
}

void ScriptingContentOverlay::Dragger::mouseDown(const MouseEvent& e)
{
	constrainer.setStartPosition(getBounds());

	if (e.eventComponent == this)
	{
		snapShot = currentlyDraggedComponent->createComponentSnapshot(currentlyDraggedComponent->getLocalBounds());

		dragger.startDraggingComponent(this, e);
	}

	if (e.mods.isRightButtonDown())
	{
		dynamic_cast<Processor*>(parentHandler->getScriptEditHandlerProcessor())->getMainController()->setEditedScriptComponent(nullptr, parentHandler->getAsComponent());
	}
}

void ScriptingContentOverlay::Dragger::mouseDrag(const MouseEvent& e)
{
	constrainer.setRasteredMovement(e.mods.isCommandDown());
	constrainer.setLockedMovement(e.mods.isShiftDown());

	copyMode = e.mods.isAltDown();

	if (e.eventComponent == this)
		dragger.dragComponent(this, e, &constrainer);
}

void ScriptingContentOverlay::Dragger::mouseUp(const MouseEvent& e)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	snapShot = Image();

	if (copyMode)
	{
		const int oldX = sc->getPosition().getX();
		const int oldY = sc->getPosition().getY();

		const int newX = oldX + constrainer.getDeltaX();
		const int newY = oldY + constrainer.getDeltaY();

		parentHandler->createNewComponent(ScriptEditHandler::Widgets::duplicateWidget, newX, newY);

		copyMode = false;

		repaint();
		return;
	}

	repaint();

	if (sc != nullptr)
	{
		undoManager.beginNewTransaction();

		if (e.eventComponent == this)
		{
			undoManager.perform(new OverlayAction(this, false, constrainer.getDeltaX(), constrainer.getDeltaY()));
		}
		else
		{
			undoManager.perform(new OverlayAction(this, true, constrainer.getDeltaWidth(), constrainer.getDeltaHeight()));
		}
	}
}

void ScriptingContentOverlay::Dragger::moveOverlayedComponent(int deltaX, int deltaY)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	const int oldX = sc->getPosition().getX();
	const int oldY = sc->getPosition().getY();

	const int newX = oldX + deltaX;
	const int newY = oldY + deltaY;

	parentHandler->changePositionOfComponent(sc, newX, newY);
}

void ScriptingContentOverlay::Dragger::resizeOverlayedComponent(int deltaX, int deltaY)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	const int oldWidth = sc->getPosition().getWidth();
	const int oldHeight = sc->getPosition().getHeight();

	const int newWidth = oldWidth + deltaX;
	const int newHeight = oldHeight + deltaY;

	sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width), newWidth, dontSendNotification);
	sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::height), newHeight, sendNotification);
	sc->setChanged();

	parentHandler->scriptComponentChanged(sc, sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width));
}

void ScriptingContentOverlay::Dragger::setDraggedControl(Component* componentToDrag, ScriptingApi::Content::ScriptComponent* sc)
{
	if (componentToDrag != nullptr && componentToDrag != currentlyDraggedComponent.getComponent())
	{
		currentScriptComponent = sc;
		currentlyDraggedComponent = componentToDrag;

		currentMovementWatcher = new MovementWatcher(componentToDrag, this);

		auto c = componentToDrag->findParentComponentOfClass<ScriptContentComponent>();

		if (c != nullptr)
		{
			auto boundsInParent = c->getLocalArea(componentToDrag->getParentComponent(), componentToDrag->getBoundsInParent());
			setBounds(boundsInParent);
		}

		setVisible(true);
		setWantsKeyboardFocus(true);

		setAlwaysOnTop(true);
		grabKeyboardFocus();
	}
	else if (componentToDrag == nullptr)
	{
		currentScriptComponent = nullptr;
		currentlyDraggedComponent = nullptr;
		currentMovementWatcher = nullptr;
		setBounds(Rectangle<int>());
		setVisible(false);
		setWantsKeyboardFocus(false);
		setAlwaysOnTop(false);
	}
}


void ScriptingContentOverlay::Dragger::MovementWatcher::componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/)
{
	auto c = getComponent()->findParentComponentOfClass<ScriptContentComponent>();

	if (c != nullptr)
	{
		auto boundsInParent = c->getLocalArea(getComponent()->getParentComponent(), getComponent()->getBoundsInParent());
		dragComponent->setBounds(boundsInParent);
	}
}

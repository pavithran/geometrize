#include "imagetaskprepostscriptswidget.h"
#include "ui_imagetaskprepostscriptswidget.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include <QEvent>
#include <QList>
#include <QLocale>
#include <QMessageBox>
#include <QString>

#include "chaiscript/chaiscript.hpp"

#include "dialog/imagetaskscriptingpanel.h"
#include "dialog/scripteditorwidget.h"
#include "preferences/globalpreferences.h"
#include "script/chaiscriptcreator.h"

namespace geometrize
{

namespace dialog
{

void showImageTaskStopConditionMetMessage(QWidget* parent)
{
    QMessageBox::information(parent, QObject::tr("Stop Condition Met"), QObject::tr("Stop condition for geometrizing was met"));
}

class ImageTaskPrePostScriptsWidget::ImageTaskPrePostScriptsWidgetImpl
{
public:
    ImageTaskPrePostScriptsWidgetImpl(ImageTaskPrePostScriptsWidget* pQ) : q{pQ}, ui{std::make_unique<Ui::ImageTaskPrePostScriptsWidget>()}, m_stopConditionId{0}, m_engine{nullptr}
    {
        ui->setupUi(q);
        populateUi();

        new geometrize::dialog::ImageTaskScriptingPanel(q);

        m_engine = createEngine();

        connect(ui->editShapeScriptsButton, &QPushButton::clicked, [this]() {
            revealShapeScriptingPanel();
        });
        connect(ui->addStopConditionButton, &QPushButton::clicked, [this]() {
            const std::string defaultCode = "shapeCount >= 1000;";
            addStopCondition(defaultCode);
        });
        connect(ui->clearScriptsButton, &QPushButton::clicked, [this]() {
            QLayoutItem* item = nullptr;
            while((item = ui->scriptEditorLayout->takeAt(0)) != nullptr) {
                delete item->widget();
                delete item;
            }

            m_stopConditionId = 0;
        });

        // Reveal shape scripting panel if global preferences are set this way
        const geometrize::preferences::GlobalPreferences& prefs{geometrize::preferences::getGlobalPreferences()};
        if(prefs.shouldShowImageTaskScriptEditorByDefault()) {
            revealShapeScriptingPanel();
        }
    }
    ~ImageTaskPrePostScriptsWidgetImpl() = default;
    ImageTaskPrePostScriptsWidgetImpl operator=(const ImageTaskPrePostScriptsWidgetImpl&) = delete;
    ImageTaskPrePostScriptsWidgetImpl(const ImageTaskPrePostScriptsWidgetImpl&) = delete;

    void setImageTask(task::ImageTask* task)
    {
        getShapeScriptingPanel()->setImageTask(task);
    }

    void syncUserInterface()
    {

    }

    void addStopCondition(const std::string& scriptCode)
    {
        const std::string editorName = tr("Custom Stop Condition").toStdString();

        const std::string functionName = "stop_condition_" + std::to_string(m_stopConditionId++);

        auto widget = new geometrize::dialog::ScriptEditorWidget(editorName, "", scriptCode, ui->scriptsGroupBox);
        ui->scriptEditorLayout->addWidget(widget);
    }

    bool stopConditionsMet(const std::size_t currentShapeCount) const
    {
        if(!m_engine) {
            assert(0 && "Failed to evaluate stop conditions, no script engine is set");
            return false;
        }

        m_engine->set_global(chaiscript::var(currentShapeCount), "shapeCount");

        const auto scriptWidgets = ui->scriptsGroupBox->findChildren<geometrize::dialog::ScriptEditorWidget*>();
        if(scriptWidgets.empty()) {
            return false;
        }

        bool scriptStopConditionMet = false;
        for(const auto& widget : scriptWidgets) {
            try {
                auto stopCondition = m_engine->eval<bool>(widget->getCurrentCode());
                if(stopCondition) {
                    scriptStopConditionMet = true;
                }
                widget->onScriptEvaluationSucceeded();
            } catch(const chaiscript::exception::eval_error& e) {
                widget->onScriptEvaluationFailed(e.pretty_print());
            } catch(...) {
                widget->onScriptEvaluationFailed("Unknown script evaluation error");
            }
        }

        return scriptStopConditionMet;
    }

    void onLanguageChange()
    {
        ui->retranslateUi(q);
        populateUi();
    }

private:
    // Utility function used to setup and display the script editor for the given image task window
    void revealShapeScriptingPanel()
    {
        if(dialog::ImageTaskScriptingPanel* scriptingPanel = getShapeScriptingPanel()) {
            scriptingPanel->setWindowState(scriptingPanel->windowState() & ~Qt::WindowMinimized);
            QApplication::setActiveWindow(scriptingPanel);
            scriptingPanel->raise();
            scriptingPanel->show();
        }
    }

    geometrize::dialog::ImageTaskScriptingPanel* getShapeScriptingPanel()
    {
        return q->findChild<geometrize::dialog::ImageTaskScriptingPanel*>();
    }

    std::unique_ptr<chaiscript::ChaiScript> createEngine()
    {
        return geometrize::script::createDefaultEngine();
    }

    void populateUi()
    {
    }

    ImageTaskPrePostScriptsWidget* q;
    std::unique_ptr<Ui::ImageTaskPrePostScriptsWidget> ui;

    int m_stopConditionId;
    std::unique_ptr<chaiscript::ChaiScript> m_engine;
};

ImageTaskPrePostScriptsWidget::ImageTaskPrePostScriptsWidget(QWidget* parent) :
    QWidget{parent},
    d{std::make_unique<ImageTaskPrePostScriptsWidget::ImageTaskPrePostScriptsWidgetImpl>(this)}
{
}

ImageTaskPrePostScriptsWidget::~ImageTaskPrePostScriptsWidget()
{
}

void ImageTaskPrePostScriptsWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        d->onLanguageChange();
    }
    QWidget::changeEvent(event);
}

void ImageTaskPrePostScriptsWidget::setImageTask(task::ImageTask* task)
{
    d->setImageTask(task);
}

void ImageTaskPrePostScriptsWidget::syncUserInterface()
{
    d->syncUserInterface();
}

void ImageTaskPrePostScriptsWidget::addStopCondition(const std::string& scriptCode)
{
    d->addStopCondition(scriptCode);
}

bool ImageTaskPrePostScriptsWidget::stopConditionsMet(const std::size_t currentShapeCount) const
{
    return d->stopConditionsMet(currentShapeCount);
}

}

}
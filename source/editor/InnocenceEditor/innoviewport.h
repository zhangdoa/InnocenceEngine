#ifndef INNOVIEWPORT_H
#define INNOVIEWPORT_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>

#include "../../engine/system/CoreSystem.h"
#include "../../game/GameInstance.h"

class ViewportEventFilter : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class InnoViewport : public QWidget
{
    Q_OBJECT

public:
    explicit InnoViewport(QWidget* parent);
    virtual ~InnoViewport() override;

    void initialize();

    ViewportEventFilter* m_viewportEventFilter;

protected:
    virtual QPaintEngine* paintEngine() const override { return NULL; }

    void paintEvent(QPaintEvent* paintEvent) override;

    void showEvent(QShowEvent* showEvent) override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

private:
    QTimer* m_timerUpdate;

    InnoCoreSystem* m_CoreSystem;
    GameInstance* m_GameInstance;

    void Resize(float width, float height);

public slots:
    void Update();
};

#endif // INNOVIEWPORT_H
